//========= Copyright � 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Class for rendering the starfield
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "StarField.h"
#include "stdlib.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CStarField::CStarField( IGameEngine *pGameEngine )
{
	m_pGameEngine = pGameEngine;

	Init();
}

void CStarField::Init()
{
	StarVertex_t StarVertex;
	m_nWidth = m_pGameEngine->GetViewportWidth();
	m_nHeight = m_pGameEngine->GetViewportHeight();

	m_VecStars.clear();
	
	// Generate star field data
	for( int i=0; i < STARFIELD_STAR_COUNT; ++i )
	{
		int32 nRand = (rand()%(255-50))+50; //value between 50 and 255 for shades of gray
		StarVertex.color = D3DCOLOR_ARGB( 255, nRand, nRand, nRand );
		
		StarVertex.x = (float)(rand()%m_nWidth);
		StarVertex.y = (float)(rand()%m_nHeight);
		
		m_VecStars.push_back( StarVertex );
		
		// bugbug jmccaskey - sometimes make "big stars" which are 4 points right next to each other?
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the star field
//-----------------------------------------------------------------------------
void CStarField::Render()
{
	if ( ( m_pGameEngine->GetViewportWidth() != m_nWidth ) || ( m_pGameEngine->GetViewportHeight() != m_nHeight ) )
	{
		Init();
	}
	
	for( size_t i = 0; i < m_VecStars.size(); ++i )
	{
		m_pGameEngine->BDrawPoint( m_VecStars[i].x, m_VecStars[i].y, m_VecStars[i].color );
	}

	m_pGameEngine->BFlushPointBuffer();
}