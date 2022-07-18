//========= Copyright © Valve LLC, All rights reserved. ============
//
// Purpose: Class that shows some examples for bringing up the Steam Overlay
//
//=============================================================================

#ifndef OVERLAYEXAMPLES_H
#define OVERLAYEXAMPLES_H

#include "SpaceWar.h"
#include "GameEngine.h"
#include "SpaceWarClient.h"


class CSpaceWarClient;
class COverlayExamplesMenu;

class COverlayExamples
{
public:
	// Constructor
	COverlayExamples( IGameEngine *pGameEngine );

	// Run a frame
	void RunFrame();

	// shows / refreshes item store
	void Show();

	// handles input from menu
	void OnMenuSelection( OverlayExample_t selection );
	
private:	
	
	// Engine
	IGameEngine *m_pGameEngine;
	
	COverlayExamplesMenu *m_pMenu;
	OverlayExample_t m_delayedCommand;
};

#endif // OVERLAYEXAMPLES_H
