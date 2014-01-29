//====== Copyright © 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: Main file for the SteamworksExample app
//
//=============================================================================

#include "stdafx.h"
#include "GameEngine.h"
#include "SpaceWarClient.h"
#include "steam/steam_api.h"


//-----------------------------------------------------------------------------
// Purpose: Wrapper around SteamAPI_WriteMiniDump which can be used directly 
// as a se translator
//-----------------------------------------------------------------------------
void MiniDumpFunction( unsigned int nExceptionCode, EXCEPTION_POINTERS *pException )
{
	// You can build and set an arbitrary comment to embed in the minidump here,
	// maybe you want to put what level the user was playing, how many players on the server,
	// how much memory is free, etc...
	SteamAPI_SetMiniDumpComment( "Minidump comment: SteamworksExample.exe\n" );

	// The 0 here is a build ID, we don't set it
	SteamAPI_WriteMiniDump( nExceptionCode, pException, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: callback hook for debug text emitted from the Steam API
//-----------------------------------------------------------------------------
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char *pchDebugText )
{
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	::OutputDebugString( pchDebugText );

	if ( nSeverity >= 1 )
	{
		// place to set a breakpoint for catching API errors
		int x = 3;
		x;
	}
}




//-----------------------------------------------------------------------------
// Purpose: Real main entry point for the program
//-----------------------------------------------------------------------------
int APIENTRY RealMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	// Initialize SteamAPI, if this fails we bail out since we depend on Steam for lots of stuff.
	// You don't necessarily have to though if you write your code to check whether all the Steam
	// interfaces are NULL before using them and provide alternate paths when they are unavailable.
	//
	// This will also load the in-game steam overlay dll into your process.  That dll is normally
	// injected by steam when it launches games, but by calling this you cause it to always load,
	// even when not launched via steam.
	if ( !SteamAPI_Init() )
	{
		OutputDebugString( "SteamAPI_Init() failed\n" );
		::MessageBox( NULL, "Steam must be running to play this game (SteamAPI_Init() failed).\n", "Fatal Error", MB_OK );
		return -1;
	}

	// set our debug handler
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );


	// Look for the +connect ipaddress:port parameter in the command line,
	// Steam will pass this when a user has used the Steam Server browser to find
	// a server for our game and is trying to join it. 
	const char* pchConnectParam = "+connect";
	char *pchCmdLine = ::GetCommandLine();
	char *pchConnect = strstr( pchCmdLine, pchConnectParam );
	char *pchServerAddress = NULL;
	if ( pchConnect && strlen( pchCmdLine ) > (pchConnect - pchCmdLine) + strlen( pchConnectParam ) + 1 )
	{
		// Address should be right after the +connect, +1 on the end to skip the space
		pchServerAddress = pchCmdLine + ( pchConnect - pchCmdLine ) + strlen( pchConnectParam ) + 1;
	}



	// Construct a new instance of the game engine 
	// bugbug jmccaskey - make screen resolution dynamic, maybe take it on command line?
	CGameEngine *pGameEngine = new CGameEngine( hInstance, nCmdShow, 1024, 768 );

	// Make sure it initialized ok
	if ( pGameEngine->BReadyForUse() )
	{
		// Initialize the game
		CSpaceWarClient *pGameClient = new CSpaceWarClient( pGameEngine, SteamUser()->GetSteamID() );

		// Black background
		pGameEngine->SetBackgroundColor( 0, 0, 0, 0 );

		// Set max FPS
		pGameEngine->SetMaxFPS( MAX_CLIENT_AND_SERVER_FPS );

		// If +connect was used to specify a server address, connect now
		if ( pchServerAddress )
		{

			int32 octet0 = 0, octet1 = 0, octet2 = 0, octet3 = 0;
			int32 uPort = 0;
			int nConverted = sscanf( pchServerAddress, "%d.%d.%d.%d:%d", &octet0, &octet1, &octet2, &octet3, &uPort );
			if ( nConverted == 5 )
			{
				char rgchIPAddress[128];
				_snprintf( rgchIPAddress, ARRAYSIZE( rgchIPAddress ), "%d.%d.%d.%d", octet0, octet1, octet2, octet3 );
				uint32 unIPAddress = ( octet3 ) + ( octet2 << 8 ) + ( octet1 << 16 ) + ( octet0 << 24 );
				pGameClient->InitiateServerConnection( unIPAddress, uPort );
			}
		}

		while( !pGameEngine->BShuttingDown() )
		{
			if ( pGameEngine->StartFrame() )
			{
				// Run a game frame
				pGameClient->RunFrame();

				pGameEngine->EndFrame();
			}			
		}

		delete pGameClient;
	}

	// Cleanup the game engine
	delete pGameEngine;

	// Shutdown the SteamAPI
	SteamAPI_Shutdown();

	// exit
	return 0;	
}


//-----------------------------------------------------------------------------
// Purpose: Main entry point for the program
//-----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR     lpCmdLine,
					 int       nCmdShow)
{
	// All we do here is call the real main function after setting up our se translator
	// this allows us to catch exceptions and report errors to Steam.
	//
	// Note that you must set your compiler flags correctly to enable structured exception 
	// handling in order for this particular setup method to work.

	if ( IsDebuggerPresent() )
	{
		// We don't want to mask exceptions (or report them to Steam!) when debugging
		return RealMain( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
	}

	_set_se_translator( MiniDumpFunction );
	try  // this try block allows the SE translator to work
	{
		return RealMain( hInstance, hPrevInstance, lpCmdLine, nCmdShow );
	}
	catch( ... )
	{
		return -1;
	}
}
