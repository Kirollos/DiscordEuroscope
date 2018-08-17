#pragma once
#ifdef EUROSCOPE31D
#include "inc\\3.1d\\EuroScopePlugIn.h"
#else
#include "inc\\3.2\\EuroScopePlugIn.h"
#endif
#include "inc\\discord_rpc.h"
#include <list>

#ifndef __E_KEK_H
#define __E_KEK_H

class DiscordEuroscopeExt : public EuroScopePlugIn::CPlugIn
{
public:
	DiscordEuroscopeExt();
	virtual ~DiscordEuroscopeExt();
	int EuroInittime;
	std::list<const char*> ACir;
	
#ifdef EUROSCOPE31D
	virtual void OnAircraftPositionUpdate (EuroScopePlugIn::CAircraft Aircraft);
	virtual void OnAircraftDisconnect (EuroScopePlugIn::CAircraft Aircraft);
#else
	virtual void OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget Aircraft);
	virtual void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan Aircraft);
#endif
	int CountTrackedAC();
};

extern EuroScopePlugIn::CPlugIn *pMyPlugIn;
extern DiscordEuroscopeExt* inst;

#define DISCORDTIMERID (0xb00b)
VOID CALLBACK DiscordTimer(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime);

#endif __E_KEK_H