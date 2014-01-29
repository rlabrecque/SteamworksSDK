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
#include "Lobby.h"

CSpaceWarClient *g_pSpaceWarClient = NULL;
CSpaceWarClient* SpaceWarClient() { return g_pSpaceWarClient; }

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpaceWarClient::CSpaceWarClient( CGameEngine *pGameEngine, CSteamID steamIDUser ) :
		m_SocketStatusCallback( this, &CSpaceWarClient::OnSocketStatusCallback ),
		m_LobbyCreatedCallback( this, &CSpaceWarClient::OnLobbyCreated ),
		m_LobbyEnteredCallback( this, &CSpaceWarClient::OnLobbyEntered ),
		m_LobbyGameCreated( this, &CSpaceWarClient::OnLobbyGameCreated )
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
	m_hSocketClient = NULL;
	m_bCreatingLobby = false;
	m_bTransitionedGameState = false;
	m_rgchErrorText[0] = 0;
	m_unServerIP = 0;
	m_usServerPort = 0;

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

	// Create matchmaking menus
	m_pServerBrowser = new CServerBrowser( m_pGameEngine );
	m_pLobbyBrowser = new CLobbyBrowser( m_pGameEngine );
	m_pLobby = new CLobby( m_pGameEngine );

	// Init stats
	m_pStatsAndAchievements = new CStatsAndAchievements( pGameEngine );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpaceWarClient::~CSpaceWarClient()
{
	DisconnectFromServer();

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

	// Close down our socket if it appears to be valid
	if ( m_hSocketClient )
	{
		SteamNetworking()->DestroySocket( m_hSocketClient, true );
		m_hSocketClient = NULL;
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
		BSendServerData( (char*)&msg, sizeof(msg), true );
		m_eConnectedStatus = k_EClientNotConnected;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receive basic server info from the server after we initiate a connection
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerInfo( CSteamID steamIDGameServer, bool bVACSecure, const char *pchServerName )
{
	m_eConnectedStatus = k_EClientConnectedPendingAuthentication;
	m_pQuitMenu->SetHeading( pchServerName );

	MsgClientBeginAuthentication_t msg;
	msg.m_ulSteamID = SteamUser()->GetSteamID().ConvertToUint64();
	msg.m_uTokenLen = SteamUser()->InitiateGameConnection( (void*)&msg.m_rgchToken, ARRAYSIZE( msg.m_rgchToken ), steamIDGameServer, CGameID( SteamUtils()->GetAppID() ), m_unServerIP, m_usServerPort, bVACSecure );

	if ( msg.m_uTokenLen < 1 )
		OutputDebugString( "Warning: Looks like InitiateGameConnection didn't give us a good token\n" );

	BSendServerData( (char*)&msg, sizeof(msg), true );
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
bool CSpaceWarClient::BSendServerData( char *pData, uint32 nSizeOfData, bool bSendReliably )
{
	if ( !SteamNetworking()->SendDataOnSocket( m_hSocketClient, pData, nSizeOfData, bSendReliably ) )
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

	// Create UDP socket for the client to talk to the server over
	m_hSocketClient = SteamNetworking()->CreateConnectionSocket( unServerAddress, nPort, 10 );
	if ( !m_hSocketClient )
	{
		OutputDebugString( "Failed creating socket for CSpaceWarClient -- won't be able to talk to server\n" );
	}

	// wait for the callback before initiating connection
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

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();

	// Create UDP socket for the client to talk to the server over
	m_hSocketClient = SteamNetworking()->CreateP2PConnectionSocket( steamIDGameServer, 0, 10 );
	if ( !m_hSocketClient )
	{
		OutputDebugString( "Failed creating socket for CSpaceWarClient -- won't be able to talk to server\n" );
	}

	// wait for the callback before initiating connection
}


//-----------------------------------------------------------------------------
// Purpose: steam callback, trigger when our connection state changes
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnSocketStatusCallback( SocketStatusCallback_t *pCallback )
{
	// make sure the callback is for our socket
	if ( pCallback->m_hSocket != m_hSocketClient )
		return;

	// Update out network data received time even though we haven't really received anything yet,
	// this will help us make sure we don't time out prematurely during the handshake
	m_ulLastNetworkDataReceivedTime = m_pGameEngine->GetGameTickCount();

	// if we've finished connecting to the server, send the connect message
	if ( pCallback->m_eSNetSocketState == k_ESNetSocketStateConnected )
	{
		MsgClientInitiateConnection_t msg;
		BSendServerData( (char*)&msg, sizeof( msg ), true );
	}

	// handling being disconnect from the server signal via network breakage, or the remote end dying
	if ( pCallback->m_eSNetSocketState == k_ESNetSocketStateRemoteEndDisconnected 
		|| pCallback->m_eSNetSocketState == k_ESNetSocketStateConnectionBroken )
	{
		OnReceiveServerExiting();
		m_hSocketClient = NULL;
	}

	if ( pCallback->m_eSNetSocketState >= k_ESNetSocketStateDisconnecting )
	{
		m_hSocketClient = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receives incoming network data
//-----------------------------------------------------------------------------
void CSpaceWarClient::ReceiveNetworkData()
{
	if ( !m_hSocketClient )
		return;

	char rgchRecvBuf[MAX_SPACEWAR_PACKET_SIZE];
	char *pchRecvBuf = rgchRecvBuf;
	uint32 cubMsgSize;
	while ( 1 )
	{
		// reset the receive buffer
		if ( pchRecvBuf != rgchRecvBuf )
		{
			free( pchRecvBuf );
			pchRecvBuf = rgchRecvBuf;
		}

		// see if there is any data waiting on the socket
		if ( !SteamNetworking()->RetrieveDataFromSocket( m_hSocketClient, rgchRecvBuf, sizeof( rgchRecvBuf ), &cubMsgSize ) )
			break;

		// not enough space in buffer, so message won't have been popped off of list
		// alloc custom size and try again
		if ( cubMsgSize > sizeof(rgchRecvBuf) )
		{
			pchRecvBuf = (char *)malloc( cubMsgSize );
			SteamNetworking()->RetrieveDataFromSocket( m_hSocketClient, pchRecvBuf, cubMsgSize, &cubMsgSize );
		}

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
				SteamNetworking()->GetSocketInfo( m_hSocketClient, NULL, NULL, &m_unServerIP, &m_usServerPort );
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
		default:
			OutputDebugString( "Unhandled message from server\n" );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle the server telling us it is exiting
//-----------------------------------------------------------------------------
void CSpaceWarClient::OnReceiveServerExiting()
{
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
void CSpaceWarClient::OnLobbyCreated( LobbyCreated_t *pCallback )
{
	if ( m_eGameState != k_EClientCreatingLobby )
		return;

	// record which lobby we're in
	if ( pCallback->m_eResult == k_EResultOK )
	{
		// success
		// now we just need to wait to join the job we just created
		SetGameState( k_EClientJoiningLobby );
		m_steamIDLobby = pCallback->m_ulSteamIDLobby;
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
void CSpaceWarClient::OnLobbyEntered( LobbyEnter_t *pCallback )
{
	if ( m_eGameState != k_EClientJoiningLobby )
		return;

	// success
	// move forward the state
	SetGameState( k_EClientInLobby );
	m_steamIDLobby = pCallback->m_ulSteamIDLobby;
	m_pLobby->SetLobbySteamID( m_steamIDLobby );

	// set the name of the lobby if it's ours
	if ( m_bCreatingLobby )
	{
		m_bCreatingLobby = false;
		char rgchLobbyName[256];
		_snprintf( rgchLobbyName, sizeof( rgchLobbyName ), "%s's lobby", SteamFriends()->GetPersonaName() );
		SteamMatchmaking()->SetLobbyData( m_steamIDLobby, "name", rgchLobbyName );
	}
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
	else
	{
		// fall back to using the IP address directly
		InitiateServerConnection( pCallback->m_unIP, pCallback->m_usPort );
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
		if ( !m_bCreatingLobby )
		{
			m_bCreatingLobby = true;

			// ask steam to create a lobby
			SteamMatchmaking()->CreateLobby( false /* public lobby, anyone can find it */ );

			// result is returned through LobbyCreated_t callback, wait for that
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
			SteamMatchmaking()->SetLobbyGameServer( m_steamIDLobby, m_pServer->GetIP(), m_pServer->GetPort(), m_pServer->GetSteamID() );

			// start connecting ourself via localhost (this will automatically leave the lobby)
			uint32 unIPAddress = ( 1 ) + ( 0 << 8 ) + ( 0 << 16 ) + ( 127 << 24 );
			InitiateServerConnection( unIPAddress, m_pServer->GetPort() );
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
			if ( m_eConnectedStatus == k_EClientConnectedAndAuthenticated )
				SteamUser()->TerminateGameConnection( m_unServerIP, m_usServerPort );

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
	case k_EClientGameStartServer:
		m_pStarField->Render();
		if ( !m_pServer )
		{
			m_pServer = new CSpaceWarServer( m_pGameEngine );
			uint32 unIPAddress = ( 1 ) + ( 0 << 8 ) + ( 0 << 16 ) + ( 127 << 24 );
			InitiateServerConnection( unIPAddress, SPACEWAR_SERVER_PORT );
		}
		else
		{
			OutputDebugString( "Server already started locally when in k_EClientGameStartServer state!\n" );
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
			BSendServerData( (char*)&msg, sizeof( msg ), false );
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

	const int32 nAvatarWidth = 40; // scaled up from 32 which steam gives us
	const int32 nAvatarHeight = 40;

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
		int iImage = SteamFriends()->GetFriendAvatar( m_rgSteamIDPlayers[i] );
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
