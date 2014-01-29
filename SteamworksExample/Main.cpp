//====== Copyright © 1996-2008, Valve Corporation, All rights reserved. =======
//
// Purpose: Main file for the SteamworksExample app
//
//=============================================================================

#include "stdafx.h"

#ifdef WIN32
#include "gameenginewin32.h"
#elif _PS3
#include "gameengineps3.h"
#endif

#include "SpaceWarClient.h"


//-----------------------------------------------------------------------------
// Purpose: Wrapper around SteamAPI_WriteMiniDump which can be used directly 
// as a se translator
//-----------------------------------------------------------------------------
#ifndef _PS3
void MiniDumpFunction( unsigned int nExceptionCode, EXCEPTION_POINTERS *pException )
{
	// You can build and set an arbitrary comment to embed in the minidump here,
	// maybe you want to put what level the user was playing, how many players on the server,
	// how much memory is free, etc...
	SteamAPI_SetMiniDumpComment( "Minidump comment: SteamworksExample.exe\n" );

	// The 0 here is a build ID, we don't set it
	SteamAPI_WriteMiniDump( nExceptionCode, pException, 0 );
}
#endif


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
		x = x;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Main loop code shared between all platforms
//-----------------------------------------------------------------------------
void RunGameLoop( IGameEngine *pGameEngine, char *pchServerAddress, char *pchLobbyID  )
{
	// Make sure it initialized ok
	if ( pGameEngine->BReadyForUse() )
	{
		// Initialize the game
		CSpaceWarClient *pGameClient = new CSpaceWarClient( pGameEngine, SteamUser()->GetSteamID() );

		// Black background
		pGameEngine->SetBackgroundColor( 0, 0, 0, 0 );

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

		// if +connect_lobby was used to specify a lobby to join, connect now
		if ( pchLobbyID )
		{
#ifndef _PS3
			CSteamID steamIDLobby( (uint64)_atoi64( pchLobbyID ) );
#else
			CSteamID steamIDLobby( (uint64)atoll( pchLobbyID ) );
#endif
			if ( steamIDLobby.IsValid() )
			{
				// act just like we had selected it from the menu
				LobbyBrowserMenuItem_t menuItem = { steamIDLobby, k_EClientJoiningLobby };
				pGameClient->OnMenuSelection( menuItem );
			}
		}

		// test a user specific secret before entering main loop
#ifndef _PS3
		Steamworks_TestSecret();
#endif

		pGameClient->RetrieveEncryptedAppTicket();

		while( !pGameEngine->BShuttingDown() )
		{
			if ( pGameEngine->StartFrame() )
			{
				pGameEngine->UpdateGameTickCount();

				// Run a game frame
				pGameClient->RunFrame();
				pGameEngine->EndFrame();

				// Sleep to limit frame rate
				while( pGameEngine->BSleepForFrameRateLimit( MAX_CLIENT_AND_SERVER_FPS ) )
				{
					// Keep running the network on the client at a faster rate than the FPS limit
					pGameClient->ReceiveNetworkData();
				}
			}			
		}

		delete pGameClient;
	}

	// Cleanup the game engine
	delete pGameEngine;
}


//-----------------------------------------------------------------------------
// Purpose: Real main entry point for the program
//-----------------------------------------------------------------------------
#ifdef WIN32
int APIENTRY RealMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	
	if ( SteamAPI_RestartAppIfNecessary( k_uAppIdInvalid ) )
	{
		// if Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the 
		// local Steam client and also launches this game again.
		
		// Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and
		// removed steam_appid.txt from the game depot.

		return EXIT_FAILURE;
	}
	

	// Init Steam CEG
	if ( !Steamworks_InitCEGLibrary() )
	{
		OutputDebugString( "Steamworks_InitCEGLibrary() failed\n" );
		::MessageBox( NULL, "Steam must be running to play this game (InitDrmLibrary() failed).\n", "Fatal Error", MB_OK );
		return EXIT_FAILURE;
	}

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
		return EXIT_FAILURE;
	}

	// set our debug handler
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );

	// Tell Steam where it's overlay should show notification dialogs, this can be top right, top left,
	// bottom right, bottom left. The default position is the bottom left if you don't call this.  
	// Generally you should use the default and not call this as users will be most comfortable with 
	// the default position.  The API is provided in case the bottom right creates a serious conflict 
	// with important UI in your game.
	SteamUtils()->SetOverlayNotificationPosition( k_EPositionTopRight );

	// Look for the +connect ipaddress:port parameter in the command line,
	// Steam will pass this when a user has used the Steam Server browser to find
	// a server for our game and is trying to join it. 
	const char *pchConnectParam = "+connect";
	char *pchCmdLine = ::GetCommandLine();
	char *pchConnect = strstr( pchCmdLine, pchConnectParam );
	char *pchServerAddress = NULL;
	if ( pchConnect && strlen( pchCmdLine ) > (pchConnect - pchCmdLine) + strlen( pchConnectParam ) + 1 )
	{
		// Address should be right after the +connect, +1 on the end to skip the space
		pchServerAddress = pchCmdLine + ( pchConnect - pchCmdLine ) + strlen( pchConnectParam ) + 1;
	}

	// look for +connect_lobby lobbyid paramter on the command line
	// Steam will pass this in if a user taken up an invite to a lobby
	const char *pchConnectLobbyParam = "+connect_lobby";
	char *pchConnectLobby = strstr( pchCmdLine, pchConnectParam );
	char *pchLobbyID = NULL;
	if ( pchConnectLobby && strlen( pchCmdLine ) > (pchConnectLobby - pchCmdLine) + strlen( pchConnectLobbyParam ) + 1 )
	{
		// Address should be right after the +connect, +1 on the end to skip the space
		pchLobbyID = pchCmdLine + ( pchConnectLobby - pchCmdLine ) + strlen( pchConnectLobbyParam ) + 1;
	}

	// do a DRM self check
	Steamworks_SelfCheck();

	// Construct a new instance of the game engine 
	// bugbug jmccaskey - make screen resolution dynamic, maybe take it on command line?
	CGameEngineWin32 *pGameEngine = new CGameEngineWin32( hInstance, nCmdShow, 1024, 768 );

	// This call will block and run until the game exits
	RunGameLoop( pGameEngine, pchServerAddress, pchLobbyID );

	// Shutdown the SteamAPI
	SteamAPI_Shutdown();

	// Shutdown Steam CEG
	Steamworks_TermCEGLibrary();

	// exit
	return EXIT_SUCCESS;	
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Main entry point for the program -- win32
//-----------------------------------------------------------------------------
#ifdef WIN32
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
		// We don't want to mask exceptions (or report them to Steam!) when debugging.
		// If you would like to step through the exception handler, attach a debugger
		// after running the game outside of the debugger.
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
#endif


