//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Main class for the space war game server
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "SpaceWarServer.h"
#include "SpaceWarClient.h"
#include "stdlib.h"
#include "time.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor -- note the syntax for setting up Steam API callback handlers
//-----------------------------------------------------------------------------
CSpaceWarServer::CSpaceWarServer( CGameEngine *pGameEngine ) :
	m_CallbackSteamServersConnected( this, &CSpaceWarServer::OnSteamServersConnected ),
	m_CallbackSteamServersDisconnected( this, &CSpaceWarServer::OnSteamServersDisconnected ),
	m_CallbackPolicyResponse( this, &CSpaceWarServer::OnPolicyResponse ),
	m_CallbackGSClientApprove( this, &CSpaceWarServer::OnGSClientApprove ),
	m_CallbackGSClientDeny( this, &CSpaceWarServer::OnGSClientDeny ),
	m_CallbackGSClientKick( this, &CSpaceWarServer::OnGSClientKick ),
	m_CallbackSocketStatus( this, &CSpaceWarServer::OnSocketStatusCallback )
{
	m_bConnectedToSteam = false;

	char *pchGameDir = "spacewar";
	uint32 unIP = INADDR_ANY;
	uint16 usSpectatorPort = 0; // We don't support specators in our game
	uint16 usMasterServerUpdaterPort = SPACEWAR_MASTER_SERVER_UPDATER_PORT;

#ifdef USE_GS_AUTH_API
	EServerMode eMode = eServerModeAuthenticationAndSecure;
#else
	// Don't let Steam do authentication
	EServerMode eMode = eServerModeNoAuthentication;
#endif
	// Initialize the SteamGameServer interface, we tell it some info about us, and we request support
	// for both Authentication (making sure users own games) and secure mode, VAC running in our game
	// and kicking users who are VAC banned
	if ( !SteamGameServer_Init( unIP, SPACEWAR_AUTHENTICATION_PORT, SPACEWAR_SERVER_PORT, usSpectatorPort, usMasterServerUpdaterPort, eMode, pchGameDir, SPACEWAR_SERVER_VERSION ) )
	{
		OutputDebugString( "SteamGameServer_Init call failed\n" );
	}

#ifdef USE_GS_AUTH_API
	// We want to actively update the master server with our presence so players can
	// find us via the steam matchmaking/server browser interfaces
	if ( SteamMasterServerUpdater() )
	{
		/*
		// hack to force local master server use for debugging
		while( SteamMasterServerUpdater()->GetNumMasterServers() > 0 )
		{
			char address[256];
			SteamMasterServerUpdater()->GetMasterServerAddress( 0, address, 256 );
		}
		SteamMasterServerUpdater()->AddMasterServer( "127.0.0.1:27011" );
		*/

		SteamMasterServerUpdater()->SetActive( true );
	}
	else
	{
		OutputDebugString( "SteamMasterServerUpdater() interface is invalid\n" );
	}
#endif

	m_uPlayerCount = 0;
	m_pGameEngine = pGameEngine;
	m_eGameState = k_EServerWaitingForPlayers;

	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		m_rguPlayerScores[i] = 0;
		m_rgpShips[i] = NULL;
	}

	// No one has won
	m_uPlayerWhoWonGame = 0;
	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_ulLastServerUpdateTick = 0;

	// zero the client connection data
	memset( &m_rgClientData, 0, sizeof( m_rgClientData ) );
	memset( &m_rgPendingClientData, 0, sizeof( m_rgPendingClientData ) );

	// Seed random num generator
	srand( (uint32)time( NULL ) );

	// Initialize sun
	m_pSun = new CSun( pGameEngine );

	// We don't create our socket to listen for clients yet, because we have to be logged
	// into Steam and have our own game server SteamID before we can really handle them.
	//
	// We'll setup our socket and start listening after that happens.
	m_hSocketServer = NULL;

	// Initialize ships
	ResetPlayerShips();

	// Start listening for client connections, as we now have enough info to really handle them
	m_hSocketServer = SteamGameServerNetworking()->CreateListenSocket( 0, 0, SPACEWAR_SERVER_PORT, true );
	if ( !m_hSocketServer )
	{
		OutputDebugString( "Failed creating socket for CSpaceWarServer -- no one will be able to connect\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpaceWarServer::~CSpaceWarServer()
{
#ifdef USE_GS_AUTH_API
	// Notify Steam master server we are going offline
	SteamMasterServerUpdater()->SetActive( false );
#endif

	delete m_pSun;

	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
		{
			// Tell this client we are exiting
			MsgServerExiting_t msg;
			BSendDataToClient( i, (char*)&msg, sizeof(msg) );

			delete m_rgpShips[i];
			m_rgpShips[i] = NULL;
		}
	}

#ifdef USE_GS_AUTH_API
	// Disconnect from the steam servers
	SteamGameServer()->LogOff();
#endif

	// Close down our socket if it appears to be valid
	if ( m_hSocketServer )
	{
		SteamGameServerNetworking()->DestroyListenSocket( m_hSocketServer, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle sending data to a client at a given index
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnSocketStatusCallback( SocketStatusCallback_t *pCallback )
{
	// make sure the callback involves our listen socket (the callbacks go to all listeners, so you have to filter)
	if ( pCallback->m_hListenSocket != m_hSocketServer )
		return;

	// lookup disconnect users
	if ( pCallback->m_eSNetSocketState >= k_ESNetSocketStateDisconnecting )
	{
		// socket has closed, kick the user associated with it
		for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
		{
			// If there is no ship, skip
			if ( !m_rgClientData[i].m_bActive )
				continue;

			if ( m_rgClientData[i].m_hSocket == pCallback->m_hSocket )
			{
				OutputDebugString( "Disconnected dropped user\n" );
				RemovePlayerFromServer( i );
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle sending data to a client at a given index
//-----------------------------------------------------------------------------
bool CSpaceWarServer::BSendDataToClient( uint32 uShipIndex, char *pData, uint32 nSizeOfData )
{
	// Validate index
	if ( uShipIndex >= MAX_PLAYERS_PER_SERVER )
		return false;

	if ( !SteamGameServerNetworking()->SendDataOnSocket( m_rgClientData[uShipIndex].m_hSocket, pData, nSizeOfData, false ) )
	{
		OutputDebugString( "Failed sending data to a client\n" );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handle sending data to a pending client at a given index
//-----------------------------------------------------------------------------
bool CSpaceWarServer::BSendDataToPendingClient( uint32 uShipIndex, char *pData, uint32 nSizeOfData )
{
	// Validate index
	if ( uShipIndex >= MAX_PLAYERS_PER_SERVER )
		return false;

	if ( !SteamGameServerNetworking()->SendDataOnSocket( m_rgPendingClientData[uShipIndex].m_hSocket, pData, nSizeOfData, false ) )
	{
		OutputDebugString( "Failed sending data to a pending client\n" );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handle sending data to a client at a given address
//-----------------------------------------------------------------------------
bool CSpaceWarServer::BSendDataToClientOnSocket( SNetSocket_t hSocket, char *pData, uint32 nSizeOfData )
{
	if ( !SteamGameServerNetworking()->SendDataOnSocket( hSocket, pData, nSizeOfData, false ) )
	{
		OutputDebugString( "Failed sending data to a client\n" );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handle a new client connecting
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnClientBeginAuthentication( SNetSocket_t hSocketClient, void *pToken, uint32 uTokenLen )
{
	// First, check this isn't a duplicate and we already have a user logged on from the same ip/port combo
	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i ) 
	{
		if ( m_rgClientData[i].m_hSocket == hSocketClient )
		{
			// We already logged them on... (should maybe tell them again incase they don't know?)
			return;
		}
	}

	// Second, do we have room?
	uint32 nPendingOrActivePlayerCount = 0;
	for ( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgPendingClientData[i].m_bActive )
			++nPendingOrActivePlayerCount;
		
		if ( m_rgClientData[i].m_bActive )
			++nPendingOrActivePlayerCount;
	}

	// We are full (or will be if the pending players auth), deny new login
	if ( nPendingOrActivePlayerCount >=  MAX_PLAYERS_PER_SERVER )
	{
		MsgServerFailAuthentication_t msg;
		BSendDataToClientOnSocket( hSocketClient, (char*)&msg, sizeof( msg ) );
	}

	// If we get here there is room, add the player as pending
	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i ) 
	{
		if ( !m_rgPendingClientData[i].m_bActive )
		{
			m_rgPendingClientData[i].m_hSocket = hSocketClient;
			m_rgPendingClientData[i].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();

			// pull the IP address of the user from the socket
			uint32 unIPClient = 0;
			SteamGameServerNetworking()->GetSocketInfo( hSocketClient, NULL, NULL, &unIPClient, NULL );

#ifdef USE_GS_AUTH_API
			// authenticate the user with the Steam back-end servers
			m_rgPendingClientData[i].m_bActive = SteamGameServer()->SendUserConnectAndAuthenticate( unIPClient, pToken, uTokenLen, &m_rgPendingClientData[i].m_SteamIDUser ); 
			if ( !m_rgPendingClientData[i].m_bActive )
			{
				MsgServerFailAuthentication_t msg;
				BSendDataToClientOnSocket( hSocketClient, (char*)&msg, sizeof( msg ) );
			}
			break;
#else
			m_rgPendingClientData[i].m_bActive = true;
			// we need to tell the server our Steam id in the non-auth case, so we stashed it in the login message, pull it back out
			m_rgPendingClientData[i].m_SteamIDUser = *(CSteamID *)pToken;
			// You would typically do your own authentication method here and later call OnAuthCompleted
			// In this sample we just automatically auth anyone who connects
			OnAuthCompleted( true, i );
			break;
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle a new client connecting
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnAuthCompleted( bool bAuthSuccessful, uint32 iPendingAuthIndex )
{
	if ( !m_rgPendingClientData[iPendingAuthIndex].m_bActive )
	{
		OutputDebugString( "Got auth completed callback for client who is not pending\n" );
		return;
	}

	if ( !bAuthSuccessful )
	{
#ifdef USE_GS_AUTH_API
		// Tell the GS the user is leaving the server
		SteamGameServer()->SendUserDisconnect( m_rgPendingClientData[iPendingAuthIndex].m_SteamIDUser );
#endif
		// Send a deny for the client, and zero out the pending data
		MsgServerFailAuthentication_t msg;
		BSendDataToClientOnSocket( m_rgPendingClientData[iPendingAuthIndex].m_hSocket, (char*)&msg, sizeof( msg ) );
		memset( &m_rgPendingClientData[iPendingAuthIndex], 0, sizeof( ClientConnectionData_t ) );
		return;
	}


	bool bAddedOk = false;
	for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i ) 
	{
		if ( !m_rgClientData[i].m_bActive )
		{
			// copy over the data from the pending array
			memcpy( &m_rgClientData[i], &m_rgPendingClientData[iPendingAuthIndex], sizeof( ClientConnectionData_t ) );
			memset( &m_rgPendingClientData[iPendingAuthIndex], 0, sizeof( ClientConnectionData_t	) );
			m_rgClientData[i].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();

			// Add a new ship, make it dead immediately
			AddPlayerShip( i );
			m_rgpShips[i]->SetDisabled( true );

			MsgServerPassAuthentication_t msg;
			msg.m_uPlayerPosition = i;
			BSendDataToClient( i, (char*)&msg, sizeof( msg ) );

			bAddedOk = true;

			break;
		}
	}

	// If we just successfully added the player, check if they are #2 so we can restart the round
	if ( bAddedOk )
	{
		uint32 uPlayers = 0;
		for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i ) 
		{
			if ( m_rgClientData[i].m_bActive )
				++uPlayers;
		}

		// If we just got the second player, immediately reset round as a draw.  This will prevent
		// the existing player getting a win, and it will cause a new round to start right off
		// so that the one player can't just float around not letting the new one get into the game.
		if ( uPlayers == 2 )
		{
			if ( m_eGameState != k_EServerWaitingForPlayers )
				SetGameState( k_EServerDraw );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used to reset scores (at start of a new game usually)
//-----------------------------------------------------------------------------
void CSpaceWarServer::ResetScores()
{
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		m_rguPlayerScores[i] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add a new player ship at given position
//-----------------------------------------------------------------------------
void CSpaceWarServer::AddPlayerShip( uint32 uShipPosition )
{
	if ( uShipPosition >= MAX_PLAYERS_PER_SERVER )
	{
		OutputDebugString( "Trying to add ship at invalid positon\n" );
		return;
	}

	if ( m_rgpShips[uShipPosition] )
	{
		OutputDebugString( "Trying to add a ship where one already exists\n" );
		return;
	}

	float flHeight = (float)m_pGameEngine->GetViewportHeight();
	float flWidth = (float)m_pGameEngine->GetViewportWidth();
	float flXOffset = flWidth*0.12f;
	float flYOffset = flHeight*0.12f;

	float flAngle = atan( flHeight/flWidth ) + PI_VALUE/2.0f;
	switch( uShipPosition )
	{
	case 0:
		m_rgpShips[uShipPosition] = new CShip( m_pGameEngine, true, flXOffset, flYOffset, g_rgPlayerColors[uShipPosition] );
		m_rgpShips[uShipPosition]->SetInitialRotation( flAngle );
		break;
	case 1:
		m_rgpShips[uShipPosition] = new CShip( m_pGameEngine, true, flWidth-flXOffset, flYOffset, g_rgPlayerColors[uShipPosition] );
		m_rgpShips[uShipPosition]->SetInitialRotation( -1.0f*flAngle );
		break;
	case 2:
		m_rgpShips[uShipPosition] = new CShip( m_pGameEngine, true, flXOffset, flHeight-flYOffset, g_rgPlayerColors[uShipPosition] );
		m_rgpShips[uShipPosition]->SetInitialRotation( PI_VALUE-flAngle );
		break;
	case 3:
		m_rgpShips[uShipPosition] = new CShip( m_pGameEngine, true, flWidth-flXOffset, flHeight-flYOffset, g_rgPlayerColors[uShipPosition] );
		m_rgpShips[uShipPosition]->SetInitialRotation( -1.0f*(PI_VALUE-flAngle) );
		break;
	default:
		OutputDebugString( "AddPlayerShip() code needs updating for more than 4 players\n" );
	}

	if ( m_rgpShips[uShipPosition] )
	{
		// Setup key bindings... don't even really need these on server?
		m_rgpShips[uShipPosition]->SetVKBindingLeft( 0x41 ); // A key
		m_rgpShips[uShipPosition]->SetVKBindingRight( 0x44 ); // D key
		m_rgpShips[uShipPosition]->SetVKBindingForwardThrusters( 0x57 ); // W key
		m_rgpShips[uShipPosition]->SetVKBindingReverseThrusters( 0x53 ); // S key
		m_rgpShips[uShipPosition]->SetVKBindingFire( VK_SPACE ); 
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes a player at the given position
//-----------------------------------------------------------------------------
void CSpaceWarServer::RemovePlayerFromServer( uint32 uShipPosition )
{
	if ( uShipPosition >= MAX_PLAYERS_PER_SERVER )
	{
		OutputDebugString( "Trying to remove ship at invalid position\n" );
		return;
	}

	if ( !m_rgpShips[uShipPosition] )
	{
		OutputDebugString( "Trying to remove a ship that does not exist\n" );
		return;
	}

	OutputDebugString( "Removing a ship\n" );
	delete m_rgpShips[uShipPosition];
	m_rgpShips[uShipPosition] = NULL;
	m_rguPlayerScores[uShipPosition] = 0;

#ifdef USE_GS_AUTH_API
	// Tell the GS the user is leaving the server
	SteamGameServer()->SendUserDisconnect( m_rgClientData[uShipPosition].m_SteamIDUser );
#endif
	memset( &m_rgClientData[uShipPosition], 0, sizeof( ClientConnectionData_t ) );
}


//-----------------------------------------------------------------------------
// Purpose: Used to reset player ship positions for a new round
//-----------------------------------------------------------------------------
void CSpaceWarServer::ResetPlayerShips()
{
	// Delete any currently active ships, but immediately recreate 
	// (which causes all ship state/position to reset)
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
		{		
			delete m_rgpShips[i];
			m_rgpShips[i] = NULL;
			AddPlayerShip( i );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used to transition game state
//-----------------------------------------------------------------------------
void CSpaceWarServer::SetGameState( EServerGameState eState )
{
	if ( m_eGameState == eState )
		return;

	// If we were in waiting for players and are now going active clear old scores
	if ( m_eGameState == k_EServerWaitingForPlayers && eState == k_EServerActive )
	{
		ResetScores();
		ResetPlayerShips();
	}

	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_eGameState = eState;
}


//-----------------------------------------------------------------------------
// Purpose: Receives incoming network data
//-----------------------------------------------------------------------------
void CSpaceWarServer::ReceiveNetworkData()
{
	if ( !m_hSocketServer )
		return;

	char *pchRecvBuf = NULL;
	SNetSocket_t hSocketClient;
	uint32 cubMsgSize;
	while ( SteamGameServerNetworking()->IsDataAvailable( m_hSocketServer, &cubMsgSize, &hSocketClient ) )
	{
		// free any previous receive buffer
		if ( pchRecvBuf )
			free( pchRecvBuf );

		// alloc a new receive buffer of the right size
		pchRecvBuf = (char *)malloc( cubMsgSize );

		// see if there is any data waiting on the socket
		if ( !SteamGameServerNetworking()->RetrieveData( m_hSocketServer, pchRecvBuf, cubMsgSize, &cubMsgSize, &hSocketClient ) )
			break;

		if ( cubMsgSize < sizeof( EMessage ) )
		{
			OutputDebugString( "Got garbage on server socket, too short\n" );
			continue;
		}

		EMessage *pEMsg = (EMessage*)pchRecvBuf;
		switch ( *pEMsg )
		{
		case k_EMsgClientInitiateConnection:
			{
				// We always let clients do this without even checking for room on the server since we reserve that for 
				// the authentication phase of the connection which comes next
				MsgServerSendInfo_t msg;
				msg.m_ulSteamIDServer = SteamGameServer()->GetSteamID().ConvertToUint64();
#ifdef USE_GS_AUTH_API
				// You can only make use of VAC when using the Steam authentication system
				msg.m_bIsVACSecure = SteamGameServer()->BSecure();
#endif
				_snprintf( msg.m_rgchServerName, sizeof( msg.m_rgchServerName ), "%s", m_sServerName.c_str() );
				BSendDataToClientOnSocket( hSocketClient, (char*)&msg, sizeof( MsgServerSendInfo_t ) );
			}
			break;
		case k_EMsgClientBeginAuthentication:
			{
				if ( cubMsgSize != sizeof( MsgClientBeginAuthentication_t ) )
				{
					OutputDebugString( "Bad connection attempt msg\n" );
					continue;
				}
				MsgClientBeginAuthentication_t *pMsg = (MsgClientBeginAuthentication_t*)pchRecvBuf;
#ifdef USE_GS_AUTH_API
				OnClientBeginAuthentication( hSocketClient, &pMsg->m_rgchToken, pMsg->m_uTokenLen );
#else
				OnClientBeginAuthentication( hSocketClient, &pMsg->m_SteamID, 0 );
#endif
			}
			break;
		case k_EMsgClientSendLocalUpdate:
			{
				if ( cubMsgSize != sizeof( MsgClientSendLocalUpdate_t ) )
				{
					OutputDebugString( "Bad client update msg\n" );
					continue;
				}

				// Find the connection that should exist for this users address
				bool bFound = false;
				for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
				{
					if ( m_rgClientData[i].m_hSocket == hSocketClient ) 
					{
						bFound = true;
						MsgClientSendLocalUpdate_t *pMsg = (MsgClientSendLocalUpdate_t*)pchRecvBuf;
						OnReceiveClientUpdateData( i, pMsg->m_ClientUpdateData );
						break;
					}
				}
				if ( !bFound )
					OutputDebugString( "Got a client data update, but couldn't find a matching client\n" );
			}
			break;
		case k_EMsgClientLeavingServer:
			if ( cubMsgSize != sizeof( MsgClientLeavingServer_t ) )
			{
				OutputDebugString( "Bad leaving server msg\n" );
				continue;
			}
			// Find the connection that should exist for this users address
			bool bFound = false;
			for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
			{
				if ( m_rgClientData[i].m_hSocket == hSocketClient )
				{
					bFound = true;
					RemovePlayerFromServer( i );
					break;
				}

				// Also check for pending connections that may match
				if ( m_rgPendingClientData[i].m_hSocket == hSocketClient )
				{
#ifdef USE_GS_AUTH_API
					// Tell the GS the user is leaving the server
					SteamGameServer()->SendUserDisconnect( m_rgPendingClientData[i].m_SteamIDUser );
#endif
					// Clear our data on the user
					memset( &m_rgPendingClientData[i], 0 , sizeof( ClientConnectionData_t ) );
					break;
				}
			}
			if ( !bFound )
				OutputDebugString( "Got a client leaving server msg, but couldn't find a matching client\n" );
		}
	}

	if ( pchRecvBuf )
		free( pchRecvBuf );
}

//-----------------------------------------------------------------------------
// Purpose: Main frame function, updates the state of the world and performs rendering
//-----------------------------------------------------------------------------
void CSpaceWarServer::RunFrame()
{
	// Run any Steam Game Server API callbacks
	SteamGameServer_RunCallbacks();

	// Get any new data off the network to begin with
	ReceiveNetworkData();

	// Update our server details
	SendUpdatedServerDetailsToSteam();

	// Timeout stale player connections, also update player count data
	uint32 uPlayerCount = 0;
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		// If there is no ship, skip
		if ( !m_rgClientData[i].m_bActive )
			continue;

		if ( m_pGameEngine->GetGameTickCount() - m_rgClientData[i].m_ulTickCountLastData > SERVER_TIMEOUT_MILLISECONDS )
		{
			OutputDebugString( "Timing out player connection\n" );
			RemovePlayerFromServer( i );
		}
		else
		{
			++uPlayerCount;
		}
	}
	m_uPlayerCount = uPlayerCount;

	switch ( m_eGameState )
	{
	case k_EServerWaitingForPlayers:
		// Wait a few seconds (so everyone can join if a lobby just started this server)
		if ( m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime >= MILLISECONDS_BETWEEN_ROUNDS )
		{
			// Just keep waiting until at least one ship is active
			for( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
			{
				if ( m_rgClientData[i].m_bActive )
				{
					// Transition to active
					OutputDebugString( "Server going active after waiting for players\n" );
					SetGameState( k_EServerActive );
				}
			}
		}
		break;
	case k_EServerDraw:
	case k_EServerWinner:
		// Update all the entities...
		m_pSun->RunFrame();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->RunFrame();
		}

		// NOTE: no collision detection, because the round is really over, objects are now invulnerable

		// After 5 seconds start the next round
		if ( m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime >= MILLISECONDS_BETWEEN_ROUNDS )
		{
			ResetPlayerShips();
			SetGameState( k_EServerActive );
		}

		break;

	case k_EServerActive:
		// Update all the entities...
		m_pSun->RunFrame();
		for( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
		{
			if ( m_rgpShips[i] )
				m_rgpShips[i]->RunFrame();
		}

		// Check for collisions which could lead to a winner this round
		CheckForCollisions();

		break;
	case k_EServerExiting:
		break;
	default:
		OutputDebugString( "Unhandled game state in CSpaceWarServer::RunFrame\n" );
	}

	// Send client updates (will internal limit itself to the tick rate desired)
	SendUpdateDataToAllClients();
}


//-----------------------------------------------------------------------------
// Purpose: Sends updates to all connected clients
//-----------------------------------------------------------------------------
void CSpaceWarServer::SendUpdateDataToAllClients()
{
	// Limit the rate at which we update, even if our internal frame rate is higher
	if ( m_pGameEngine->GetGameTickCount() - m_ulLastServerUpdateTick < 1000.0f/SERVER_UPDATE_SEND_RATE )
		return;

	m_ulLastServerUpdateTick = m_pGameEngine->GetGameTickCount();

	MsgServerUpdateWorld_t msg;

	msg.m_ServerUpdateData.m_eCurrentGameState = m_eGameState;
	for( int i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		msg.m_ServerUpdateData.m_rgPlayersActive[i] = m_rgClientData[i].m_bActive;
		msg.m_ServerUpdateData.m_rgPlayerScores[i] = m_rguPlayerScores[i]; 
		msg.m_ServerUpdateData.m_rgPlayerSteamIDs[i] = m_rgClientData[i].m_SteamIDUser.ConvertToUint64();

		if ( m_rgpShips[i] )
		{
			m_rgpShips[i]->BuildServerUpdate( &msg.m_ServerUpdateData.m_rgShipData[i] );
		}
	}

	msg.m_ServerUpdateData.m_uPlayerWhoWonGame = m_uPlayerWhoWonGame;
	
	for( int i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( !m_rgClientData[i].m_bActive ) 
			continue;

		BSendDataToClient( i, (char*)&msg, sizeof( msg ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Receives update data from clients
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnReceiveClientUpdateData( uint32 uShipIndex, ClientSpaceWarUpdateData_t UpdateData )
{
	if ( m_rgClientData[uShipIndex].m_bActive && m_rgpShips[uShipIndex] )
	{
		m_rgClientData[uShipIndex].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();
		m_rgpShips[uShipIndex]->OnReceiveClientUpdate( UpdateData );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks various game objects for collisions and updates state 
//			appropriately if they have occurred
//-----------------------------------------------------------------------------
void CSpaceWarServer::CheckForCollisions()
{
	// Make the ships check their photons for ones that have hit the sun and remove
	// them before we go and check for them hitting the opponent
	for ( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
			m_rgpShips[i]->DestroyPhotonsColldingWith( m_pSun );
	}

	// Array to track who exploded, can't set the ship exploding within the loop below,
	// or it will prevent that ship from colliding with later ships in the sequence
	bool rgbExplodingShips[MAX_PLAYERS_PER_SERVER];
	memset( rgbExplodingShips, 0, sizeof( rgbExplodingShips ) );

	// Check each ship for colliding with the sun or another ships photons
	for ( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		// If the pointer is invalid skip the ship
		if ( !m_rgpShips[i] )
			continue;

		rgbExplodingShips[i] |= m_rgpShips[i]->BCollidesWith( m_pSun );

		for( uint32 j=0; j<MAX_PLAYERS_PER_SERVER; ++j )
		{
			// Don't check against your own photons, or NULL pointers!
			if ( j == i || !m_rgpShips[j] )
				continue;
			
			rgbExplodingShips[i] |= m_rgpShips[i]->BCollidesWith( m_rgpShips[j] );
			rgbExplodingShips[i] |= m_rgpShips[j]->BCheckForPhotonsCollidingWith( m_rgpShips[i] );
		}
	}

	for ( uint32 i=0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( rgbExplodingShips[i] && m_rgpShips[i] )
			m_rgpShips[i]->SetExploding( true );
	}

	// Count how many ships are active, and how many are exploding
	uint32 uActiveShips = 0;
	uint32 uShipsExploding = 0;
	uint32 uLastShipFoundAlive = 0;
	for ( uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgpShips[i] )
		{
			// Disabled ships don't count at all
			if ( m_rgpShips[i]->BIsDisabled() )
				continue;

			++uActiveShips;
		
			if ( m_rgpShips[i]->BIsExploding() )
				++uShipsExploding;
			else
				uLastShipFoundAlive = i;
		}
	}

	// If exploding == active, then its a draw, everyone is dead
	if ( uActiveShips == uShipsExploding )
	{
		SetGameState( k_EServerDraw );
	}
	else if ( uActiveShips > 1 && uActiveShips - uShipsExploding == 1 )
	{
		// If only one ship is alive they win
		m_uPlayerWhoWonGame = uLastShipFoundAlive;
		m_rguPlayerScores[uLastShipFoundAlive]++;
		SetGameState( k_EServerWinner );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Take any action we need to on Steam notifying us we are now logged in
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnSteamServersConnected( SteamServersConnected_t *pLogonSuccess )
{
	OutputDebugString( "SpaceWarServer connected to Steam successfully\n" );
	m_bConnectedToSteam = true;

	// log on is not finished until OnPolicyResponse() is called

	// Tell Steam about our server details
	SendUpdatedServerDetailsToSteam();
}


//-----------------------------------------------------------------------------
// Purpose: Callback from Steam3 when logon is fully completed and VAC secure policy is set
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnPolicyResponse( GSPolicyResponse_t *pPolicyResponse )
{
#ifdef USE_GS_AUTH_API
	// Check if we were able to go VAC secure or not
	if ( SteamGameServer()->BSecure() )
	{
		OutputDebugString( "SpaceWarServer is VAC Secure!\n" );
	}
	else
	{
		OutputDebugString( "SpaceWarServer is not VAC Secure!\n" );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Called when we were previously logged into steam but get logged out
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnSteamServersDisconnected( SteamServersDisconnected_t *pLoggedOff )
{
	m_bConnectedToSteam = false;
	OutputDebugString( "SpaceWarServer got logged out of Steam\n" );
}


//-----------------------------------------------------------------------------
// Purpose: Called once we are connected to Steam to tell it about our details
//-----------------------------------------------------------------------------
void CSpaceWarServer::SendUpdatedServerDetailsToSteam()
{
	char *pchGameDir = "spacewar";

#ifdef USE_GS_AUTH_API
	// Tell the Steam Master Server about our game
	SteamMasterServerUpdater()->SetBasicServerData( 
		7,		// Protocol version to talk to the master server with, use the magic number 7
		true,	// Are we a dedicated server? Lie and say yes for our game since we don't have dedicated servers.
		"255",  // Region name is unused, use the magic value "255" always
		"SteamworksExample", // Our game name
		4,		// Maximum number of players for our server
		false,	// Does our server require a password?
		"Steamworks Example" // Description of our game
	);
#endif

	// Tell the Steam authentication servers about our game
	char rgchServerName[128];
	if ( SpaceWarClient() )
	{
		// If a client is running (should always be since we don't support a dedicated server)
		// then we'll form the name based off of it
		_snprintf( rgchServerName, ARRAYSIZE( rgchServerName ), "%s's game", SpaceWarClient()->GetLocalPlayerName() );
	}
	else
	{
		_snprintf( rgchServerName, ARRAYSIZE( rgchServerName ), "%s", "Spacewar!" );
	}
	m_sServerName = rgchServerName;

#ifdef USE_GS_AUTH_API
	SteamGameServer()->UpdateServerStatus(  
		m_uPlayerCount,				// Current player count
		4,							// Maximum number of players
		0,							// Number of AI/Bot players
		m_sServerName.c_str(),		// Server name
		"",							// Spectator server name, unused for us
		"MilkyWay"					// Map name
		);

	// Update all the players names/scores
	for( uint32 i=0; i < MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( m_rgClientData[i].m_bActive && m_rgpShips[i] )
		{
			SteamGameServer()->BUpdateUserData( m_rgClientData[i].m_SteamIDUser, m_rgpShips[i]->GetPlayerName(), m_rguPlayerScores[i] );
		}
	}
#endif

	// game type is a special string you can use for your game to differentiate different game play types occurring on the same maps
	// When users search for this parameter they do a sub-string search of this string 
	// (i.e if you report "abc" and a client requests "ab" they return your server)
	//SteamGameServer()->SetGameType( "dm" );

	// update any rule values we publish
	//SteamMasterServerUpdater()->SetKeyValue( "rule1_setting", "value" );
	//SteamMasterServerUpdater()->SetKeyValue( "rule2_setting", "value2" );
}


//-----------------------------------------------------------------------------
// Purpose: Tells us Steam3 (VAC and newer license checking) has accepted the user connection
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnGSClientApprove( GSClientApprove_t *pGSClientApprove )
{
	// This is the final approval, and means we should let the client play (find the pending auth by steamid)
	for ( uint32 i = 0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( !m_rgPendingClientData[i].m_bActive )
			continue;
		else if ( m_rgPendingClientData[i].m_SteamIDUser == pGSClientApprove->m_SteamID )
		{
			OutputDebugString( "Auth completed for a client\n" );
			OnAuthCompleted( true, i );
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Callback indicating that Steam has told us to deny a user connection
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnGSClientDeny( GSClientDeny_t *pGSClientDeny )
{
	// Looks like we shouldn't let this user play, kick them
	for ( uint32 i = 0; i<MAX_PLAYERS_PER_SERVER; ++i )
	{
		if ( !m_rgPendingClientData[i].m_bActive )
			continue;
		else if ( m_rgPendingClientData[i].m_SteamIDUser == pGSClientDeny->m_SteamID )
		{
			switch( pGSClientDeny->m_eDenyReason )
			{
			case k_EDenySteamValidationStalled:
				// k_EDenySteamValidationStalled means we couldn't get a timely response from Steam 
				// as to whether this client should really be allowed to play, but we did at least validate
				// that they are who they say they are... As such, we'll decide that we should let them play
				// as this may just mean the Steam auth servers are down.
				OutputDebugString( "Auth stalled for a client, let them play anyway\n" );
				OnAuthCompleted( true, i );
				break;
			default:
				OnAuthCompleted( false, i );
				break;
			}
			
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Steam is telling us to kick a client (they probably lost connection to 
// Steam and now VAC can't verify them...)
//-----------------------------------------------------------------------------
void CSpaceWarServer::OnGSClientKick( GSClientKick_t *pGSClientKick )
{
	char buffer[128];
	_snprintf( buffer, 128, "Steam told us to kick: %u - %u\n", CSteamID( pGSClientKick->m_SteamID ).GetAccountID(), pGSClientKick->m_eDenyReason );
	OutputDebugString( buffer );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the public IP address of the game server
//-----------------------------------------------------------------------------
uint32 CSpaceWarServer::GetIP()
{
	if ( m_hSocketServer )
	{
		uint32 unIP;
		uint16 unPort;
		SteamGameServerNetworking()->GetListenSocketInfo( m_hSocketServer, &unIP, &unPort );
		return unIP;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the public port of the game server
//-----------------------------------------------------------------------------
uint16 CSpaceWarServer::GetPort()
{
	if ( m_hSocketServer )
	{
		uint32 unIP;
		uint16 unPort;
		SteamGameServerNetworking()->GetListenSocketInfo( m_hSocketServer, &unIP, &unPort );
		return unPort;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the SteamID of the game server
//-----------------------------------------------------------------------------
CSteamID CSpaceWarServer::GetSteamID()
{
#ifdef USE_GS_AUTH_API
	return SteamGameServer()->GetSteamID();
#else
	// this is a placeholder steam id to use when not making use of Steam auth or matchmaking
	return k_steamIDNonSteamGS;
#endif
}
