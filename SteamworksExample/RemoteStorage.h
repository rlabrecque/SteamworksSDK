//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Class for tracking stats and achievements
//
// $NoKeywords: $
//=============================================================================

#ifndef REMOTE_STORAGE_H
#define REMOTE_STORAGE_H

#include "SpaceWar.h"
#include "GameEngine.h"

class ISteamUser;
class CSpaceWarClient;

class CRemoteStorage
{
public:

	// Constructor
	CRemoteStorage( IGameEngine *pGameEngine );

	// Display the remote storage screen
	void Render();

	// Done showing this page?
	bool BFinished() { return m_bFinished; }
	

private:

	void GetFileStats();

	// Engine
	IGameEngine *m_pGameEngine;

	// Display font
	HGAMEFONT m_hDisplayFont;

	// Steam User interface
	ISteamUser *m_pSteamUser;

	// Steam RemoteStorage interface
	ISteamRemoteStorage *m_pSteamRemoteStorage;

	// Greeting message
	char m_rgchGreeting[40];
	char m_rgchGreetingNext[40];

	bool m_bFinished;

	int32 m_nNumFilesInCloud;
	int32 m_nBytesQuota;
	int32 m_nAvailableBytes;
};

#endif
