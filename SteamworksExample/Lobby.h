//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Class for handling finding & creating lobbies, getting their details, 
//			and seeing other users in the current lobby
//
//=============================================================================

#ifndef LOBBY_H
#define LOBBY_H

#include "SpaceWar.h"
#include "GameEngine.h"
#include "steam/isteammatchmaking.h"
#include "BaseMenu.h"
#include <list>

class CSpaceWarClient;
class CLobbyBrowserMenu;
class CLobbyMenu;

//-----------------------------------------------------------------------------
// Purpose: Displays the other users in a lobby and allows the game to be started
//-----------------------------------------------------------------------------
class CLobby
{
public:
	CLobby( CGameEngine *pGameEngine );
	~CLobby();

	// sets which lobby to display
	void SetLobbySteamID( const CSteamID &steamIDLobby );

	// Run a frame (to handle KB input and such as well as render)
	void RunFrame();

private:
	CSteamID m_steamIDLobby;

	// Pointer to engine instance (so we can draw stuff)
	CGameEngine *m_pGameEngine;

	// Menu object
	CLobbyMenu *m_pMenu;

	// user state change handler
	STEAM_CALLBACK( CLobby, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange );

	// lobby state change handler
	STEAM_CALLBACK( CLobby, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate );
	STEAM_CALLBACK( CLobby, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackChatDataUpdate );
};


// an item in the list of lobbies we've found to display
struct Lobby_t
{
	CSteamID m_steamIDLobby;
	char m_rgchName[256];
};


//-----------------------------------------------------------------------------
// Purpose: Displaying and allows selection from a list of lobbies
//-----------------------------------------------------------------------------
class CLobbyBrowser
{
public:
	CLobbyBrowser( CGameEngine *pGameEngine );
	~CLobbyBrowser();

	// rebuild the list
	void Refresh();

	// Run a frame (to handle KB input and such as well as render)
	void RunFrame();

private:
	// Pointer to engine instance (so we can draw stuff)
	CGameEngine *m_pGameEngine;

	// Track whether we are in the middle of a refresh or not
	bool m_bRequestingLobbies;

	// Menu object
	CLobbyBrowserMenu *m_pMenu;

	STEAM_CALLBACK( CLobbyBrowser, OnLobbyMatchListCallback, LobbyMatchList_t, m_CallbackLobbyMatchList );
	STEAM_CALLBACK( CLobbyBrowser, OnLobbyDataUpdatedCallback, LobbyDataUpdate_t, m_CallbackLobbyDataUpdated );

	std::list< Lobby_t > m_ListLobbies; 
};


#endif //LOBBY_H