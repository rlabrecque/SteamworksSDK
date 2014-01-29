//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the wire protocol for the game
//
// $NoKeywords: $
//=============================================================================

#ifndef MESSAGES_H
#define MESSAGES_H

#include <map>

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

	// Client messages
	k_EMsgClientBegin = 500,
	k_EMsgClientInitiateConnection = k_EMsgClientBegin+1,
	k_EMsgClientBeginAuthentication = k_EMsgClientBegin+2,
	k_EMsgClientSendLocalUpdate = k_EMsgClientBegin+3,
	k_EMsgClientLeavingServer = k_EMsgClientBegin+4,


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

	uint64 m_ulSteamID;
	
	uint32 m_uTokenLen;
	char m_rgchToken[1024];
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


#endif // MESSAGES_H