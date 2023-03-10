/*
	Copyright(C) 2023 Kirollos Nashaat

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


DiscordEuroscopeExt::DiscordEuroscopeExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, "Discord Euroscope", "1.2.0", "Kirollos Nashaat", "GNU GPLv3")
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	// handlers
	Discord_Initialize(DISCORD_APPID, &handlers, 1, NULL);
	this->EuroInittime = (int)time(NULL);

	char DllPathFile[_MAX_PATH];
	std::string RCPath;
	GetModuleFileNameA(HINSTANCE(&__ImageBase), DllPathFile, sizeof(DllPathFile));
	RCPath = DllPathFile;
	RCPath.resize(RCPath.size() - strlen("DiscordEuroscope.dll"));
	RCPath += "DiscordEuroscope_RadioCallsigns.txt";

	FILE* tempfile = NULL;
	errno_t tempfile_err = fopen_s(&tempfile, RCPath.c_str(), "a");
	if (tempfile != NULL && tempfile_err == 0)
		fclose(tempfile);
	else
	{
		DisplayUserMessage("Message", "DiscordEuroscope", "Warning: Unable to access/create DiscordEuroscope_RadioCallsigns.txt", true, true, false, true, false);
	}

	std::ifstream fs;
	fs.open(RCPath.c_str(), std::ifstream::in);

	while (fs.good())
	{
		char* line = new char[50];
		fs.getline(line, 50);
		if (line != 0)
		{
			// I want to split in C instead
			if (std::string(line).find(' ') == std::string::npos)
			{delete[] line; continue;}
			std::string callsign;
			std::string radioname;
			int i = 0;
			while (*(line + i) != ' ')
			{
				callsign += (*(line + i));
				i++;
			}
			i++; // space before radioname
			while (*(line + i) != 0)
			{
				radioname += (*(line + i));
				i++;
			}
			if (callsign.length() > 0 && radioname.length() > 0)
			{
				RadioCallsigns.insert(std::pair<std::string, std::string>(callsign, radioname));
			}
		}
		delete[] line;
	}
	if(fs.is_open())
		fs.close();
#ifdef EUROSCOPE32
	char* dmsg = new char[100];
	int count = RadioCallsigns.size();
	sprintf_s(dmsg, 100, "Successfully parsed %i callsign%s", count, count == 1 ? "!" : "s!");
	DisplayUserMessage("Message", "DiscordEuroscope", dmsg, true, true, false, true, false);
	if (count == 0)
	{
		DisplayUserMessage("Message", "DiscordEuroscope", "If you haven't configured this properly, make sure you are writing to", true, true, false, true, false);
		DisplayUserMessage("Message", "DiscordEuroscope", "DiscordEuroscope_RadioCallsigns.txt, Each line holds a callsign", true, true, false, true, false);
		DisplayUserMessage("Message", "DiscordEuroscope", "Example: HECC_CTR Cairo Control", true, true, false, true, false);
	}
	delete[] dmsg;
#endif
}


DiscordEuroscopeExt::~DiscordEuroscopeExt()
{
	RadioCallsigns.clear();
}

VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	if (inst == nullptr)
		return;
	if (inst->EuroInittime == 0)
		return;
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.largeImageKey = PRESENCE_LARGE_IMAGE_KEY;
	discordPresence.startTimestamp = inst->EuroInittime;
	switch (pMyPlugIn->GetConnectionType())
	{
		using namespace EuroScopePlugIn;
	case CONNECTION_TYPE_NO:
		discordPresence.details = "Idle";
		Discord_UpdatePresence(&discordPresence);
		return;
	case CONNECTION_TYPE_PLAYBACK:
		discordPresence.details = "Playback";
		
		Discord_UpdatePresence(&discordPresence);
		return;
	case CONNECTION_TYPE_SWEATBOX:
#if SWEATBOX_BYPASS == FALSE
		discordPresence.details = "Sweatbox";
		Discord_UpdatePresence(&discordPresence);
		return;
#else
		break;
#endif
	case CONNECTION_TYPE_DIRECT:
		break;
	default:
		Discord_ClearPresence();
		return;
	}
	EuroScopePlugIn::CController controller = pMyPlugIn->ControllerMyself();
	const char* callsign = controller.GetCallsign();
	double frequency = controller.GetPrimaryFrequency();
	char* tmp = new char[100];
	char* tmp2 = new char[100];
	char* tmp3 = new char[100];
	if (controller.IsController())
	{
		if (inst->RadioCallsigns.find(callsign) != inst->RadioCallsigns.end())
			discordPresence.largeImageText =
											#if RADIO_CALLSIGN_MAIN == FALSE
														inst->RadioCallsigns[callsign].c_str();
											#else
														inst->RadioCallsigns[callsign].length() ? callsign : "";
											#endif
		sprintf_s(tmp, 100, "%s %.3fMHz", 
											#if RADIO_CALLSIGN_MAIN == TRUE
														inst->RadioCallsigns[callsign].length() ? inst->RadioCallsigns[callsign].c_str() : callsign
											#else
													callsign
											#endif
			, frequency);
		sprintf_s(tmp2, 100, "Aircraft tracked (%i of %i)", inst->CountTrackedAC(), inst->CountACinRange());
		if (inst->tracklist.size() > 0) {
			sprintf_s(tmp3, 100, "Total tracks: %i", inst->tracklist.size());
			discordPresence.smallImageText = tmp3;
			discordPresence.smallImageKey = PRESENCE_SMALL_IMAGE_KEY;
		}
	}
	else
	{
		sprintf_s(tmp, 100, "Observing as %s", callsign);
		sprintf_s(tmp2, 100, "Aircraft in range: %i", inst->CountACinRange());
	}
	discordPresence.details = tmp;
	discordPresence.state = tmp2;
	discordPresence.startTimestamp = inst->EuroInittime;
	Discord_UpdatePresence(&discordPresence);
	Discord_RunCallbacks();
	delete[] tmp;
	delete[] tmp2;
	delete[] tmp3;
}


int DiscordEuroscopeExt::CountACinRange()
{
	int count = 0;
#ifdef EUROSCOPE31D
	EuroScopePlugIn::CAircraft ac = AircraftSelectFirst();
#else
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();
#endif
	while (ac.IsValid())
	{
		count++;
#ifdef EUROSCOPE31D
		ac = AircraftSelectNext(ac);
#else
		ac = RadarTargetSelectNext(ac);
#endif
	}
	return count;
}

int DiscordEuroscopeExt::CountTrackedAC()
{
	int count = 0;
#ifdef EUROSCOPE31D
	EuroScopePlugIn::CAircraft ac = AircraftSelectFirst();
#else
	EuroScopePlugIn::CRadarTarget ac = RadarTargetSelectFirst();
#endif
	while (ac.IsValid())
	{
#ifdef EUROSCOPE31D
		if(ac.GetTrackingControllerIsMe() == true)
#else
		if (ac.GetCorrelatedFlightPlan().GetTrackingControllerIsMe() == true)
#endif
		{
			count++;
			if (std::find(inst->tracklist.begin(), inst->tracklist.end(), ac.GetCallsign()) == inst->tracklist.end())
				inst->tracklist.push_back(ac.GetCallsign());
		}
#ifdef EUROSCOPE31D
		ac = AircraftSelectNext(ac);
#else
		ac = RadarTargetSelectNext(ac);
#endif
	}
	return count;
}