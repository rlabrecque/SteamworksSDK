//====== Copyright Valve Corporation, All rights reserved. ====================
//
// Purpose: Internal implementation details for SteamGameServer
//
//=============================================================================

#ifndef STEAM_GAMESERVER_INTERNAL_H
#define STEAM_GAMESERVER_INTERNAL_H
#pragma once

#ifndef STEAM_GAMESERVER_H
#error "This file should only be included from steam_gameserver.h"
#endif

S_API bool S_CALLTYPE SteamInternal_GameServer_Init( uint32 unIP, uint16 usLegacySteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );

inline bool SteamGameServer_Init( uint32 unIP, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString )
{
	if ( !SteamInternal_GameServer_Init( unIP, 0, usGamePort, usQueryPort, eServerMode, pchVersionString ) )
		return false;

	return true;
}

inline void SteamGameServer_ReleaseCurrentThreadMemory()
{
	SteamAPI_ReleaseCurrentThreadMemory();
}

#endif // #ifndef STEAM_GAMESERVER_INTERNAL_H
