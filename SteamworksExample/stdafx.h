//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 2k or later.
#define WINVER 0x0500		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows 2k or later.                   
#define _WIN32_WINNT 0x0500	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers



#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )
#pragma comment( lib, "dxguid.lib" )

// Windows Header Files:
#include <windows.h>
#include <tchar.h>

// Winsock
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib" )

// d3d header files
#include "d3d9.h"
#include "d3dx9.h"

typedef __int16 int16;
typedef unsigned __int16 uint16;
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;

// steam api header file
#include "steam/steam_api.h"

#ifdef STEAM_CEG
	// Steam DRM header file
	#include "cegclient.h"
#else
	#define Steamworks_InitCEGLibrary() (true)
	#define Steamworks_TermCEGLibrary() (true)
	#define Steamworks_TestSecret() (true)
	#define Steamworks_SelfCheck() (true)
#endif

