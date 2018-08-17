// DiscordEuroscope.h : main header file for the DiscordEuroscope DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CDiscordEuroscopeApp
// See DiscordEuroscope.cpp for the implementation of this class
//

class CDiscordEuroscopeApp : public CWinApp
{
public:
	CDiscordEuroscopeApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

