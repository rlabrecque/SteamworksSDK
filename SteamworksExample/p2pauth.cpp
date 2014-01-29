//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================


#include "stdafx.h"
#include "SpaceWarClient.h"
#include "p2pauth.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CNetworkTransport::CNetworkTransport() : 
	m_CallbackP2PSessionRequest( this, &CNetworkTransport::OnP2PSessionRequest ),
	m_CallbackP2PSessionConnectFail( this, &CNetworkTransport::OnP2PSessionConnectFail )
{
}


//-----------------------------------------------------------------------------
// Purpose: send ticket message to steamIDTo
//-----------------------------------------------------------------------------
bool CNetworkTransport::SendTicket( CSteamID steamIDFrom, CSteamID steamIDTo, uint32 cubTicket, uint8 *pubTicket )
{
	MsgP2PSendingTicket_t msg;
	msg.m_uTokenLen = cubTicket;
	memcpy( msg.m_rgchToken, pubTicket, cubTicket );
	msg.m_SteamID = steamIDFrom;
	return SendNetMessage( steamIDTo, (void*)&msg, sizeof( msg ) );
}


//-----------------------------------------------------------------------------
// Purpose: actual implementation
//-----------------------------------------------------------------------------
bool CNetworkTransport::SendNetMessage( CSteamID steamIDTo, void *pMessage, int cubMessage )
{
	return SteamNetworking()->SendP2PPacket( steamIDTo, pMessage, cubMessage, k_EP2PSendReliable );
}


//-----------------------------------------------------------------------------
// Purpose: close connection to steamID
//-----------------------------------------------------------------------------
void CNetworkTransport::CloseConnection( CSteamID steamID )
{
	SteamNetworking()->CloseP2PSessionWithUser( steamID );
}


//-----------------------------------------------------------------------------
// Purpose: another user has sent us a packet - do we accept?
//-----------------------------------------------------------------------------
void CNetworkTransport::OnP2PSessionRequest( P2PSessionRequest_t *pP2PSessionRequest )
{
	// always accept packets
	// the packet itself will come through when you call SteamNetworking()->ReadP2PPacket()
	SteamNetworking()->AcceptP2PSessionWithUser( pP2PSessionRequest->m_steamIDRemote );
}


