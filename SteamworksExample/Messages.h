//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the wire protocol for the game
//
// $NoKeywords: $
//=============================================================================

#ifndef MESSAGES_H
#define MESSAGES_H

#include <map>

// Would normally set this to 1 byte, however we use 8 byte to support older clients that didn't specify packing
#pragma pack( push, 8 )

// Network message types
enum EMessage
{
	// Server messages
	k_EMsgServerBegin = 0,
	k_EMsgServerSendInfo = k_EMsgServerBegin+1,
	k_EMsgServerFailAuthentication = k_EMsgServerBegin+2,
	k_EMsgServerPassAuthentication = k_EMsgServerBegin+3,
	k_EMsgServerUpdateWorld = k_EMsgServerBegin+4,
	k_EMsgServerExiting = k_EMsgServerBegin+5,
	k_EMsgServerPingResponse = k_EMsgServerBegin+6,

	// Client messages
	k_EMsgClientBegin = 500,
	k_EMsgClientInitiateConnection = k_EMsgClientBegin+1,
	k_EMsgClientBeginAuthentication = k_EMsgClientBegin+2,
	k_EMsgClientSendLocalUpdate = k_EMsgClientBegin+3,
	k_EMsgClientLeavingServer = k_EMsgClientBegin+4,
	k_EMsgClientPing = k_EMsgClientBegin+5,

	// P2P messages
	k_EMsgP2PBegin = 500,
	k_EMsgP2PSendingTicket = k_EMsgP2PBegin+1,


	// force 32-bit size enum so the wire protocol doesn't get outgrown later
	k_EForceDWORD  = 0x7fffffff, 
};


// Msg from the server to the client which is sent right after communications are established
// and tells the client what SteamID the game server is using as well as whether the server is secure
struct MsgServerSendInfo_t
{
	MsgServerSendInfo_t() : m_dwMessageType( k_EMsgServerSendInfo ) {}
	const DWORD m_dwMessageType;

	uint64 m_ulSteamIDServer;
	bool m_bIsVACSecure;
	char m_rgchServerName[128];
};

// Msg from the server to the client when refusing a connection
struct MsgServerFailAuthentication_t
{
	MsgServerFailAuthentication_t() : m_dwMessageType( k_EMsgServerFailAuthentication ) {}
	const DWORD m_dwMessageType;
};

// Msg from the server to client when accepting a pending connection
struct MsgServerPassAuthentication_t
{
	MsgServerPassAuthentication_t() : m_dwMessageType( k_EMsgServerPassAuthentication ) {}
	const DWORD m_dwMessageType;

	uint32 m_uPlayerPosition;
};

// Msg from the server to clients when updating the world state
struct MsgServerUpdateWorld_t
{
	MsgServerUpdateWorld_t() : m_dwMessageType( k_EMsgServerUpdateWorld ) {}
	const DWORD m_dwMessageType;

	ServerSpaceWarUpdateData_t m_ServerUpdateData;
};

// Msg from server to clients when it is exiting
struct MsgServerExiting_t
{
	MsgServerExiting_t() : m_dwMessageType( k_EMsgServerExiting ) {}
	const DWORD m_dwMessageType;
};

// Msg from server to clients when it is exiting
struct MsgServerPingResponse_t
{
	MsgServerPingResponse_t() : m_dwMessageType( k_EMsgServerPingResponse ) {}
	const DWORD m_dwMessageType;
};

// Msg from client to server when trying to connect
struct MsgClientInitiateConnection_t
{
	MsgClientInitiateConnection_t() : m_dwMessageType( k_EMsgClientInitiateConnection ) {}
	const DWORD m_dwMessageType;
};

// Msg from client to server when initiating authentication
struct MsgClientBeginAuthentication_t
{
	MsgClientBeginAuthentication_t() : m_dwMessageType( k_EMsgClientBeginAuthentication ) {}
	const DWORD m_dwMessageType;
	
	uint32 m_uTokenLen;
#ifdef USE_GS_AUTH_API
	char m_rgchToken[1024];
#endif
	CSteamID m_SteamID;
};

// Msg from client to server when sending state update
struct MsgClientSendLocalUpdate_t
{
	MsgClientSendLocalUpdate_t() : m_dwMessageType( k_EMsgClientSendLocalUpdate ) {}
	const DWORD m_dwMessageType;

	uint32 m_uShipPosition;
	ClientSpaceWarUpdateData_t m_ClientUpdateData;
};

// Msg from the client telling the server it is about to leave
struct MsgClientLeavingServer_t
{
	MsgClientLeavingServer_t() : m_dwMessageType( k_EMsgClientLeavingServer ) {}
	const DWORD m_dwMessageType;
};

// server ping
struct MsgClientPing_t
{
	MsgClientPing_t() : m_dwMessageType( k_EMsgClientPing ) {}
	const DWORD m_dwMessageType;
};

// Msg from client to server when trying to connect
struct MsgP2PSendingTicket_t
{
	MsgP2PSendingTicket_t() : m_dwMessageType( k_EMsgP2PSendingTicket ) {}
	const DWORD m_dwMessageType;
	uint32 m_uTokenLen;
	char m_rgchToken[1024];
	CSteamID m_SteamID;
};

#pragma pack( pop )

#endif // MESSAGES_H