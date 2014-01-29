//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Class for rendering the starfield
//
// $NoKeywords: $
//=============================================================================

#ifndef STARFIELD_H
#define STARFIELD_H

#include <vector>
#include "GameEngine.h"

#define STARFIELD_STAR_COUNT 600

struct StarVertex_t
{
	float x, y;
	DWORD color;
};

class CStarField
{
public:
	// Constructor
	CStarField( CGameEngine *pGameEngine );

	// Render the star field
	void Render();

private:

	// Game engine instance we are running under
	CGameEngine *m_pGameEngine;

	// Vector for starfield data
	std::vector<StarVertex_t> m_VecStars;
};

#endif // STARFIELD_H