//-----------------------------------------------------------------------------
// Purpose: another user has sent us a packet - do we accept?
//-----------------------------------------------------------------------------
void CNetworkTransport::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pP2PSessionConnectFail )
{
	// we've sent a packet to the user, but it never got through
	// we can just use the normal timeout
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CP2PAuthPlayer::CP2PAuthPlayer( CGameEngine *pGameEngine, CNetworkTransport *pNetworkTransport ):m_CallbackBeginAuthResponse( this, &CP2PAuthPlayer::OnBeginAuthResponse )
{
	m_pGameEngine = pGameEngine;
	m_pNetworkTransport = pNetworkTransport;
	m_bSentTicket = false;
	m_bSubmittedHisTicket = false;
	m_bHaveAnswer = false;
	m_ulConnectTime = GetGameTimeInSeconds();
	m_cubTicketIGaveThisUser = 0;
	m_cubTicketHeGaveMe = 0;
}


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CP2PAuthPlayer::~CP2PAuthPlayer()
{
	EndGame();
	m_pNetworkTransport->CloseConnection( SteamUser()->GetSteamID() );
	m_pNetworkTransport = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: the steam backend has responded
//-----------------------------------------------------------------------------
void CP2PAuthPlayer::OnBeginAuthResponse( ValidateAuthTicketResponse_t *pCallback )
{
	if ( m_steamID == pCallback->m_SteamID )
	{
		char rgch[128];
		sprintf( rgch, "P2P:: Received steam response for account=%d\n", m_steamID.GetAccountID() );
		OutputDebugStringA( rgch );
		m_ulAnswerTime = GetGameTimeInSeconds();
		m_bHaveAnswer = true;
		m_eAuthSessionResponse = pCallback->m_eAuthSessionResponse;
	}
}


//-----------------------------------------------------------------------------
// Purpose: internal implementation
//-----------------------------------------------------------------------------
void CP2PAuthPlayer::InitPlayer( CSteamID steamID )
{
	m_steamID = steamID;
	m_bSentTicket = false;
	m_bSubmittedHisTicket = false;
	m_bHaveAnswer = false;
	m_ulConnectTime = GetGameTimeInSeconds();
	m_cubTicketIGaveThisUser = 0;
	m_cubTicketHeGaveMe = 0;
}

void CP2PAuthPlayer::StartAuthPlayer()
{
	// send him the ticket if we havent yet
	if ( m_cubTicketIGaveThisUser == 0 )
	{
		m_hAuthTicketIGaveThisUser = SteamUser()->GetAuthSessionTicket( m_rgubTicketIGaveThisUser, sizeof( m_rgubTicketIGaveThisUser ), &m_cubTicketIGaveThisUser );
	}
	// sample has no retry here
	m_bSentTicket = m_pNetworkTransport->SendTicket( SteamUser()->GetSteamID(), m_steamID, m_cubTicketIGaveThisUser, m_rgubTicketIGaveThisUser );
	// start a timer on this, if we dont get a ticket back within reasonable time, mark him timed out
	m_ulTicketTime = GetGameTimeInSeconds();
}


//-----------------------------------------------------------------------------
// Purpose: is this auth ok?
//-----------------------------------------------------------------------------
bool CP2PAuthPlayer::BIsAuthOk()
{
	if ( m_steamID.IsValid() )
	{
		// Timeout if we fail to establish communication with this player
		if ( !m_bSentTicket && !m_bSubmittedHisTicket )
		{
			if ( GetGameTimeInSeconds() - m_ulConnectTime > 30  )
			{
				char rgch[128];
				sprintf( rgch, "P2P:: Nothing received for account=%d\n", m_steamID.GetAccountID() );
				OutputDebugStringA( rgch );
				return false;
			}
		}
		// first ticket check: if i submitted his ticket - was it good?
		if ( m_bSubmittedHisTicket && m_eBeginAuthSessionResult != k_EBeginAuthSessionResultOK )
		{
			char rgch[128];
			sprintf( rgch, "P2P:: Ticket from account=%d was bad\n", m_steamID.GetAccountID() );
			OutputDebugStringA( rgch );
			return false;
		}
		// second ticket check: if the steam backend replied, was that good?
		if ( m_bHaveAnswer && m_eAuthSessionResponse != k_EAuthSessionResponseOK )
		{
			char rgch[128];
			sprintf( rgch, "P2P:: Steam response for account=%d was bad\n", m_steamID.GetAccountID() );
			OutputDebugStringA( rgch );
			return false;
		}
		// last: if i sent him a ticket and he has not reciprocated, time out after 30 sec
		if ( m_bSentTicket && !m_bSubmittedHisTicket )
		{
			if ( GetGameTimeInSeconds() - m_ulTicketTime > 30  )
			{
				char rgch[128];
				sprintf( rgch, "P2P:: No ticket received for account=%d\n", m_steamID.GetAccountID() );
				OutputDebugStringA( rgch );
				return false;
			}
		}
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: the game engine is telling us about someone who left the game
//-----------------------------------------------------------------------------
void CP2PAuthPlayer::EndGame()
{
	if ( m_bSentTicket )
	{
		SteamUser()->CancelAuthTicket( m_hAuthTicketIGaveThisUser );
		m_bSentTicket = false;
	}
	if ( m_bSubmittedHisTicket )
	{
		SteamUser()->EndAuthSession( m_steamID );
		m_bSubmittedHisTicket = false;
	}
	if ( m_steamID.IsValid() )
		m_pNetworkTransport->CloseConnection( m_steamID );
}


//-----------------------------------------------------------------------------
// Purpose: message handler
//-----------------------------------------------------------------------------
bool CP2PAuthPlayer::HandleMessage( EMessage eMsg, void *pMessage )
{
	switch ( eMsg )
	{
		// message from another player providing his ticket
	case k_EMsgP2PSendingTicket:
		{
			MsgP2PSendingTicket_t *pMsg = (MsgP2PSendingTicket_t*)pMessage;
			// is this message for me?
			if ( pMsg->m_SteamID != m_steamID )
				return false;
			m_cubTicketHeGaveMe = pMsg->m_uTokenLen;
			memcpy( m_rgubTicketHeGaveMe, pMsg->m_rgchToken, m_cubTicketHeGaveMe );
			m_eBeginAuthSessionResult = SteamUser()->BeginAuthSession( m_rgubTicketHeGaveMe, m_cubTicketHeGaveMe, m_steamID );
			m_bSubmittedHisTicket = true;
			char rgch[128];
			sprintf( rgch, "P2P:: ReceivedTicket from account=%d \n", m_steamID.GetAccountID() );
			OutputDebugStringA( rgch );
			if ( !m_bSentTicket )
				StartAuthPlayer();
			return true;
		}
	default:
		{
			OutputDebugStringA( "P2P:: Received unknown message on our listen socket\n" );
			break;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: utility wrapper
//-----------------------------------------------------------------------------
CSteamID CP2PAuthPlayer::GetSteamID()
{
	return SteamUser()->GetSteamID();
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CP2PAuthedGame::CP2PAuthedGame( CGameEngine *pGameEngine )
{
	m_pNetworkTransport = new CNetworkTransport;
	m_pGameEngine = pGameEngine;
	// no players yet
	for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
	{
		m_rgpP2PAuthPlayer[i] = NULL;
	}
	// no queued messages
	for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
	{
		m_rgpQueuedMessage[i] = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: game with this player is over
//-----------------------------------------------------------------------------
void CP2PAuthedGame::PlayerDisconnect( int iSlot )
{
	if ( m_rgpP2PAuthPlayer[iSlot] )
	{
		m_rgpP2PAuthPlayer[iSlot]->EndGame();
		delete m_rgpP2PAuthPlayer[iSlot];
		m_rgpP2PAuthPlayer[iSlot] = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: game is over, disconnect everyone
//-----------------------------------------------------------------------------
void CP2PAuthedGame::EndGame()
{
	for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
	{
		if ( m_rgpP2PAuthPlayer[i] )
		{
			m_rgpP2PAuthPlayer[i]->EndGame();
			delete m_rgpP2PAuthPlayer[i];
			m_rgpP2PAuthPlayer[i] = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: initialize player
//-----------------------------------------------------------------------------
void CP2PAuthedGame::InternalInitPlayer( int iSlot, CSteamID steamID, bool bStartAuthProcess )
{
	char rgch[128];
	sprintf( rgch, "P2P:: StartAuthPlayer slot=%d account=%d \n", iSlot, steamID.GetAccountID() );
	OutputDebugStringA( rgch );
	m_rgpP2PAuthPlayer[iSlot] = new CP2PAuthPlayer( m_pGameEngine, m_pNetworkTransport );
	m_rgpP2PAuthPlayer[iSlot]->InitPlayer( steamID );
	if ( bStartAuthProcess )
		m_rgpP2PAuthPlayer[iSlot]->StartAuthPlayer();

	// check our queued messages list to see if this guys ticket message arrived before we noticed him
	for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
	{
		if ( m_rgpQueuedMessage[i] != NULL )
		{
			if ( m_rgpP2PAuthPlayer[i]->HandleMessage( k_EMsgP2PSendingTicket, m_rgpQueuedMessage[i] ) )
			{
				OutputDebugStringA( "P2P:: Consumed queued message \n" );
				free( m_rgpQueuedMessage[i] );
				m_rgpQueuedMessage[i] = NULL;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: game host register this player, we wait for this player
//			to start the auth process by sending us his ticket, then we will
//			reciprocate
//-----------------------------------------------------------------------------
void CP2PAuthedGame::RegisterPlayer( int iSlot, CSteamID steamID )
{
	if ( iSlot < MAX_PLAYERS_PER_SERVER )
		InternalInitPlayer( iSlot, steamID, false );
}


//-----------------------------------------------------------------------------
// Purpose: start the auth process by sending ticket to this player
//			he will reciprocate
//-----------------------------------------------------------------------------
void CP2PAuthedGame::StartAuthPlayer( int iSlot, CSteamID steamID )
{
	if ( iSlot < MAX_PLAYERS_PER_SERVER )
		InternalInitPlayer( iSlot, steamID, true );
}


//-----------------------------------------------------------------------------
// Purpose: message handler
//-----------------------------------------------------------------------------
bool CP2PAuthedGame::HandleMessage( EMessage eMsg, void *pMessage )
{
	switch ( eMsg )
	{
		// message from another player providing his ticket
	case k_EMsgP2PSendingTicket:
		{
			MsgP2PSendingTicket_t *pMsg = (MsgP2PSendingTicket_t*)pMessage;
			for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				if ( m_rgpP2PAuthPlayer[i] )
				{
					if ( m_rgpP2PAuthPlayer[i]->HandleMessage( eMsg, pMessage ) )
					{
						return true;
					}
				}
			}
			// if we dont have the player in our list yet, lets queue the message and assume he will show up soon
			OutputDebugStringA( "P2P:: HandleMessage queueing message \n" );
			for ( int i = 0; i < MAX_PLAYERS_PER_SERVER; i++ )
			{
				if ( m_rgpQueuedMessage[i] == NULL )
				{
					 m_rgpQueuedMessage[i] = pMsg;
					 return false;
				}
			}
			break;
		}
	default:
		{
			OutputDebugStringA( "Received unknown message on our listen socket\n" );
			break;
		}
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: utility wrapper
//-----------------------------------------------------------------------------
CSteamID CP2PAuthedGame::GetSteamID()
{
	return SteamUser()->GetSteamID();
}