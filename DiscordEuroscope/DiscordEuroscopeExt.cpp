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
#include "DiscordEuroscopeExt.h"
#include "ConfigManager.h"
#include "MessageFormatter.h"

DiscordEuroscopeExt::DiscordEuroscopeExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, "Discord Euroscope", "1.3.0", "Kirollos Nashaat", "GNU GPLv3")
{
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
			// File cannot be opened, try to generate
			std::string error_msg = "Error: ";
			error_msg += e.what();
			DisplayUserMessage("Message", "DiscordEuroscope", error_msg.c_str(), true, true, false, true, false);
			DisplayUserMessage("Message", "DiscordEuroscope", "Attempting to generate " CONFIG_FILENAME, true, true, false, true, false);
			config.GenerateConfigFile(RCPath);
			try {
				config.Init(RCPath);
			}
			catch (std::exception& e2)
			{
				std::string error_msg = "Error: ";
				error_msg += e2.what();
				error_msg += ".. Halting..";
				DisplayUserMessage("Message", "DiscordEuroscope", error_msg.c_str(), true, true, false, true, false);
				return;
			}
		}
		else
		{
			std::string error_msg = "Error: ";
			error_msg += e.what();
			DisplayUserMessage("Message", "DiscordEuroscope", error_msg.c_str(), true, true, false, true, false);
			return;
		}
	}
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	// handlers
	Discord_Initialize(static_cast<const char*>(config.Data().discord_appid.c_str()), &handlers, 1, NULL);
	this->EuroInittime = (int)time(NULL);
	config.LoadRadioCallsigns();

	char dmsg[40] = { 0 };
	int count = config.Data().RadioCallsigns.size();
	sprintf_s(dmsg, 40, "Successfully parsed %i callsign%s", count, count == 1 ? "!" : "s!");
	DisplayUserMessage("Message", "DiscordEuroscope", dmsg, true, true, false, true, false);
}


DiscordEuroscopeExt::~DiscordEuroscopeExt()
{
	config.Cleanup();
}

VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	using namespace DiscordEuroScope_Configuration;
	if (pMyPlugIn == nullptr)
		return;
	if (pMyPlugIn->EuroInittime == 0)
		return;
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


int DiscordEuroscopeExt::CountACinRange()
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

int DiscordEuroscopeExt::CountTrackedAC()
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

bool DiscordEuroscopeExt::OnCompileCommand(const char* sCommandLine)
{
	if (strcmp(sCommandLine, ".discord hotreload") == 0)
	{
		DisplayUserMessage("Message", "DiscordEuroscope", "Resetting...", true, true, false, true, false);
		config.Cleanup();
		DisplayUserMessage("Message", "DiscordEuroscope", "Reloading...", true, true, false, true, false);
		try {
			config.Init();
		}
		catch (std::exception& e)
		{
			std::string error_msg = "Error: ";
			error_msg += e.what();
			DisplayUserMessage("Message", "DiscordEuroscope", error_msg.c_str(), true, true, false, true, false);
			return true;
		}

		this->EuroInittime = (int)time(NULL);
		config.LoadRadioCallsigns();

		char dmsg[40] = { 0 };
		int count = config.Data().RadioCallsigns.size();
		sprintf_s(dmsg, 40, "Successfully parsed %i callsign%s", count, count == 1 ? "!" : "s!");
		DisplayUserMessage("Message", "DiscordEuroscope", dmsg, true, true, false, true, false);
		return true;
	}
	return false;
}