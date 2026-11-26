#include "stdafx.h"
#include "WebhookSender.h"
#include "./inc/rapidjson/stringbuffer.h"
#include "./inc/rapidjson/writer.h"
#include <winhttp.h>
#include <chrono>

#pragma comment(lib, "winhttp.lib")

// Discord rejects messages over 2000 characters; leave some headroom.
static const size_t MAX_CONTENT = 1900;
static const size_t MAX_QUEUED = 200;
// Webhooks allow roughly 30 messages a minute per channel.
static const std::chrono::milliseconds SEND_INTERVAL(2500);
static const int HTTP_TIMEOUT_MS = 5000;

std::string WebhookSender::AnsiToUtf8(const std::string& in)
{
	if (in.empty())
		return in;
	int wlen = MultiByteToWideChar(CP_ACP, 0, in.c_str(), (int)in.size(), NULL, 0);
	std::wstring wide(wlen, L'\0');
	MultiByteToWideChar(CP_ACP, 0, in.c_str(), (int)in.size(), &wide[0], wlen);
	int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wlen, NULL, 0, NULL, NULL);
	std::string out(len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), wlen, &out[0], len, NULL, NULL);
	return out;
}

static std::wstring Utf8ToWide(const std::string& in)
{
	if (in.empty())
		return std::wstring();
	int wlen = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), NULL, 0);
	std::wstring out(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), (int)in.size(), &out[0], wlen);
	return out;
}

// Cuts a UTF-8 string to at most max bytes without splitting a character.
static void TruncateUtf8(std::string& s, size_t max)
{
	if (s.size() <= max)
		return;
	size_t end = max;
	while (end > 0 && (static_cast<unsigned char>(s[end]) & 0xC0) == 0x80)
		end--;
	s.resize(end);
}

WebhookSender::WebhookSender()
{
	worker = std::thread(&WebhookSender::Run, this);
}

WebhookSender::~WebhookSender()
{
	Stop();
}

void WebhookSender::Enqueue(const std::string& url, const std::string& username, const std::string& line)
{
	if (url.empty())
		return;
	std::string utf8 = line;
	TruncateUtf8(utf8, MAX_CONTENT);
	{
		std::lock_guard<std::mutex> guard(lock);
		if (stopping)
			return;
		if (queue.size() >= MAX_QUEUED)
			queue.pop_front();
		queue.push_back({ url, username, utf8 });
	}
	wake.notify_one();
}

int WebhookSender::TestPost(const std::string& url, const std::string& username, const std::string& content)
{
	return Post(url, username, content);
}

void WebhookSender::Stop()
{
	{
		std::lock_guard<std::mutex> guard(lock);
		stopping = true;
	}
	wake.notify_one();
	if (worker.joinable())
		worker.join();
}

void WebhookSender::Run()
{
	std::unique_lock<std::mutex> guard(lock);
	for (;;)
	{
		wake.wait(guard, [this] { return stopping || !queue.empty(); });
		if (stopping)
			return;

		// Batch queued lines for the same webhook into one message.
		Item first = queue.front();
		queue.pop_front();
		std::string content = first.line;
		while (!queue.empty()
			&& queue.front().url == first.url
			&& queue.front().username == first.username
			&& content.size() + 1 + queue.front().line.size() <= MAX_CONTENT)
		{
			content += "\n";
			content += queue.front().line;
			queue.pop_front();
		}

		guard.unlock();
		Post(first.url, first.username, content);
		guard.lock();

		wake.wait_for(guard, SEND_INTERVAL, [this] { return stopping; });
	}
}

int WebhookSender::Post(const std::string& url, const std::string& username, const std::string& content)
{
	rapidjson::StringBuffer body;
	rapidjson::Writer<rapidjson::StringBuffer> writer(body);
	writer.StartObject();
	writer.Key("content");
	writer.String(content.c_str(), (rapidjson::SizeType)content.size());
	if (!username.empty())
	{
		writer.Key("username");
		writer.String(username.c_str(), (rapidjson::SizeType)username.size());
	}
	// Never let chat text ping @everyone, roles or users.
	writer.Key("allowed_mentions");
	writer.StartObject();
	writer.Key("parse");
	writer.StartArray();
	writer.EndArray();
	writer.EndObject();
	writer.EndObject();

	std::wstring wurl = Utf8ToWide(url);
	URL_COMPONENTS parts;
	memset(&parts, 0, sizeof(parts));
	parts.dwStructSize = sizeof(parts);
	parts.dwHostNameLength = (DWORD)-1;
	parts.dwUrlPathLength = (DWORD)-1;
	parts.dwExtraInfoLength = (DWORD)-1;
	if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &parts))
		return 0;
	std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
	std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
	if (parts.lpszExtraInfo != NULL)
		path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

	int http_status = 0;
	HINTERNET session = WinHttpOpen(L"VATPHIL-EuroScope", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	HINTERNET connection = NULL;
	HINTERNET request = NULL;
	if (session != NULL)
	{
		WinHttpSetTimeouts(session, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS);
		connection = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
	}
	if (connection != NULL)
	{
		DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
		request = WinHttpOpenRequest(connection, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	}
	if (request != NULL)
	{
		const wchar_t* headers = L"Content-Type: application/json\r\n";
		if (WinHttpSendRequest(request, headers, (DWORD)-1L, (LPVOID)body.GetString(), (DWORD)body.GetSize(), (DWORD)body.GetSize(), 0)
			&& WinHttpReceiveResponse(request, NULL))
		{
			DWORD status = 0;
			DWORD size = sizeof(status);
			if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX))
				http_status = (int)status;
		}
	}
	if (request != NULL) WinHttpCloseHandle(request);
	if (connection != NULL) WinHttpCloseHandle(connection);
	if (session != NULL) WinHttpCloseHandle(session);
	return http_status;
}
