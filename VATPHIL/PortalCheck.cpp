#include "stdafx.h"
#include "PortalCheck.h"
#include "./inc/rapidjson/document.h"
#include <winhttp.h>
#include <fstream>
#include <cctype>

#pragma comment(lib, "winhttp.lib")

static const int PORTAL_TIMEOUT_MS = 8000;
static const DWORD MAX_BODY = 64 * 1024;

static std::wstring ToWide(const std::string& in)
{
	if (in.empty())
		return std::wstring();
	int wlen = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), NULL, 0);
	std::wstring out(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), &out[0], wlen);
	return out;
}

static std::string UrlEncode(const std::string& in)
{
	static const char* hex = "0123456789ABCDEF";
	std::string out;
	for (unsigned char c : in)
	{
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			out += (char)c;
		else
		{
			out += '%';
			out += hex[c >> 4];
			out += hex[c & 15];
		}
	}
	return out;
}

PortalCheck::~PortalCheck()
{
	Join();
}

void PortalCheck::Join()
{
	if (worker.joinable())
		worker.join();
}

bool PortalCheck::Start(const std::string& url, const std::string& callsign, const std::string& sectorFiles)
{
	if (running)
		return false;
	Join();
	done = false;
	running = true;
	const std::string full = url + "?callsign=" + UrlEncode(callsign) + "&sct=" + UrlEncode(sectorFiles);
	worker = std::thread([this, full]()
	{
		result = Fetch(full);
		done = true;
		running = false;
	});
	return true;
}

bool PortalCheck::Poll(Result& out)
{
	if (!done)
		return false;
	Join();
	done = false;
	out = result;
	return true;
}

PortalCheck::Result PortalCheck::Fetch(const std::string& url)
{
	Result r;
	std::wstring wurl = ToWide(url);
	URL_COMPONENTS parts;
	memset(&parts, 0, sizeof(parts));
	parts.dwStructSize = sizeof(parts);
	parts.dwHostNameLength = (DWORD)-1;
	parts.dwUrlPathLength = (DWORD)-1;
	parts.dwExtraInfoLength = (DWORD)-1;
	if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &parts))
		return r;
	std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
	std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
	if (parts.lpszExtraInfo != NULL)
		path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

	std::string body;
	DWORD http_status = 0;
	HINTERNET session = WinHttpOpen(L"VATPHIL-EuroScope", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	HINTERNET connection = NULL;
	HINTERNET request = NULL;
	if (session != NULL)
	{
		WinHttpSetTimeouts(session, PORTAL_TIMEOUT_MS, PORTAL_TIMEOUT_MS, PORTAL_TIMEOUT_MS, PORTAL_TIMEOUT_MS);
		connection = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
	}
	if (connection != NULL)
	{
		DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
		request = WinHttpOpenRequest(connection, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	}
	if (request != NULL
		&& WinHttpSendRequest(request, L"Accept: application/json\r\n", (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
		&& WinHttpReceiveResponse(request, NULL))
	{
		DWORD size = sizeof(http_status);
		WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &http_status, &size, WINHTTP_NO_HEADER_INDEX);
		for (;;)
		{
			DWORD available = 0;
			if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
				break;
			if (body.size() + available > MAX_BODY)
				break;
			std::string chunk(available, '\0');
			DWORD read = 0;
			if (!WinHttpReadData(request, &chunk[0], available, &read) || read == 0)
				break;
			body.append(chunk, 0, read);
		}
	}
	if (request != NULL) WinHttpCloseHandle(request);
	if (connection != NULL) WinHttpCloseHandle(connection);
	if (session != NULL) WinHttpCloseHandle(session);

	// Only a normal answer counts: "slow down" (429) or a portal error (503) is a failed check, which
	// never removes a screen that's already up
	if (http_status != 200 || body.empty())
		return r;
	rapidjson::Document doc;
	doc.Parse(body.c_str());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("status") || !doc["status"].IsString())
		return r;

	const std::string status = doc["status"].GetString();
	if (status == "ok")
		r.status = Status::Ok;
	else if (status == "blocked")
		r.status = Status::Blocked;
	else if (status == "waiting")
		r.status = Status::Waiting;
	else
		return r;

	if (doc.HasMember("messages") && doc["messages"].IsArray())
		for (auto& m : doc["messages"].GetArray())
			if (m.IsString())
				r.messages.push_back(m.GetString());
	// A "blocked" with nothing to say would be a popup nobody can act on
	if (r.status == Status::Blocked && r.messages.empty())
		r.status = Status::Ok;
	return r;
}

// The .sct files in the sector package: the folder the Updater installs to, which holds one per AIRAC
// ("2612.sct"). The DLL sits in <package>\RPHI\Plugins\VATPHIL\, so look in each folder above it.
std::string PortalCheck::FindSectorFiles()
{
	HMODULE self = NULL;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&PortalCheck::FindSectorFiles, &self))
		return std::string();
	char buf[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(self, buf, MAX_PATH) == 0)
		return std::string();
	std::string dir = buf;
	for (int level = 0; level < 5; level++)
	{
		size_t cut = dir.find_last_of("\\/");
		if (cut == std::string::npos)
			break;
		dir = dir.substr(0, cut);
		std::string names;
		WIN32_FIND_DATAA found;
		HANDLE h = FindFirstFileA((dir + "\\*.sct").c_str(), &found);
		if (h == INVALID_HANDLE_VALUE)
			continue;
		do
		{
			if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			std::string name = found.cFileName;
			name = name.substr(0, name.size() - 4); // drop ".sct"
			if (name.empty() || name.find(',') != std::string::npos || names.size() + name.size() > 300)
				continue;
			if (!names.empty())
				names += ",";
			names += name;
		} while (FindNextFileA(h, &found));
		FindClose(h);
		if (!names.empty())
			return names;
	}
	return std::string();
}
