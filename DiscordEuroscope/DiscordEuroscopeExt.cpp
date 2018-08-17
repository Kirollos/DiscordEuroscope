#include "stdafx.h"
#include "DiscordEuroscopeExt.h"


DiscordEuroscopeExt::DiscordEuroscopeExt() : EuroScopePlugIn::CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, "Discord Euroscope", "1.0.0", "Kirollos Nashaat", "https://github.com/Kirollos/DiscordEuroscope")
{
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	// handlers
	Discord_Initialize("477907858072272896", &handlers, 1, NULL);
	this->EuroInittime = (int)time(NULL);
}


DiscordEuroscopeExt::~DiscordEuroscopeExt()
{
}

VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	if (inst == nullptr)
		return;
	if (inst->EuroInittime == 0)
		return;
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.largeImageKey = "es";
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
		discordPresence.details = "Sweatbox";
		Discord_UpdatePresence(&discordPresence);
		return;
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
	if (controller.IsController())
	{
		sprintf(tmp, "%s %.2fMHz", callsign, frequency);
		sprintf(tmp2, "Aircraft tracked (%i of %i)", inst->CountTrackedAC(), inst->ACir.size());
	}
	else
	{
		sprintf(tmp, "Observing as %s", callsign);
		sprintf(tmp2, "Aircraft in range: %i", inst->ACir.size());
	}
	discordPresence.details = tmp;
	discordPresence.state = tmp2;
	discordPresence.startTimestamp = inst->EuroInittime;
	Discord_UpdatePresence(&discordPresence);
	Discord_RunCallbacks();
	delete tmp;
	delete tmp2;
}

#ifdef EUROSCOPE31D
void DiscordEuroscopeExt::OnAircraftPositionUpdate(EuroScopePlugIn::CAircraft Aircraft)
#else
void DiscordEuroscopeExt::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget Aircraft)
#endif
{
	bool found = false;
	const char* callsign = Aircraft.GetCallsign();
	for(std::list<const char*>::iterator it = this->ACir.begin(); it != this->ACir.end(); it ++)
	{
		if(strcmp(*it, callsign) == 0)
		{
			found = true;
			break;
		}
	}
	if(!found)
	{
		//if(Aircraft.GetSectorEntryMinutes() == 0)
		this->ACir.push_back(callsign);
	}
}

#ifdef EUROSCOPE31D
void DiscordEuroscopeExt::OnAircraftDisconnect(EuroScopePlugIn::CAircraft Aircraft)
#else
void DiscordEuroscopeExt::OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan Aircraft)
#endif
{
	bool found = false;
	const char* callsign = Aircraft.GetCallsign();
	for(std::list<const char*>::iterator it = this->ACir.begin(); it != this->ACir.end(); it ++)
	{
		if(strcmp(*it, callsign) == 0)
		{
			found = true;
			break;
		}
	}
	if(found)
	{
		this->ACir.remove(callsign);
	}
}

int DiscordEuroscopeExt::CountTrackedAC()
{
	int count = 0;
	for(std::list<const char*>::iterator it = this->ACir.begin(); it != this->ACir.end(); it ++)
	{
#ifdef EUROSCOPE31D
		if(this->AircraftSelect(*it).GetTrackingControllerIsMe() == true)
#else
		if (this->RadarTargetSelect(*it).GetCorrelatedFlightPlan().GetTrackingControllerIsMe() == true)
#endif
			count++;
	}
	return count;
}