//-----------------------------------------------------------------------------
// Purpose: Main entry point for the program -- ps3
//-----------------------------------------------------------------------------
#ifdef _PS3


//-----------------------------------------------------------------------------
// Purpose: Loads the Steam PS3 module
//-----------------------------------------------------------------------------
sys_prx_id_t g_sys_prx_id_steam = -1;
static bool LoadSteamPS3Module()
{
	g_sys_prx_id_steam = sys_prx_load_module( SYS_APP_HOME "/steam_api_ps3.sprx", 0, NULL );
	if ( g_sys_prx_id_steam < CELL_OK )
	{
		OutputDebugString( "LoadSteamModule() - failed to load steam_api_ps3\n" );
		return false;
	}

	int modres;
	int res = sys_prx_start_module( g_sys_prx_id_steam, 0, NULL, &modres, 0, NULL);
	if ( res < CELL_OK )
	{
		OutputDebugString( "LoadSteamModule() - failed to start steam_api_ps3\n" );
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Unloads the Steam PS3 module
//-----------------------------------------------------------------------------
static bool UnloadSteamPS3Module()
{
	// check if loaded
	if ( g_sys_prx_id_steam < CELL_OK )
		return false;

	int modres;
	int res = sys_prx_stop_module( g_sys_prx_id_steam, 0, NULL, &modres, 0, NULL);
	if ( res < CELL_OK )
	{
		OutputDebugString( "LoadSteamModule() - failed to stop steam_api_ps3\n" );
		return false;
	}

	res = sys_prx_unload_module( g_sys_prx_id_steam, 0, NULL );
	if ( res < CELL_OK )
	{
		OutputDebugString( "LoadSteamModule() - failed to unload steam_api_ps3\n" );
		return false;
	}

	g_sys_prx_id_steam = -1;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initializes Steam PS3 params for our application
//-----------------------------------------------------------------------------
bool SetSteamPS3Params( SteamPS3Params_t *pParams )
{
	// Steam needs the system cache path
	CellSysCacheParam sysCacheParams;
	memset( &sysCacheParams, 0, sizeof( CellSysCacheParam ) );
	if ( cellSysCacheMount( &sysCacheParams ) < 0 )
		return false;

	// configure the Steamworks PS3 parameters. All params need to be set
	strncpy( pParams->m_rgchInstallationPath, SYS_APP_HOME, STEAM_PS3_PATH_MAX );
	strncpy( pParams->m_rgchSystemCache, sysCacheParams.getCachePath, STEAM_PS3_PATH_MAX );

	return true;
}

// Global for PS3 params
SteamPS3Params_t g_SteamPS3Params;


//-----------------------------------------------------------------------------
// Purpose: Main entry point for the program -- ps3
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	printf( "PS3 main\n" );

	// Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
	sys_spu_initialize(6, 1);	

	// Load Steam
	if ( !LoadSteamPS3Module() )
		return EXIT_FAILURE;

	// No restart app if necessary, or CEG initialization on PS3

	// Initialize SteamAPI, if this fails we bail out since we depend on Steam for lots of stuff.
	// You don't necessarily have to though if you write your code to check whether all the Steam
	// interfaces are NULL before using them and provide alternate paths when they are unavailable.

	if ( !SetSteamPS3Params( &g_SteamPS3Params ) )
	{
		OutputDebugString( "SetSteamPS3Params() failed\n" );
		return EXIT_FAILURE;
	}

	if ( !SteamAPI_Init( &g_SteamPS3Params ) )
	{
		OutputDebugString( "SteamAPI_Init() failed\n" );
		return EXIT_FAILURE;
	}

	// set our debug handler
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );

	// bugbug ps3
	SteamUser()->LogOn( "jmccaskeybeta", "test123" );

	// No overlay notification position setting on PS3

	// No +connect support on PS3 since Steam isn't launching us

	// Construct a new instance of the game engine 
	// bugbug jmccaskey - make screen resolution dynamic, maybe take it on command line?
	CGameEnginePS3 *pGameEngine = new CGameEnginePS3();

	// This call will block and run until the game exits
	RunGameLoop( pGameEngine, NULL, NULL );

	// Shutdown the SteamAPI
	SteamAPI_Shutdown();

	// Unload Steam
	UnloadSteamPS3Module();

	// exit
	return 0;	
}
#endif