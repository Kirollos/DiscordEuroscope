/*
	Copyright(C) 2023-2024 Kirollos Nashaat

	This program is free software : you can redistribute it and /or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "config.h"
#include "VATPHILExt.h"
#include "ConfigManager.h"
#include "MessageFormatter.h"
#include <ctime>
#include <cstring>

// The webhook URL is compiled into the DLL instead of being read from the config.
// This 256-byte block is replaced in the built DLL by tools/bake_webhook.py; the
// public release build keeps the placeholder. It is exported so the linker keeps
// the whole block in the image.
extern "C" __declspec(dllexport) const char g_bakedWebhookUrl[256] = WEBHOOK_BAKED_URL_PLACEHOLDER;

// Close EuroScope was pressed but EuroScope is still connected this long after: show the screen again.
static const int RESHOW_AFTER_CLOSE_S = 20;

namespace
{
	const char* const BAKED_WEBHOOK_PLACEHOLDER_PREFIX = "https://bake.invalid/";

	bool HasBakedWebhookUrl()
	{
		return g_bakedWebhookUrl[0] != '\0'
			&& std::strncmp(g_bakedWebhookUrl, BAKED_WEBHOOK_PLACEHOLDER_PREFIX, std::strlen(BAKED_WEBHOOK_PLACEHOLDER_PREFIX)) != 0;
	}

	std::string BakedWebhookUrl()
	{
		return std::string(g_bakedWebhookUrl);
	}

	std::string CurrentUtcTime()
	{
		std::time_t now = std::time(nullptr);
		std::tm utc_buf = {};
		gmtime_s(&utc_buf, &now);
		char buffer[16] = { 0 };
		std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &utc_buf);
		return std::string(buffer);
	}
}

// Returns the Wine version string, or nullptr when running on real Windows.
static const char* GetWineVersion()
{
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL)
		return nullptr;
	typedef const char* (CDECL* wine_get_version_t)(void);
	wine_get_version_t wine_get_version = (wine_get_version_t)GetProcAddress(ntdll, "wine_get_version");
	if (wine_get_version == NULL)
		return nullptr;
	return wine_get_version();
}

// Checks whether any discord-ipc pipe exists, without connecting to it.
static bool DiscordPipeExists()
{
	char pipeName[] = "\\\\.\\pipe\\discord-ipc-0";
	const size_t pipeDigit = sizeof(pipeName) - 2;
	for (char digit = '0'; digit <= '9'; digit++)
	{
		pipeName[pipeDigit] = digit;
		// Busy (ERROR_SEM_TIMEOUT) still means the pipe exists.
		if (WaitNamedPipeA(pipeName, 1) || GetLastError() != ERROR_FILE_NOT_FOUND)
			return true;
	}
	return false;
}

void VATPHILExt::CheckWineBridge(bool announce_ok)
{
	const char* wine_version = GetWineVersion();
	if (wine_version == nullptr)
		return;

	std::string msg = "Running under Wine ";
	msg += wine_version;
	if (DiscordPipeExists())
	{
		if (announce_ok)
		{
			msg += ". Discord bridge found.";
			DisplayUserMessage("Message", PLUGIN_NAME, msg.c_str(), true, true, false, true, false);
		}
		return;
	}
	msg += ". Discord can't be reached from Wine on its own, so rich presence won't show.";
	DisplayUserMessage("Message", PLUGIN_NAME, msg.c_str(), true, true, false, true, false);
	DisplayUserMessage("Message", PLUGIN_NAME, "Install rpc-bridge (github.com/EnderIce2/rpc-bridge) in this Wine prefix, make sure Discord is open, then type .discord hotreload", true, true, false, true, false);
}

VATPHILExt::VATPHILExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, "VATPHIL", "GNU GPLv3, based on DiscordEuroscope by Kirollos Nashaat")
{
	// Set first, so the portal check runs even if the config below fails to load
	this->EuroInittime = (int)time(NULL);
	popup.onTryAgain = [this]() { StartAccessCheck(); };
	// The sector file is checked as soon as EuroScope loads the plugin, before anyone can connect
	nextCheckAt = time(NULL) + 3;
	popup.onCloseEuroScope = [this]()
	{
		// The screen covers EuroScope, so this is the way out. If EuroScope doesn't close (a save prompt
		// they cancel) and is still connected, the screen comes back.
		HWND es = AccessPopup::EuroScopeWindow();
		popup.Close();
		reshowAt = time(NULL) + RESHOW_AFTER_CLOSE_S;
		if (es != NULL)
			PostMessage(es, WM_CLOSE, 0, 0);
	};

	char DllPathFile[_MAX_PATH];
	std::string RCPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	RCPath = DllPathFile;
	RCPath = RCPath.substr(0, RCPath.find_last_of('\\') + 1);
	RCPath += CONFIG_FILENAME;
	
	try {
		config.Init(RCPath);
	}
	catch (std::exception& e)
	{
		if (std::string(e.what()).find("Failed to open") != std::string::npos)
		{
			// File cannot be opened (normal on first run), quietly generate it
			config.GenerateConfigFile(RCPath);
			try {
				config.Init(RCPath);
			}
			catch (std::exception& e2)
			{
				std::string error_msg = "Error: ";
				error_msg += e2.what();
				error_msg += ".. Halting..";
				DisplayUserMessage("Message", PLUGIN_NAME, error_msg.c_str(), true, true, false, true, false);
				return;
			}
		}
		else
		{
			std::string error_msg = "Error: ";
			error_msg += e.what();
			DisplayUserMessage("Message", PLUGIN_NAME, error_msg.c_str(), true, true, false, true, false);
			return;
		}
	}
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	// handlers
	Discord_Initialize(static_cast<const char*>(config.Data().discord_appid.c_str()), &handlers, 1, NULL);
	// Stay quiet on startup; only speak up when something is wrong.
	CheckWineBridge(false);
	this->EuroInittime = (int)time(NULL);
	config.LoadRadioCallsigns();
}


VATPHILExt::~VATPHILExt()
{
	config.Cleanup();
}

VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	using namespace VATPHIL_Configuration;
	if (pMyPlugIn == nullptr)
		return;
	if (pMyPlugIn->EuroInittime == 0)
		return;
	pMyPlugIn->TickAccessCheck();
	if (!pMyPlugIn->config.isReady())
	{
		Discord_ClearPresence();
		Discord_RunCallbacks();
		return;
	}
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	const ConfigData data = pMyPlugIn->config.Data();
	discordPresence.largeImageKey = data.discord_presence_large_image_key.c_str();
	discordPresence.smallImageKey = data.discord_presence_small_image_key.c_str();
	discordPresence.startTimestamp = pMyPlugIn->EuroInittime;
	
	DiscordButton buttons[2] = { nullptr, nullptr };
	Button configButtons[2];

	switch (pMyPlugIn->GetConnectionType())
	{
		using namespace EuroScopePlugIn;
	case CONNECTION_TYPE_NO:
		discordPresence.details = data.states[State_Idle].details.c_str();
		discordPresence.largeImageKey = ConfigData::LocalOrGlobal(data.states[State_Idle].presence_large_image_key, data.discord_presence_large_image_key).c_str();
		discordPresence.largeImageText = data.states[State_Idle].presence_large_image_text.c_str();
		discordPresence.smallImageKey = ConfigData::LocalOrGlobal(data.states[State_Idle].presence_small_image_key, data.discord_presence_small_image_key).c_str();
		discordPresence.smallImageText = data.states[State_Idle].presence_small_image_text.c_str();
		discordPresence.state = data.states[State_Idle].state.c_str();
		configButtons[0] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Idle].buttons[0].label, data.buttons[0].label),
			ConfigData::LocalOrGlobal(data.states[State_Idle].buttons[0].url, data.buttons[0].url)
		);
		configButtons[1] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Idle].buttons[1].label, data.buttons[1].label),
			ConfigData::LocalOrGlobal(data.states[State_Idle].buttons[1].url, data.buttons[1].url)
		);

		if (configButtons[0].IsValid())
		{
			configButtons[0].FillStruct(buttons);
			// Nested condition because there will never be a single button in index 1.
			if (configButtons[1].IsValid())
			{
				configButtons[1].FillStruct(buttons+1);
			}
			else {
				buttons[1] = { "" , "" };
			}
			discordPresence.buttons = buttons;
		}
		Discord_UpdatePresence(&discordPresence);
		Discord_RunCallbacks();
		return;
	case CONNECTION_TYPE_PLAYBACK:
		discordPresence.details = data.states[State_Playback].details.c_str();
		discordPresence.largeImageKey = ConfigData::LocalOrGlobal(data.states[State_Playback].presence_large_image_key, data.discord_presence_large_image_key).c_str();
		discordPresence.largeImageText = data.states[State_Playback].presence_large_image_text.c_str();
		discordPresence.smallImageKey = ConfigData::LocalOrGlobal(data.states[State_Playback].presence_small_image_key, data.discord_presence_small_image_key).c_str();
		discordPresence.smallImageText = data.states[State_Playback].presence_small_image_text.c_str();
		discordPresence.state = data.states[State_Playback].state.c_str();
		configButtons[0] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Playback].buttons[0].label, data.buttons[0].label),
			ConfigData::LocalOrGlobal(data.states[State_Playback].buttons[0].url, data.buttons[0].url)
		);
		configButtons[1] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Playback].buttons[1].label, data.buttons[1].label),
			ConfigData::LocalOrGlobal(data.states[State_Playback].buttons[1].url, data.buttons[1].url)
		);

		if (configButtons[0].IsValid())
		{
			configButtons[0].FillStruct(buttons);
			// Nested condition because there will never be a single button in index 1.
			if (configButtons[1].IsValid())
			{
				configButtons[1].FillStruct(buttons+1);
			}
			else {
				buttons[1] = { "" , "" };
			}
			discordPresence.buttons = buttons;
		}
		Discord_UpdatePresence(&discordPresence);
		Discord_RunCallbacks();
		return;
	case CONNECTION_TYPE_SWEATBOX:
		if (data.sweatbox_bypass == true)
			break;
		discordPresence.details = data.states[State_Sweatbox].details.c_str();
		discordPresence.largeImageKey = ConfigData::LocalOrGlobal(data.states[State_Sweatbox].presence_large_image_key, data.discord_presence_large_image_key).c_str();
		discordPresence.largeImageText = data.states[State_Sweatbox].presence_large_image_text.c_str();
		discordPresence.smallImageKey = ConfigData::LocalOrGlobal(data.states[State_Sweatbox].presence_small_image_key, data.discord_presence_small_image_key).c_str();
		discordPresence.smallImageText = data.states[State_Sweatbox].presence_small_image_text.c_str();
		discordPresence.state = data.states[State_Sweatbox].state.c_str();
		configButtons[0] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Sweatbox].buttons[0].label, data.buttons[0].label),
			ConfigData::LocalOrGlobal(data.states[State_Sweatbox].buttons[0].url, data.buttons[0].url)
		);
		configButtons[1] = Button(
			ConfigData::LocalOrGlobal(data.states[State_Sweatbox].buttons[1].label, data.buttons[1].label),
			ConfigData::LocalOrGlobal(data.states[State_Sweatbox].buttons[1].url, data.buttons[1].url)
		);

		if (configButtons[0].IsValid())
		{
			configButtons[0].FillStruct(buttons);
			// Nested condition because there will never be a single button in index 1.
			if (configButtons[1].IsValid())
			{
				configButtons[1].FillStruct(buttons+1);
			}
			else {
				buttons[1] = { "" , "" };
			}
			discordPresence.buttons = buttons;
		}
		Discord_UpdatePresence(&discordPresence);
		Discord_RunCallbacks();
		return;
	case CONNECTION_TYPE_DIRECT:
		break;
	default:
		Discord_ClearPresence();
		Discord_RunCallbacks();
		return;
	}
	EuroScopePlugIn::CController controller = pMyPlugIn->ControllerMyself();
	const char* callsign = controller.GetCallsign();
	double frequency = controller.GetPrimaryFrequency();

	int use_state = -1;

	if (data.states[State_Direct_SUP].used && controller.GetRating() >= 11)
	{
		// Supervisor format
		use_state = State_Direct_SUP;
	}
	else if (data.states[State_Direct_OBS].used && !controller.IsController())
	{
		// Observer format
		use_state = State_Direct_OBS;
	}
	else if (data.states[State_Direct_CON].used)
	{
		// Controller format
		use_state = State_Direct_CON;
	}
	else
	{
		// Global format
		use_state = State_Direct;
	}
	
	std::string presence_small_image_key, presence_large_image_key, state, details, presence_small_image_text, presence_large_image_text;

	std::string freq_str(8, 0);
	std::snprintf((char*)freq_str.c_str(), 8, "%.3f", frequency);
	freq_str = std::string(freq_str, 0, strlen(freq_str.c_str()));
	std::string radio_callsign;
	pMyPlugIn->config.FindRadioCallsign(callsign, freq_str, radio_callsign);
	const std::map<std::string, std::string> Dictionary =
	{
		{"callsign", callsign},
		{"rcallsign", radio_callsign},
		{"frequency", freq_str},
		{"current_tracked", std::to_string(pMyPlugIn->CountTrackedAC())},
		{"total_inrange", std::to_string(pMyPlugIn->CountACinRange())},
		{"total_tracked", std::to_string(pMyPlugIn->tracklist.size())}
	};

	presence_small_image_key = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].presence_small_image_key, data.states[State_Direct].presence_small_image_key), data.discord_presence_small_image_key);
	presence_large_image_key = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].presence_large_image_key, data.states[State_Direct].presence_large_image_key), data.discord_presence_large_image_key);
	state = ConfigData::LocalOrGlobal(data.states[use_state].state, data.states[State_Direct].state);
	details = ConfigData::LocalOrGlobal(data.states[use_state].details, data.states[State_Direct].details);
	presence_small_image_text = ConfigData::LocalOrGlobal(data.states[use_state].presence_small_image_text, data.states[State_Direct].presence_small_image_text);
	presence_large_image_text = ConfigData::LocalOrGlobal(data.states[use_state].presence_large_image_text, data.states[State_Direct].presence_large_image_text);

	std::string button1_label = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].buttons[0].label, data.states[State_Direct].buttons[0].label), data.buttons[0].label);
	MessageFormatter::formatmap(button1_label, Dictionary);
	std::string button1_url = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].buttons[0].url, data.states[State_Direct].buttons[0].url), data.buttons[0].url);
	MessageFormatter::formatmap(button1_url, Dictionary);
	configButtons[0] = Button(
		button1_label,
		button1_url
	);			
	std::string button2_label = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].buttons[1].label, data.states[State_Direct].buttons[1].label), data.buttons[1].label);
	MessageFormatter::formatmap(button2_label, Dictionary);
	std::string button2_url = ConfigData::LocalOrGlobal(ConfigData::LocalOrGlobal(data.states[use_state].buttons[1].url, data.states[State_Direct].buttons[1].url), data.buttons[1].url);
	MessageFormatter::formatmap(button2_url, Dictionary);
	configButtons[1] = Button(
		button2_label.c_str(),
		button2_url.c_str()
	);

	if (configButtons[0].IsValid())
	{
		configButtons[0].FillStruct(buttons);
		// Nested condition because there will never be a single button in index 1.
		if (configButtons[1].IsValid())
		{
			configButtons[1].FillStruct(buttons+1);
		}
		else {
			buttons[1] = { "" , "" };
		}
		discordPresence.buttons = buttons;
	}

	MessageFormatter::formatmap(state, Dictionary);
	MessageFormatter::formatmap(details, Dictionary);
	MessageFormatter::formatmap(presence_small_image_text, Dictionary);
	MessageFormatter::formatmap(presence_large_image_text, Dictionary);

	discordPresence.smallImageKey = presence_small_image_key.c_str();
	discordPresence.largeImageKey = presence_large_image_key.c_str();
	discordPresence.state = state.c_str();
	discordPresence.details = details.c_str();
	discordPresence.smallImageText = presence_small_image_text.c_str();
	discordPresence.largeImageText = presence_large_image_text.c_str();

	discordPresence.startTimestamp = pMyPlugIn->EuroInittime;
	Discord_UpdatePresence(&discordPresence);
	Discord_RunCallbacks();
}


int VATPHILExt::CountACinRange()
{
	int count = 0;
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();
	while (ac.IsValid())
	{
		count++;
		ac = RadarTargetSelectNext(ac);
	}
	return count;
}

int VATPHILExt::CountTrackedAC()
{
	int count = 0;
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();
	while (ac.IsValid())
	{
		if (ac.GetCorrelatedFlightPlan().GetTrackingControllerIsMe() == true)
		{
			count++;
			if (std::find(pMyPlugIn->tracklist.begin(), pMyPlugIn->tracklist.end(), ac.GetCallsign()) == pMyPlugIn->tracklist.end())
				pMyPlugIn->tracklist.push_back(ac.GetCallsign());
		}
		ac = RadarTargetSelectNext(ac);
	}
	return count;
}

bool VATPHILExt::OnCompileCommand(const char* sCommandLine)
{
	if (_stricmp(sCommandLine, ".vatphil hotreload") == 0 || strcmp(sCommandLine, ".discord hotreload") == 0)
	{
		DisplayUserMessage("Message", PLUGIN_NAME, "Resetting...", true, true, false, true, false);
		config.Cleanup();
		DisplayUserMessage("Message", PLUGIN_NAME, "Reloading...", true, true, false, true, false);
		try {
			config.Init();
		}
		catch (std::exception& e)
		{
			std::string error_msg = "Error: ";
			error_msg += e.what();
			DisplayUserMessage("Message", PLUGIN_NAME, error_msg.c_str(), true, true, false, true, false);
			return true;
		}

		CheckWineBridge(true);
		this->EuroInittime = (int)time(NULL);
		config.LoadRadioCallsigns();

		char dmsg[40] = { 0 };
		int count = config.Data().RadioCallsigns.size();
		sprintf_s(dmsg, 40, "Successfully parsed %i callsign%s", count, count == 1 ? "!" : "s!");
		DisplayUserMessage("Message", PLUGIN_NAME, dmsg, true, true, false, true, false);
		return true;
	}
	if (_stricmp(sCommandLine, ".vatphil webhooktest") == 0 || strcmp(sCommandLine, ".discord webhooktest") == 0)
	{
		if (!config.isReady() || !HasBakedWebhookUrl())
		{
			DisplayUserMessage("Message", PLUGIN_NAME, "No webhook URL is baked into this build.", true, true, false, true, false);
			return true;
		}
		TestWebhook();
		return true;
	}
	return false;
}

std::string VATPHILExt::WebhookUsername()
{
	if (GetConnectionType() != EuroScopePlugIn::CONNECTION_TYPE_NO)
	{
		EuroScopePlugIn::CController controller = ControllerMyself();
		const char* callsign = controller.GetCallsign();
		if (callsign != nullptr && callsign[0] != '\0')
			return WebhookSender::AnsiToUtf8(callsign);
	}
	return "EuroScope";
}

std::string VATPHILExt::FormatChatLine(const std::string& format, const std::string& sender, const std::string& message, const std::string& time)
{
	const std::string effective_format = format.empty() ? std::string(WEBHOOK_DEFAULT_LINE_FORMAT) : format;
	std::string line;
	line.reserve(effective_format.size() + sender.size() + message.size());
	for (size_t i = 0; i < effective_format.size();)
	{
		if (effective_format.compare(i, 8, "{sender}") == 0)
		{
			line += sender;
			i += 8;
		}
		else if (effective_format.compare(i, 9, "{message}") == 0)
		{
			line += message;
			i += 9;
		}
		else if (effective_format.compare(i, 6, "{time}") == 0)
		{
			line += time;
			i += 6;
		}
		else
		{
			line += effective_format[i];
			i++;
		}
	}
	return line;
}

void VATPHILExt::SendToWebhook(const std::string& sender, const std::string& message)
{
	if (!config.isReady() || message.empty() || !HasBakedWebhookUrl())
		return;
	const VATPHIL_Configuration::ConfigData data = config.Data();

	const std::string line = FormatChatLine(
		data.webhook.line_format,
		WebhookSender::AnsiToUtf8(sender),
		WebhookSender::AnsiToUtf8(message),
		CurrentUtcTime()
	);
	webhook.Enqueue(BakedWebhookUrl(), WebhookUsername(), line);
}

bool VATPHILExt::TestWebhook()
{
	if (!config.isReady() || !HasBakedWebhookUrl())
		return false;
	const VATPHIL_Configuration::ConfigData data = config.Data();

	const std::string line = FormatChatLine(
		data.webhook.line_format,
		"EuroScope",
		"Test message from the VATPHIL plugin",
		CurrentUtcTime()
	);
	const int status = WebhookSender::TestPost(BakedWebhookUrl(), WebhookUsername(), line);
	if (status >= 200 && status < 300)
	{
		DisplayUserMessage("Message", PLUGIN_NAME, "Test message posted to the webhook.", true, true, false, true, false);
		return true;
	}

	std::string error = "Webhook test failed";
	if (status == 0)
		error += " (network or URL error; check the connection and the webhook URL).";
	else if (status == 401 || status == 403)
		error += " (webhook not authorised; check the URL).";
	else if (status == 404)
		error += " (webhook not found; check the URL).";
	else if (status == 429)
		error += " (Discord rate limit; try again shortly).";
	else
		error += " (HTTP " + std::to_string(status) + ").";
	DisplayUserMessage("Message", PLUGIN_NAME, error.c_str(), true, true, false, true, false);
	return false;
}

void VATPHILExt::OnCompilePrivateChat(const char* sSenderCallsign, const char* sReceiverCallsign, const char* sChatMessage)
{
	// Only the ATC channel is forwarded. Frequency chat, private messages,
	// broadcasts and server messages must never reach the webhook.
	if (!config.isReady() || sSenderCallsign == nullptr || sReceiverCallsign == nullptr || sChatMessage == nullptr)
		return;
	if (_stricmp(sReceiverCallsign, "ATC") != 0)
		return;
	if (sSenderCallsign[0] == '\0' || sChatMessage[0] == '\0' || _stricmp(sSenderCallsign, "server") == 0)
		return;
	SendToWebhook(sSenderCallsign, sChatMessage);
}

// ---- VATPHIL portal check ----
// As soon as EuroScope loads the plugin, before connecting, ask the portal whether the sector package has
// the current AIRAC's .sct. If not, a black screen covers EuroScope, so Connect can't be reached, until
// Try again finds it (run the Updater first) or they press Close EuroScope.
// When someone connects to a position on the live network, ask again with the callsign: activity,
// endorsements and rating too. The same screen stays up until Try again passes, they disconnect, or
// they press Close EuroScope.
// The portal being unreachable never blocks anyone who wasn't already blocked.

static const int FIRST_CHECK_DELAY_S = 5;    // let EuroScope settle after connecting
static const int WAITING_RETRY_S = 20;       // the VATSIM feed lags about a minute behind a connect
static const int WAITING_MAX_TRIES = 15;     // ~5 minutes
static const int FAILED_RETRY_S = 60;
static const int FAILED_MAX_TRIES = 5;
static const int SECTOR_RECHECK_S = 30 * 60; // a new AIRAC can start while EuroScope is open
static const char* const HEADING_BEFORE_CONNECT = "You can't connect to VATSIM";

void VATPHILExt::StartAccessCheck()
{
	if (portal.Busy())
		return;
	nextCheckAt = 0;
	checkedFor = checkCallsign;
	if (portal.Start(PORTAL_CHECK_URL, checkCallsign, PortalCheck::FindSectorFiles()))
		popup.SetChecking(true);
}

void VATPHILExt::Block(const std::string& heading, const std::vector<std::string>& lines)
{
	blockedHeading = heading;
	blockedLines = lines;
	if (reshowAt == 0)
		popup.Show(blockedHeading, blockedLines);
}

void VATPHILExt::TickAccessCheck()
{
	using namespace EuroScopePlugIn;
	const time_t now = time(NULL);

	// Which position are we on? Only live network connections are checked (not sweatbox or playback).
	std::string callsign;
	const int connection = GetConnectionType();
	if (connection == CONNECTION_TYPE_DIRECT || connection == CONNECTION_TYPE_VIA_PROXY)
	{
		CController me = ControllerMyself();
		if (me.IsValid() && me.IsController() && me.GetCallsign() != nullptr)
			callsign = me.GetCallsign();
	}
	if (callsign != checkCallsign)
	{
		// Connected, disconnected or changed position: start over. Back to "" (disconnected): the
		// sector file is checked again, as before connecting.
		checkCallsign = callsign;
		popup.Close();
		retries = 0;
		reshowAt = 0;
		blockedLines.clear();
		nextCheckAt = now + (callsign.empty() ? 3 : FIRST_CHECK_DELAY_S);
	}

	PortalCheck::Result result;
	if (portal.Poll(result) && checkedFor == checkCallsign)
	{
		popup.SetChecking(false);
		const bool beforeConnect = checkCallsign.empty();
		switch (result.status)
		{
		case PortalCheck::Status::Ok:
		case PortalCheck::Status::Waiting:
			if (beforeConnect || result.status == PortalCheck::Status::Ok)
			{
				popup.Close();
				reshowAt = 0;
				blockedLines.clear();
				retries = 0;
				if (beforeConnect)
					nextCheckAt = now + SECTOR_RECHECK_S;
				break;
			}
			// Connected but the portal can't see who we are yet: nothing wrong found so far, ask again shortly
			popup.Close();
			blockedLines.clear();
			if (++retries < WAITING_MAX_TRIES)
				nextCheckAt = now + WAITING_RETRY_S;
			break;
		case PortalCheck::Status::Blocked:
			Block(beforeConnect ? std::string(HEADING_BEFORE_CONNECT) : "You can't control " + WebhookSender::AnsiToUtf8(checkCallsign), result.messages);
			break;
		case PortalCheck::Status::Failed:
			// Keep a screen that's already up; otherwise try again later and let them carry on meanwhile
			if (!popup.IsOpen() && reshowAt == 0)
				nextCheckAt = now + (++retries < FAILED_MAX_TRIES ? FAILED_RETRY_S : SECTOR_RECHECK_S);
			break;
		}
	}

	if (reshowAt != 0 && now >= reshowAt)
	{
		// Close EuroScope was pressed but EuroScope is still here
		reshowAt = 0;
		if (!blockedLines.empty())
			popup.Show(blockedHeading, blockedLines);
	}
	if (nextCheckAt != 0 && now >= nextCheckAt)
		StartAccessCheck();
	popup.KeepInFront();
}
