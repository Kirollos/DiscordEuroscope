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
// DiscordEuroscope.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "DiscordEuroscope.h"
#include "DiscordEuroscopeExt.h"
#include "config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef EUROSCOPE31D
#include "inc\\3.1d\\EuroScopePlugIn.h"
#else
#include "inc\\3.2\\EuroScopePlugIn.h"
#endif
#include "inc\\discord_rpc.h"

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

EuroScopePlugIn::CPlugIn *pMyPlugIn = nullptr;
DiscordEuroscopeExt* inst = nullptr;
UINT_PTR DISCORDTIMERID = 0;

void __declspec (dllexport)
EuroScopePlugInInit(EuroScopePlugIn::CPlugIn ** ppPlugInInstance)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	// allocate
	inst = new DiscordEuroscopeExt;
	*ppPlugInInstance = pMyPlugIn =	inst;
}

void __declspec (dllexport)
EuroScopePlugInExit(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	
	inst->EuroInittime = 0;
	Discord_Shutdown();
	KillTimer(0, DISCORDTIMERID);
	delete pMyPlugIn;
}


// CDiscordEuroscopeApp

BEGIN_MESSAGE_MAP(CDiscordEuroscopeApp, CWinApp)
END_MESSAGE_MAP()


// CDiscordEuroscopeApp construction

CDiscordEuroscopeApp::CDiscordEuroscopeApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDiscordEuroscopeApp object

CDiscordEuroscopeApp theApp;


// CDiscordEuroscopeApp initialization

BOOL CDiscordEuroscopeApp::InitInstance()
{
	CWinApp::InitInstance();
	DISCORDTIMERID = SetTimer(0, 0, UPDATE_TIMER, DiscordTimer);
	return TRUE;
}
