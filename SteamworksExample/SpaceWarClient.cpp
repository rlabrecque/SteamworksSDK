//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Main class for the space war game client
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "SpaceWarClient.h"
#include "SpaceWarServer.h"
#include "MainMenu.h"
#include "QuitMenu.h"
#include "stdlib.h"
#include "time.h"
#include "ServerBrowser.h"
#include "Leaderboards.h"
#include "Lobby.h"
#include "p2pauth.h"

CSpaceWarClient *g_pSpaceWarClient = NULL;
CSpaceWarClient* SpaceWarClient() { return g_pSpaceWarClient; }

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpaceWarClient::CSpaceWarClient( CGameEngine *pGameEngine, CSteamID steamIDUser ) :
		m_CallbackP2PSessionConnectFail( this, &CSpaceWarClient::OnP2PSessionConnectFail ),
		m_LobbyGameCreated( this, &CSpaceWarClient::OnLobbyGameCreated ),
		m_IPCFailureCallback( this, &CSpaceWarClient::OnIPCFailure ),
		m_SteamShutdownCallback( this, &CSpaceWarClient::OnSteamShutdown )

{
	g_pSpaceWarClient = this;
	m_pGameEngine = pGameEngine;
	m_eGameState = k_EClientGameMenu;
	m_uPlayerWhoWonGame = 0;
	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_ulLastNetworkDataReceivedTime = 0;
	m_pServer = NULL;
	m_uPlayerShipIndex = 0;
	m_SteamIDLocalUser = steamIDUser;
	m_eConnectedStatus = k_EClientNotConnected;
	m_bTransitionedGameState = false;
	m_rgchErrorText[0] = 0;
	m_unServerIP = 0;
	m_usServerPort = 0;
	m_ulPingSentTime = 0;

	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		m_rguPlayerScores[i] = 0;
		m_rgpShips[i] = NULL;
	}

	// Seed random num generator
	srand( (uint32)time( NULL ) );

	m_hHUDFont = pGameEngine->HCreateFont( HUD_FONT_HEIGHT, FW_BOLD, false, "Arial" );
	if ( !m_hHUDFont )
		OutputDebugString( "HUD font was not created properly, text won't draw\n" );

	m_hInstructionsFont = pGameEngine->HCreateFont( INSTRUCTIONS_FONT_HEIGHT, FW_BOLD, false, "Arial" );
	if ( !m_hHUDFont )
		OutputDebugString( "HUD font was not created properly, text won't draw\n" );

	// Initialize starfield
	m_pStarField = new CStarField( pGameEngine );

	// Initialize main menu
	m_pMainMenu = new CMainMenu( pGameEngine );

	// Initialize pause menu
	m_pQuitMenu = new CQuitMenu( pGameEngine );

	// Initialize sun
	m_pSun = new CSun( pGameEngine );

	// initialize P2P auth engine
	m_pP2PAuthedGame = new CP2PAuthedGame( m_pGameEngine );

	// Create matchmaking menus
	m_pServerBrowser = new CServerBrowser( m_pGameEngine );
	m_pLobbyBrowser = new CLobbyBrowser( m_pGameEngine );
	m_pLobby = new CLobby( m_pGameEngine );


	// Init stats
	m_pStatsAndAchievements = new CStatsAndAchievements( pGameEngine );
	m_pLeaderboards = new CLeaderboards( pGameEngine );

	// Remote Storage page
	m_pRemoteStorage = new CRemoteStorage( pGameEngine );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpaceWarClient::~CSpaceWarClient()
{
	DisconnectFromServer();

	if ( m_pP2PAuthedGame )
	{
		m_pP2PAuthedGame->EndGame();
		delete m_pP2PAuthedGame;
		m_pP2PAuthedGame = NULL;
	}

	if ( m_pServer )
	{
		delete m_pServer;
		m_pServer = NULL; 
	}

	if ( m_pStarField )
		delete m_pStarField;

	if ( m_pMainMenu )
		delete m_pMainMenu;

	if ( m_pQuitMenu ) 
		delete m_pQuitMenu;

	if ( m_pSun )
		delete m_pSun;

	if ( m_pStatsAndAchievements )
		delete m_pStatsAndAchievements;

	if ( m_pServerBrowser )
		delete m_pServerBrowser; 

	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
		{
			delete m_rgpShips[i];
			m_rgpShips[i] = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tell the connected server we are disconnecting (if we are connected)
//-----------------------------------------------------------------------------
void CSpaceWarClient::DisconnectFromServer()
{
	if ( m_eConnectedStatus != k_EClientNotConnected )
	{
		SteamUser()->TerminateGameConnection( m_unServerIP, m_usServerPort );

		MsgClientLeavingServer_t msg;
		BSendServerData( &msg, sizeof(msg) );
		m_eConnectedStatus = k_EClientNotConnected;
	}
	if ( m_pP2PAuthedGame )
	{
		m_pP2PAuthedGame->EndGame();
	}

	// forget the game server ID
	if ( m_steamIDGameServer.IsValid() )
	{
		SteamNetworking()->CloseP2PSessionWithUser( m_steamIDGameServer );
		m_steamIDGameServer = CSteamID();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receive basic server info from the server after we initiate a connection
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerInfo( CSteamID steamIDGameServer, bool bVACSecure, const char *pchServerName )
{
	m_eConnectedStatus = k_EClientConnectedPendingAuthentication;
	m_pQuitMenu->SetHeading( pchServerName );
	m_steamIDGameServer = steamIDGameServer;

	// look up the servers IP and Port from the connection
	P2PSessionState_t p2pSessionState;
	SteamNetworking()->GetP2PSessionState( steamIDGameServer, &p2pSessionState );
	m_unServerIP = p2pSessionState.m_nRemoteIP;
	m_usServerPort = p2pSessionState.m_nRemotePort;

	MsgClientBeginAuthentication_t msg;
#ifdef USE_GS_AUTH_API
	msg.m_uTokenLen = SteamUser()->InitiateGameConnection( (void*)&msg.m_rgchToken, ARRAYSIZE( msg.m_rgchToken ), steamIDGameServer, m_unServerIP, m_usServerPort, bVACSecure );
#else
	// When you aren't using Steam auth you still call InitiateGameConnection() so you can communicate presence data to the friends
	// system. Make sure to pass k_steamIDNonSteamGS so Steam won't try to authenticate your user.
	msg.m_uTokenLen = SteamUser()->InitiateGameConnection( NULL, 0, k_steamIDNonSteamGS, m_unServerIP, m_usServerPort, false );
	msg.m_SteamID = SteamUser()->GetSteamID();
#endif

	Steamworks_TestSecret();

	if ( msg.m_uTokenLen < 1 )
		OutputDebugString( "Warning: Looks like InitiateGameConnection didn't give us a good token\n" );

	BSendServerData( &msg, sizeof(msg) );
}


//-----------------------------------------------------------------------------
// Purpose: Receive an authentication response from the server
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerAuthenticationResponse( bool bSuccess, uint32 uPlayerPosition )
{
	if ( !bSuccess )
	{
		SetConnectionFailureText( "Connection failure.\nMultiplayer authentication failed\n" );
		SetGameState( k_EClientGameConnectionFailure );
		DisconnectFromServer();
	}
	else
	{
		// Is this a duplicate message? If so ignore it...
		if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_uPlayerShipIndex == uPlayerPosition )
			return;

		m_uPlayerShipIndex = uPlayerPosition;
		m_eConnectedStatus = k_EClientConnectedAndAuthenticated;

		// send a ping, to measure round-trip time
		m_ulPingSentTime = m_pGameEngine->GetGameTickCount();
		MsgClientPing_t msg;
		BSendServerData( &msg, sizeof( msg ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles receiving a state update from the game server
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerUpdate( ServerSpaceWarUpdateData_t UpdateData )
{
	// Update our client state based on what the server tells us
	
	switch( UpdateData.m_eCurrentGameState )
	{
	case k_EServerWaitingForPlayers:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if (m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameWaitingForPlayers );
		break;
	case k_EServerActive:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if (m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameActive );
		break;
	case k_EServerDraw:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if ( m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameDraw );
		break;
	case k_EServerWinner:
		if ( m_eGameState == k_EClientGameQuitMenu )
			break;
		else if ( m_eGameState == k_EClientGameMenu )
			break;
		else if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameWinner );
		break;
	case k_EServerExiting:
		if ( m_eGameState == k_EClientGameExiting )
			break;

		SetGameState( k_EClientGameMenu );
		break;
	}

	// Update scores
	memcpy( m_rguPlayerScores, UpdateData.m_rgPlayerScores, sizeof( m_rguPlayerScores ) );

	// Update who won last
	m_uPlayerWhoWonGame = UpdateData.m_uPlayerWhoWonGame;

	if ( m_pP2PAuthedGame )
	{
		// has the player list changed?
		if ( m_pServer )
		{
			// if i am the server owner i need to auth everyone who wants to play
			// assume i am in slot 0, so start at slot 1
			for( uint32 i=1; i < MAX_PLAYERS_PER_SERVER; ++i )
			{
				CSteamID steamIDNew( UpdateData.m_rgPlayerSteamIDs[i] );
				if ( steamIDNew == SteamUser()->GetSteamID() )
				{
					OutputDebugStringA( "Server player slot 0 is not server owner.\n" );
				}
				else if ( steamIDNew != m_rgSteamIDPlayers[i] )
				{
					if ( m_rgSteamIDPlayers[i].IsValid() )
					{
						m_pP2PAuthedGame->PlayerDisconnect( i );
					}
					if ( steamIDNew.IsValid() )
					{
						m_pP2PAuthedGame->RegisterPlayer( i, steamIDNew );
					}
				}
			}
		}
		else
		{
			// i am just a client, i need to auth the game owner ( slot 0 )
			CSteamID steamIDNew( UpdateData.m_rgPlayerSteamIDs[0] );
			if ( steamIDNew == SteamUser()->GetSteamID() )
			{
				OutputDebugStringA( "Server player slot 0 is not server owner.\n" );
			}
			else if ( steamIDNew != m_rgSteamIDPlayers[0] )
			{
				if ( m_rgSteamIDPlayers[0].IsValid() )
				{
					OutputDebugStringA( "Server player slot 0 has disconnected - but thats the server owner.\n" );
					m_pP2PAuthedGame->PlayerDisconnect( 0 );
				}
				if ( steamIDNew.IsValid() )
				{
					m_pP2PAuthedGame->StartAuthPlayer( 0, steamIDNew );
				}
			}
		}
	}

	// Update the ships
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		// Update steamid array with data from server
		m_rgSteamIDPlayers[i].SetFromUint64( UpdateData.m_rgPlayerSteamIDs[i] );

		if ( UpdateData.m_rgPlayersActive[i] )
		{
			// Check if we have a ship created locally for this player slot, if not create it
			if ( !m_rgpShips[i] )
			{
				m_rgpShips[i] = new CShip( m_pGameEngine, false, UpdateData.m_rgShipData[i].m_flXPosition, UpdateData.m_rgShipData[i].m_flYPosition, g_rgPlayerColors[i] );
				if ( i == m_uPlayerShipIndex )
				{
					// If this is our local ship, then setup key bindings appropriately
					m_rgpShips[i]->SetVKBindingLeft( 0x41 ); // A key
					m_rgpShips[i]->SetVKBindingRight( 0x44 ); // D key
					m_rgpShips[i]->SetVKBindingForwardThrusters( 0x57 ); // W key
					m_rgpShips[i]->SetVKBindingReverseThrusters( 0x53 ); // S key
					m_rgpShips[i]->SetVKBindingFire( VK_SPACE ); 
				}
			}

			if ( i == m_uPlayerShipIndex )
				m_rgpShips[i]->SetIsLocalPlayer( true );
			else
				m_rgpShips[i]->SetIsLocalPlayer( false );

			m_rgpShips[i]->OnReceiveServerUpdate( UpdateData.m_rgShipData[i] );			
		}
		else
		{
			// Make sure we don't have a ship locally for this slot
			if ( m_rgpShips[i] )
			{
				delete m_rgpShips[i];
				m_rgpShips[i] = NULL;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used to transition game state
//-----------------------------------------------------------------------------
void CSpaceWarClient::SetGameState( EClientGameState eState )
{
	if ( m_eGameState == eState )
		return;

	m_bTransitionedGameState = true;
	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_eGameState = eState;

	// Let the stats handler check the state (so it can detect wins, losses, etc...)
	m_pStatsAndAchievements->OnGameStateChange( eState );
}


//-----------------------------------------------------------------------------
// Purpose: Used to transition game state
//-----------------------------------------------------------------------------
void CSpaceWarClient::SetConnectionFailureText( const char *pchErrorText )
{
	_snprintf( m_rgchErrorText, sizeof( m_rgchErrorText ), "%s", pchErrorText );
}


//-----------------------------------------------------------------------------
// Purpose: Send data to the current server
//-----------------------------------------------------------------------------
bool CSpaceWarClient::BSendServerData( const void *pData, uint32 nSizeOfData )
{
	if ( !SteamNetworking()->SendP2PPacket( m_steamIDGameServer, pData, nSizeOfData, k_EP2PSendUnreliable ) )
	{
		OutputDebugString( "Failed sending data to server\n" );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initiates a connection to a server
//-----------------------------------------------------------------------------
void CSpaceWarClient::InitiateServerConnection( uint32 unServerAddress, const int32 nPort )
{
	if ( m_eGameState == k_EClientInLobby && m_steamIDLobby.IsValid() )
	{
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
	}

	SetGameState( k_EClientGameConnecting );

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();

	// ping the server to find out what it's steamID is
	m_unServerIP = unServerAddress;
	m_usServerPort = (uint16)nPort;
	m_GameServerPing.RetrieveSteamIDFromGameServer( this, m_unServerIP, m_usServerPort );
}


//-----------------------------------------------------------------------------
// Purpose: Initiates a connection to a server via P2P (NAT-traversing) connection
//-----------------------------------------------------------------------------
void CSpaceWarClient::InitiateServerConnection( CSteamID steamIDGameServer )
{
	if ( m_eGameState == k_EClientInLobby && m_steamIDLobby.IsValid() )
	{
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
	}

	SetGameState( k_EClientGameConnecting );

	m_steamIDGameServer = steamIDGameServer;

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();

	// send the packet to the server
	MsgClientInitiateConnection_t msg;
	BSendServerData( &msg, sizeof( msg ) );
}


//-----------------------------------------------------------------------------
// Purpose: steam callback, trigger when our connection state changes
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pCallback )
{
	if ( pCallback->m_steamIDRemote == m_steamIDGameServer )
	{
		// failed, error out
		OutputDebugString( "Failed to make P2P connection, quiting server" );
		OnReceiveServerExiting();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receives incoming network data
//-----------------------------------------------------------------------------
void CSpaceWarClient::ReceiveNetworkData()
{
	char rgchRecvBuf[MAX_SPACEWAR_PACKET_SIZE];
	char *pchRecvBuf = rgchRecvBuf;
	uint32 cubMsgSize;
	for (;;)
	{
		// reset the receive buffer
		if ( pchRecvBuf != rgchRecvBuf )
		{
			free( pchRecvBuf );
			pchRecvBuf = rgchRecvBuf;
		}

		// see if there is any data waiting on the socket
		if ( !SteamNetworking()->IsP2PPacketAvailable( &cubMsgSize ) )
			break;

		// not enough space in default buffer
		// alloc custom size and try again
		if ( cubMsgSize > sizeof(rgchRecvBuf) )
		{
			pchRecvBuf = (char *)malloc( cubMsgSize );
		}
		CSteamID steamIDRemote;
		if ( !SteamNetworking()->ReadP2PPacket( pchRecvBuf, cubMsgSize, &cubMsgSize, &steamIDRemote ) )
			break;

		// see if it's from the game server
		if ( steamIDRemote == m_steamIDGameServer )
		{
			m_ulLastNetworkDataReceivedTime = m_pGameEngine->GetGameTickCount();

			// make sure we're connected
			if ( m_eConnectedStatus == k_EClientNotConnected && m_eGameState != k_EClientGameConnecting )
				continue;

			if ( cubMsgSize < sizeof( EMessage ) )
			{
				OutputDebugString( "Got garbage on client socket, too short\n" );
			}

			EMessage *pEMsg = (EMessage*)pchRecvBuf;
			switch ( *pEMsg )
			{
			case k_EMsgServerSendInfo:
				{
					if ( cubMsgSize != sizeof( MsgServerSendInfo_t ) )
					{
						OutputDebugString ("Bad server info msg\n" );
						continue;
					}
					MsgServerSendInfo_t *pMsg = (MsgServerSendInfo_t*)pchRecvBuf;

					// pull the IP address of the user from the socket
					OnReceiveServerInfo( CSteamID( pMsg->m_ulSteamIDServer ), pMsg->m_bIsVACSecure, pMsg->m_rgchServerName );
				}
				break;
			case k_EMsgServerPassAuthentication:
				{
					if ( cubMsgSize != sizeof( MsgServerPassAuthentication_t ) )
					{
						OutputDebugString( "Bad accept connection msg\n" );
						continue;
					}
					MsgServerPassAuthentication_t *pMsg = (MsgServerPassAuthentication_t*)pchRecvBuf;

					// Our game client doesn't really care about whether the server is secure, or what its 
					// steamID is, but if it did we would pass them in here as they are part of the accept message
					OnReceiveServerAuthenticationResponse( true, pMsg->m_uPlayerPosition );
				}
				break;
			case k_EMsgServerFailAuthentication:
				{
					OnReceiveServerAuthenticationResponse( false, 0 );
				}
				break;
			case k_EMsgServerUpdateWorld:
				{
					if ( cubMsgSize != sizeof( MsgServerUpdateWorld_t ) )
					{
						OutputDebugString( "Bad server world update msg\n" );
						continue;
					}

					MsgServerUpdateWorld_t *pMsg = (MsgServerUpdateWorld_t*)pchRecvBuf;
					OnReceiveServerUpdate( pMsg->m_ServerUpdateData );
				}
				break;
			case k_EMsgServerExiting:
				{
					if ( cubMsgSize != sizeof( MsgServerExiting_t ) )
					{
						OutputDebugString( "Bad server exiting msg\n" );
					}
					OnReceiveServerExiting();
				}
				break;
			case k_EMsgServerPingResponse:
				{
					uint64 ulTimePassedMS = m_pGameEngine->GetGameTickCount() - m_ulPingSentTime;
					char rgchT[256];
					_snprintf( rgchT, sizeof(rgchT), "Round-trip ping time to server %d ms\n", (int)ulTimePassedMS );
					OutputDebugString( rgchT );
					m_ulPingSentTime = 0;
				}
				break;
			default:
				OutputDebugString( "Unhandled message from server\n" );
				break;
			}
		}
		else
		{
			EMessage *pEMsg = (EMessage*)pchRecvBuf;
			m_pP2PAuthedGame->HandleMessage( *pEMsg, pchRecvBuf );
		}
	}

	// if we're running a server, do that as well
	if ( m_pServer )
	{
		m_pServer->ReceiveNetworkData();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle the server telling us it is exiting
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerExiting()
{
	if ( m_pP2PAuthedGame )
		m_pP2PAuthedGame->EndGame();
	if ( m_eGameState != k_EClientGameActive )
		return;

	if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated )
		SteamUser()->TerminateGameConnection( m_unServerIP, m_usServerPort );

	m_eConnectedStatus = k_EClientNotConnected;

	SetConnectionFailureText( "Game server has exited." );
	SetGameState( k_EClientGameConnectionFailure );
}


//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby of our own creation
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure )
{
	if ( m_eGameState != k_EClientCreatingLobby )
		return;

	// record which lobby we're in
	if ( pCallback->m_eResult == k_EResultOK )
	{
		// success
		m_steamIDLobby = pCallback->m_ulSteamIDLobby;
		m_pLobby->SetLobbySteamID( m_steamIDLobby );

		// set the name of the lobby if it's ours
		char rgchLobbyName[256];
		_snprintf( rgchLobbyName, sizeof( rgchLobbyName ), "%s's lobby", SteamFriends()->GetPersonaName() );
		SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "name", rgchLobbyName );

		// mark that we're in the lobby
		SetGameState( k_EClientInLobby );
	}
	else
	{
		// failed, show error
		SetConnectionFailureText( "Failed to create lobby (lost connection to Steam back-end servers." );
		SetGameState( k_EClientGameConnectionFailure );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Finishes up entering a lobby
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnLobbyEntered( LobbyEnter_t *pCallback, bool bIOFailure )
{
	if ( m_eGameState != k_EClientJoiningLobby )
		return;

	// success
	// move forward the state
	SetGameState( k_EClientInLobby );
	m_steamIDLobby = pCallback->m_ulSteamIDLobby;
	m_pLobby->SetLobbySteamID( m_steamIDLobby );

}

void CSpaceWarClient::OnLobbyDataChange( LobbyDataUpdate_t *pCallback, bool bIOFailure )
{
	OutputDebugStringA( "OnLobbyDataChange ");
	CSteamID steamIDLobbyMember( pCallback->m_ulSteamIDMember );
}

void CSpaceWarClient::OnLobbyChatUpdate( LobbyChatUpdate_t *pCallback, bool bIOFailure )
{
	CSteamID steamIDLobbyMember( pCallback->m_ulSteamIDUserChanged );
	CSteamID steamIDLobbyMember2( pCallback->m_ulSteamIDMakingChange );
	OutputDebugStringA( "OnLobbyChatUpdate ");
}

//-----------------------------------------------------------------------------
// Purpose: Joins a game from a lobby
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnLobbyGameCreated( LobbyGameCreated_t *pCallback )
{
	if ( m_eGameState != k_EClientInLobby )
		return;

	// join the game server specified, via whichever method we can
	if ( CSteamID( pCallback->m_ulSteamIDGameServer ).IsValid() )
	{
		InitiateServerConnection( CSteamID( pCallback->m_ulSteamIDGameServer ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles menu actions in a lobby
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnMenuSelection( LobbyMenuItem_t selection )
{
	if ( selection.m_bLeaveLobby )
	{
		// leave the lobby
		SteamMatchmaking()->LeaveLobby( m_steamIDLobby );
		m_steamIDLobby = CSteamID();

		// return to main menu
		SetGameState( k_EClientGameMenu );
	}
	else if ( selection.m_bToggleReadyState )
	{
		// update our state
		bool bOldState = ( 1 == atoi( SteamMatchmaking()->GetLobbyMemberData( m_steamIDLobby, SteamUser()->GetSteamID(), "ready" ) ) );
		bool bNewState = !bOldState;
		// publish to everyone
		SteamMatchmaking()->SetLobbyMemberData( m_steamIDLobby, "ready", bNewState ? "1" : "0" );
	}
	else if ( selection.m_bStartGame )
	{
		// make sure we're not already starting a server
		if ( m_pServer )
			return;

		// broadcast to everyone in the lobby that the game is starting
		SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "game_starting", "1" );
		// start a local game server
		m_pServer = new CSpaceWarServer( m_pGameEngine );
		// we'll have to wait until the game server connects to the Steam server back-end 
		// before telling all the lobby members to join (so that the NAT traversal code has a path to contact the game server)
		OutputDebugString( "Game server being created; game will start soon.\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles menu actions when viewing a leaderboard
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnMenuSelection( LeaderboardMenuItem_t selection )
{
	m_pLeaderboards->OnMenuSelection( selection );
}


//-----------------------------------------------------------------------------
// Purpose: does work on transitioning from one game state to another
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnGameStateChanged( EClientGameState eGameStateNew )
{
	if ( m_eGameState == k_EClientFindInternetServers )
	{
		// If we are just opening the find servers screen, then start a refresh
		m_pServerBrowser->RefreshInternetServers();
	}
	else if ( m_eGameState == k_EClientFindLANServers )
	{
		m_pServerBrowser->RefreshLANServers();
	}
	else if ( m_eGameState == k_EClientCreatingLobby )
	{
		// start creating the lobby
		if ( !m_SteamCallResultLobbyCreated.IsActive() )
		{
			// ask steam to create a lobby
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby( k_ELobbyTypePublic /* public lobby, anyone can find it */, 4 );
			// set the function to call when this completes
			m_SteamCallResultLobbyCreated.Set( hSteamAPICall, this, &CSpaceWarClient::OnLobbyCreated );
		}
	}
	else if ( m_eGameState == k_EClientFindLobby )
	{
		m_pLobbyBrowser->Refresh();
	}
	else if ( m_eGameState == k_EClientGameMenu )
	{
		// we've switched out to the main menu

		// Tell the server we have left if we are connected
		DisconnectFromServer();

		// shut down any server we were running
		if ( m_pServer )
		{
			delete m_pServer;
			m_pServer = NULL;
		}
	}
	else if ( m_eGameState == k_EClientGameWinner || m_eGameState == k_EClientGameDraw )
	{
		// game over.. update the leaderboard
		m_pLeaderboards->UpdateLeaderboards( m_pStatsAndAchievements );
	}
	else if ( m_eGameState == k_EClientLeaderboards )
	{
		// we've switched to the leaderboard menu
		m_pLeaderboards->Show();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification of a steam ipc failure
// we may get multiple callbacks, one for each IPC operation we attempted
// since the actual failure, so protect ourselves from alerting more than once.
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnIPCFailure( IPCFailure_t *failure )
{
	static bool bExiting = false;
	if ( ! bExiting )
	{
		OutputDebugString( "Steam IPC Failure, shutting down\n" );
		::MessageBoxA( NULL, "Connection to Steam Lost, Exiting", "Steam Connection Error", MB_OK );
		m_pGameEngine->Shutdown();
		bExiting = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification of a Steam shutdown request since a Windows
// user in a second concurrent session requests to play this game. Shutdown
// this process immediately if possible.
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnSteamShutdown( SteamShutdown_t *callback )
{
	static bool bExiting = false;
	if ( !bExiting )
	{
		OutputDebugString( "Steam shutdown request, shutting down\n" );
		m_pGameEngine->Shutdown();
		bExiting = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Main frame function, updates the state of the world and performs rendering
//-----------------------------------------------------------------------------
void CSpaceWarClient::RunFrame()
{
	// Get any new data off the network to begin with
	ReceiveNetworkData();

	if ( m_eConnectedStatus != k_EClientNotConnected && m_pGameEngine->GetGameTickCount() - m_ulLastNetworkDataReceivedTime > MILLISECONDS_CONNECTION_TIMEOUT )
	{
		SetConnectionFailureText( "Game server connection failure." );
		DisconnectFromServer(); // cleanup on our side, even though server won't get our disconnect msg
		SetGameState( k_EClientGameConnectionFailure );
	}

	// Check if escape has been pressed, we'll use that info in a couple places below
	bool bEscapePressed = false;
	if ( m_pGameEngine->BIsKeyDown( VK_ESCAPE ) )
	{
		static uint64 m_ulLastESCKeyTick = 0;
		uint64 ulCurrentTickCount = m_pGameEngine->GetGameTickCount();
		if ( ulCurrentTickCount - 250 > m_ulLastESCKeyTick )
		{
			m_ulLastESCKeyTick = ulCurrentTickCount;
			bEscapePressed = true;
		}
	}

	// Run Steam client callbacks
	SteamAPI_RunCallbacks();

	// For now, run stats/achievements every frame
	m_pStatsAndAchievements->RunFrame();

	// if we just transitioned state, perform on change handlers
	if ( m_bTransitionedGameState )
	{
		m_bTransitionedGameState = false;
		OnGameStateChanged( m_eGameState );
	}

	// Update state for everything
	switch ( m_eGameState )
	{
	case k_EClientGameMenu:
		m_pStarField->Render();
		m_pMainMenu->RunFrame();
		break;
	case k_EClientFindInternetServers:
	case k_EClientFindLANServers:
		m_pStarField->Render();
		m_pServerBrowser->RunFrame();
		break;
	
	case k_EClientCreatingLobby:
		m_pStarField->Render();
		// draw some text about creating lobby (may take a second or two)
		break;

	case k_EClientInLobby:
		m_pStarField->Render();
		// display the lobby
		m_pLobby->RunFrame();
		
		// see if we have a game server ready to play on
		if ( m_pServer && m_pServer->IsConnectedToSteam() )
		{
			// server is up; tell everyone else to connect
			SteamMatchmaking()->SetLobbyGameServer( m_steamIDLobby, 0, 0, m_pServer->GetSteamID() );
			// start connecting ourself via localhost (this will automatically leave the lobby)
			InitiateServerConnection( m_pServer->GetSteamID() );
		}
		break;

	case k_EClientFindLobby:
		m_pStarField->Render();

		// display the list of lobbies
		m_pLobbyBrowser->RunFrame();
		break;

	case k_EClientJoiningLobby:
		m_pStarField->Render();

		// Draw text telling the user a connection attempt is in progress
		DrawConnectionAttemptText();

		// Check if we've waited too long and should time out the connection
		if ( m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT )
		{
			SetConnectionFailureText( "Timed out connecting to lobby." );
			SetGameState( k_EClientGameConnectionFailure );
		}
		break;

	case k_EClientGameConnectionFailure:
		m_pStarField->Render();
		DrawConnectionFailureText();

		if ( bEscapePressed )
			SetGameState( k_EClientGameMenu );

		break;
	case k_EClientGameConnecting:
		m_pStarField->Render();

		// Draw text telling the user a connection attempt is in progress
		DrawConnectionAttemptText();

		// Check if we've waited too long and should time out the connection
		if ( m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT )
		{
			if ( m_pP2PAuthedGame )
				m_pP2PAuthedGame->EndGame();
			if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated )
				SteamUser()->TerminateGameConnection( m_unServerIP, m_usServerPort );
			
			m_GameServerPing.CancelPing();
			SetConnectionFailureText( "Timed out connecting to game server" );
			SetGameState( k_EClientGameConnectionFailure );
		}

		break;
	case k_EClientGameQuitMenu:
		m_pStarField->Render();

		// Update all the entities (this is client side interpolation)...
		m_pSun->RunFrame();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->RunFrame();
		}

		// Now draw the menu
		m_pQuitMenu->RunFrame();
		break;
	case k_EClientGameInstructions:
		m_pStarField->Render();
		DrawInstructions();

		if ( bEscapePressed )
			SetGameState( k_EClientGameMenu );
		break;
	case k_EClientStatsAchievements:
		m_pStarField->Render();
		m_pStatsAndAchievements->Render();

		if ( bEscapePressed )
			SetGameState( k_EClientGameMenu );
		break;
	case k_EClientLeaderboards:
		m_pStarField->Render();
		m_pLeaderboards->RunFrame();		

		if ( bEscapePressed )
			SetGameState( k_EClientGameMenu );
		break;

	case k_EClientRemoteStorage:
		m_pStarField->Render();
		m_pRemoteStorage->Render();

		if ( m_pRemoteStorage->BFinished() )
			SetGameState( k_EClientGameMenu );
		break;

	case k_EClientMinidump:
		RaiseException( EXCEPTION_NONCONTINUABLE_EXCEPTION,
			EXCEPTION_NONCONTINUABLE,
			0, NULL );

		SetGameState( k_EClientGameMenu );
		break;

	case k_EClientGameStartServer:
		m_pStarField->Render();
		if ( !m_pServer )
		{
			m_pServer = new CSpaceWarServer( m_pGameEngine );
		}

		if ( m_pServer && m_pServer->IsConnectedToSteam() )
		{
			// server is ready, connect to it
			InitiateServerConnection( m_pServer->GetSteamID() );
		}
		break;
	case k_EClientGameDraw:
	case k_EClientGameWinner:
	case k_EClientGameWaitingForPlayers:
		m_pStarField->Render();

		// Update all the entities (this is client side interpolation)...
		m_pSun->RunFrame();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->RunFrame();
		}

		DrawHUDText();
		DrawWinnerDrawOrWaitingText();

		if ( bEscapePressed )
			SetGameState( k_EClientGameQuitMenu );
		break;

	case k_EClientGameActive:
		m_pStarField->Render();

		// Update all the entities...
		m_pSun->RunFrame();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->RunFrame();
		}
		DrawHUDText();

		m_pStatsAndAchievements->RunFrame();

		if ( bEscapePressed )
			SetGameState( k_EClientGameQuitMenu );
		break;
	case k_EClientGameExiting:
		DisconnectFromServer();
		m_pGameEngine->Shutdown();
		return;
	default:
		OutputDebugString( "Unhandled game state in CSpaceWar::RunFrame\n" );
	}


	// Send an update on our local ship to the server
	if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated &&  m_rgpShips[ m_uPlayerShipIndex ] )
	{
		MsgClientSendLocalUpdate_t msg;
		msg.m_uShipPosition = m_uPlayerShipIndex;

		// If this fails, it probably just means its not time to send an update yet
		if ( m_rgpShips[ m_uPlayerShipIndex ]->BGetClientUpdateData( &msg.m_ClientUpdateData ) )
			BSendServerData( &msg, sizeof( msg ) );
	}

	if ( m_pP2PAuthedGame )
	{
		if ( m_pServer )
		{
			// Now if we are the owner of the game, lets make sure all of our players are legit.
			// if they are not, we tell the server to kick them off
			// Start at 1 to skip myself
			for ( int i = 1; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				if ( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i] && !m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i]->BIsAuthOk() )
				{
					m_pServer->KickPlayerOffServer( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[i]->m_steamID );
				}
			}
		}
		else
		{
			// If we are not the owner of the game, lets make sure the game owner is legit
			// if he is not, we leave the game
			if ( m_pP2PAuthedGame->m_rgpP2PAuthPlayer[0] )
			{
				if ( !m_pP2PAuthedGame->m_rgpP2PAuthPlayer[0]->BIsAuthOk() )
				{
					// leave the game
					SetGameState( k_EClientGameMenu );
				}
			}
		}
	}

	// If we've started a local server run it
	if ( m_pServer )
	{
		m_pServer->RunFrame();
	}

	// Accumulate stats
	for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
			m_rgpShips[i]->AccumulateStats( m_pStatsAndAchievements );
	}

	// Render everything that might have been updated by the server
	switch ( m_eGameState )
	{
	case k_EClientGameDraw:
	case k_EClientGameWinner:
	case k_EClientGameActive:
		// Now render all the objects
		m_pSun->Render();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->Render();
		}
		break;
	default:
		// Any needed drawing was already done above before server updates
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws some HUD text indicating game status
//-----------------------------------------------------------------------------
void CSpaceWarClient::DrawHUDText()
{
	// Padding from the edge of the screen for hud elements
	const int32 nHudPadding = 15;

	const int32 width = m_pGameEngine->GetViewportWidth();
	const int32 height = m_pGameEngine->GetViewportHeight();

	const int32 nAvatarWidth = 64;
	const int32 nAvatarHeight = 64;

	const int32 nSpaceBetweenAvatarAndScore = 6;

	LONG scorewidth = LONG((m_pGameEngine->GetViewportWidth() - nHudPadding*2.0f)/4.0f);

	char rgchBuffer[256];
	for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		// Draw nothing in the spot for an inactive player
		if ( !m_rgpShips[i] )
			continue;


		// We use Steam persona names for our players in-game name.  To get these we 
		// just call SteamFriends()->GetFriendPersonaName() this call will work on friends, 
		// players on the same game server as us (if using the Steam game server auth API) 
		// and on ourself.
		char rgchPlayerName[128];
		if ( m_rgSteamIDPlayers[i].IsValid() )
		{
			_snprintf( rgchPlayerName, ARRAYSIZE( rgchPlayerName ), "%s", SteamFriends()->GetFriendPersonaName( m_rgSteamIDPlayers[i] ) );
		}
		else
		{
			_snprintf( rgchPlayerName, ARRAYSIZE( rgchPlayerName ), "Unknown Player" );
		}

		// We also want to use the Steam Avatar image inside the HUD if it is available.
		// We look it up via GetFriendAvatar, which returns an image index we use
		// to look up the actual RGBA data below.
		int iImage = SteamFriends()->GetFriendAvatar( m_rgSteamIDPlayers[i], k_EAvatarSize64x64 );
		HGAMETEXTURE hTexture = GetSteamImageAsTexture( iImage );

		RECT rect;
		switch( i )
		{
		case 0:
			rect.top = nHudPadding;
			rect.bottom = rect.top+nAvatarHeight;
			rect.left = nHudPadding;
			rect.right = rect.left + scorewidth;

			if ( hTexture )
			{
				m_pGameEngine->BDrawTexturedQuad( (float)rect.left, (float)rect.top, (float)rect.left+nAvatarWidth, (float)rect.bottom, 
					0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );
				rect.left += nAvatarWidth + nSpaceBetweenAvatarAndScore;
				rect.right += nAvatarWidth + nSpaceBetweenAvatarAndScore;
			}
			
			_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s\nScore: %2u", rgchPlayerName, m_rguPlayerScores[i] );
			m_pGameEngine->BDrawString( m_hHUDFont, rect, g_rgPlayerColors[i], DT_LEFT|DT_VCENTER, rgchBuffer );
			break;
		case 1:

			rect.top = nHudPadding;
			rect.bottom = rect.top+nAvatarHeight;
			rect.left = width-nHudPadding-scorewidth;
			rect.right = width-nHudPadding;

			if ( hTexture )
			{
				m_pGameEngine->BDrawTexturedQuad( (float)rect.right - nAvatarWidth, (float)rect.top, (float)rect.right, (float)rect.bottom, 
					0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );
				rect.right -= nAvatarWidth + nSpaceBetweenAvatarAndScore;
				rect.left -= nAvatarWidth + nSpaceBetweenAvatarAndScore;
			}

			_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s\nScore: %2u", rgchPlayerName, m_rguPlayerScores[i] );
			m_pGameEngine->BDrawString( m_hHUDFont, rect, g_rgPlayerColors[i], DT_RIGHT|DT_VCENTER, rgchBuffer );
			break;
		case 2:
			rect.top = height-nHudPadding-nAvatarHeight;
			rect.bottom = rect.top+nAvatarHeight;
			rect.left = nHudPadding;
			rect.right = rect.left + scorewidth;

			if ( hTexture )
			{
				m_pGameEngine->BDrawTexturedQuad( (float)rect.left, (float)rect.top, (float)rect.left+nAvatarWidth, (float)rect.bottom, 
					0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );
				rect.right += nAvatarWidth + nSpaceBetweenAvatarAndScore;
				rect.left += nAvatarWidth + nSpaceBetweenAvatarAndScore;
			}

			_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s\nScore: %2u", rgchPlayerName, m_rguPlayerScores[i] );
			m_pGameEngine->BDrawString( m_hHUDFont, rect, g_rgPlayerColors[i], DT_LEFT|DT_BOTTOM, rgchBuffer );
			break;
		case 3:
			rect.top = height-nHudPadding-nAvatarHeight;
			rect.bottom = rect.top+nAvatarHeight;
			rect.left = width-nHudPadding-scorewidth;
			rect.right = width-nHudPadding;

			if ( hTexture )
			{
				m_pGameEngine->BDrawTexturedQuad( (float)rect.right - nAvatarWidth, (float)rect.top, (float)rect.right, (float)rect.bottom, 
					0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );
				rect.right -= nAvatarWidth + nSpaceBetweenAvatarAndScore;
				rect.left -= nAvatarWidth + nSpaceBetweenAvatarAndScore;
			}

			_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s\nScore: %2u", rgchPlayerName, m_rguPlayerScores[i] );
			m_pGameEngine->BDrawString( m_hHUDFont, rect, g_rgPlayerColors[i], DT_RIGHT|DT_BOTTOM, rgchBuffer );
			break;
		default:
			OutputDebugString( "DrawHUDText() needs updating for more players\n" );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws some instructions on how to play the game
//-----------------------------------------------------------------------------
void CSpaceWarClient::DrawInstructions()
{
	const int32 width = m_pGameEngine->GetViewportWidth();

	RECT rect;
	rect.top = 0;
	rect.bottom = m_pGameEngine->GetViewportHeight();
	rect.left = 0;
	rect.right = width;

	char rgchBuffer[256];
	_snprintf( rgchBuffer, sizeof( rgchBuffer), "Turn Ship Left: 'A'\nTurn Ship Right: 'D'\nForward Thrusters: 'W'\nReverse Thrusters: 'S'\nFire Photon Beams: 'Space'" );
	m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );

	
	rect.left = 0;
	rect.right = width;
	rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.7);
	rect.bottom = m_pGameEngine->GetViewportHeight();

	_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Press ESC to return to the Main Menu" );
	m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_TOP, rgchBuffer );

}

//-----------------------------------------------------------------------------
// Purpose: Draws some text indicating a connection attempt is in progress
//-----------------------------------------------------------------------------
void CSpaceWarClient::DrawConnectionAttemptText()
{
	const int32 width = m_pGameEngine->GetViewportWidth();

	RECT rect;
	rect.top = 0;
	rect.bottom = m_pGameEngine->GetViewportHeight();
	rect.left = 0;
	rect.right = width;

	// Figure out how long we are still willing to wait for success
	uint32 uSecondsLeft = (MILLISECONDS_CONNECTION_TIMEOUT - uint32(m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime ))/1000;

	char rgchTimeoutString[256];
	if ( uSecondsLeft < 25 )
		_snprintf( rgchTimeoutString, sizeof( rgchTimeoutString ), ", timeout in %u...\n", uSecondsLeft );
	else
		_snprintf( rgchTimeoutString, sizeof( rgchTimeoutString ), "...\n" );
		

	char rgchBuffer[256];
	if ( m_eGameState == k_EClientJoiningLobby )
		_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Connecting to lobby%s", rgchTimeoutString );
	else
		_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Connecting to server%s", rgchTimeoutString );

	m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );
}


//-----------------------------------------------------------------------------
// Purpose: Draws some text indicating a connection failure
//-----------------------------------------------------------------------------
void CSpaceWarClient::DrawConnectionFailureText()
{
	const int32 width = m_pGameEngine->GetViewportWidth();

	RECT rect;
	rect.top = 0;
	rect.bottom = m_pGameEngine->GetViewportHeight();
	rect.left = 0;
	rect.right = width;

	char rgchBuffer[256];
	_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s\n", m_rgchErrorText );
	m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );

	rect.left = 0;
	rect.right = width;
	rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.7);
	rect.bottom = m_pGameEngine->GetViewportHeight();

	_snprintf( rgchBuffer, sizeof( rgchBuffer ), "Press ESC to return to the Main Menu" );
	m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_TOP, rgchBuffer );
}


//-----------------------------------------------------------------------------
// Purpose: Draws some text about who just won (or that there was a draw)
//-----------------------------------------------------------------------------
void CSpaceWarClient::DrawWinnerDrawOrWaitingText()
{
	int nSecondsToRestart = ((MILLISECONDS_BETWEEN_ROUNDS - (int)(m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime) )/1000) + 1;

	RECT rect;
	rect.top = 0;
	rect.bottom = int(m_pGameEngine->GetViewportHeight()*0.6f);
	rect.left = 0;
	rect.right = m_pGameEngine->GetViewportWidth();

	char rgchBuffer[256];
	if ( m_eGameState == k_EClientGameWaitingForPlayers )
	{
		_snprintf( rgchBuffer, sizeof( rgchBuffer), "Server is waiting for players.\n\nStarting in %d seconds...", nSecondsToRestart );
		m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );
	} 
	else if ( m_eGameState == k_EClientGameDraw )
	{
		_snprintf( rgchBuffer, sizeof( rgchBuffer), "The round is a draw!\n\nStarting again in %d seconds...", nSecondsToRestart );
		m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );
	} 
	else if ( m_eGameState == k_EClientGameWinner )
	{
		if ( m_uPlayerWhoWonGame < 0 || m_uPlayerWhoWonGame >= MAX_PLAYERS_PER_SERVER )
		{
			OutputDebugString( "Invalid winner value\n" );
			return;
		}

		char rgchPlayerName[128];
		if ( m_rgSteamIDPlayers[m_uPlayerWhoWonGame].IsValid() )
		{
			_snprintf( rgchPlayerName, ARRAYSIZE( rgchPlayerName ), "%s", SteamFriends()->GetFriendPersonaName( m_rgSteamIDPlayers[m_uPlayerWhoWonGame] ) );
		}
		else
		{
			_snprintf( rgchPlayerName, ARRAYSIZE( rgchPlayerName ), "Unknown Player" );
		}

		_snprintf( rgchBuffer, sizeof( rgchBuffer), "%s wins!\n\nStarting again in %d seconds...", rgchPlayerName, nSecondsToRestart );
		
		m_pGameEngine->BDrawString( m_hInstructionsFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), DT_CENTER|DT_VCENTER, rgchBuffer );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Did we win the last game?
//-----------------------------------------------------------------------------
bool CSpaceWarClient::BLocalPlayerWonLastGame()
{
	if ( m_eGameState == k_EClientGameWinner )
	{
		if ( m_uPlayerWhoWonGame < 0 || m_uPlayerWhoWonGame >= MAX_PLAYERS_PER_SERVER )
		{
			// ur
			return false;
		}

		if ( m_rgpShips[m_uPlayerWhoWonGame] && m_rgpShips[m_uPlayerWhoWonGame]->BIsLocalPlayer() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scale pixel sizes to "real" sizes
//-----------------------------------------------------------------------------
float CSpaceWarClient::PixelsToFeet( float flPixels )
{
	// This game is actual size! (at 72dpi) LOL
	// Those are very tiny ships, and an itty bitty neutron star

	float flReturn = ( flPixels / 72 ) / 12;

	return flReturn;
}


//-----------------------------------------------------------------------------
// Purpose: Get a specific Steam image RGBA as a game texture
//-----------------------------------------------------------------------------
HGAMETEXTURE CSpaceWarClient::GetSteamImageAsTexture( int iImage )
{
	HGAMETEXTURE hTexture = NULL;

	// iImage of 0 from steam means no avatar is set
	if ( iImage )
	{
		std::map<int, HGAMETEXTURE>::iterator iter;
		iter = m_MapSteamImagesToTextures.find( iImage );
		if ( iter == m_MapSteamImagesToTextures.end() )
		{
			// We haven't created a texture for this image index yet, do so now

			// Get the image size from Steam, making sure it looks valid afterwards
			uint32 uAvatarWidth, uAvatarHeight;
			SteamUtils()->GetImageSize( iImage, &uAvatarWidth, &uAvatarHeight );
			if ( uAvatarWidth > 0 && uAvatarHeight > 0 )
			{
				// Get the actual raw RGBA data from Steam and turn it into a texture in our game engine
				byte *pAvatarRGBA = new byte[ uAvatarWidth * uAvatarHeight * 4];
				SteamUtils()->GetImageRGBA( iImage, (uint8*)pAvatarRGBA, uAvatarWidth * uAvatarHeight * 4 );
				hTexture = m_pGameEngine->HCreateTexture( pAvatarRGBA, uAvatarWidth, uAvatarHeight );
				delete[] pAvatarRGBA;
				if ( hTexture )
				{
					m_MapSteamImagesToTextures[ iImage ] = hTexture;
				}
			}
		}
		else
		{
			hTexture = iter->second;
		}
	}

	return hTexture;
}
