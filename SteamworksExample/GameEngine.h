//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Main class for the game engine
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <set>
#include <map>

// How big is the vertex buffer for batching lines in total?
// NOTE: This must be a multiple of the batch size below!!! (crashes will occur if it isn't)
#define LINE_BUFFER_TOTAL_SIZE 1000

// How many lines do we put in the buffer in between flushes?
//
// This should be enough smaller than the total size so that draw calls
// can finish using the data before we wrap around and discard it.
#define LINE_BUFFER_BATCH_SIZE 250

// How big is the vertex buffer for batching points in total?
// NOTE: This must be a multiple of the batch size below!!! (crashes will occur if it isn't)
#define POINT_BUFFER_TOTAL_SIZE 1800

// How many points do we put in the buffer in between flushes?
//
// This should be enough smaller than the total size so that draw calls
// can finish using the data before we wrap around and discard it.
#define POINT_BUFFER_BATCH_SIZE 600

// How big is the vertex buffer for batching quads in total?
// NOTE: This must be a multiple of the batch size below!!! (crashes will occur if it isn't)
#define QUAD_BUFFER_TOTAL_SIZE 1000

// How many quads do we put in the buffer in between flushes?
//
// This should be enough smaller than the total size so that draw calls
// can finish using the data before we wrap around and discard it.
#define QUAD_BUFFER_BATCH_SIZE 250

// Vertex struct for line batches
struct LineVertex_t
{
	float x, y, z, rhw;
	DWORD color;
};


// Vertex struct for point batches
struct PointVertex_t
{
	float x, y, z, rhw;
	DWORD color;
};

// Vertex struct for textured quads
struct TexturedQuadVertex_t
{
	float x, y, z, rhw;
	DWORD color;
	float u, v; // texture coordinates
};

// WndProc declaration
LRESULT CALLBACK GameWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

// Typedef for font handles
typedef int HGAMEFONT;

// Typedef for vertex buffer handles
typedef int HGAMEVERTBUF;

// Typedef for texture handles
typedef int HGAMETEXTURE;

class CGameEngine
{
public:

	// Static methods for tracing mapping of game engine class instances to hwnds
	static CGameEngine * FindEngineInstanceForHWND( HWND hWnd );
	static void AddInstanceToHWNDMap( CGameEngine* pInstance, HWND hWnd );
	static void RemoveInstanceFromHWNDMap( HWND hWnd );

	// Constructor
	CGameEngine( HINSTANCE hInstance, int nShowCommand, int32 nWindowWidth, int32 nWindowHeight );

	// Destructor
	~CGameEngine() { Shutdown(); }

	// Check if the game engine is initialized ok and ready for use
	bool BReadyForUse() { return m_bEngineReadyForUse; }

	// Check if the engine is shutting down
	bool BShuttingDown() { return m_bShuttingDown; }

	// Set the background color
	void SetBackgroundColor( short a, short r, short g, short b );

	// Start a frame, clear(), beginscene(), etc
	bool StartFrame();

	// Finish a frame, endscene(), present(), etc.
	void EndFrame();

	// Shutdown the game engine
	void Shutdown();

	// Pump messages from the OS
	void MessagePump();

	// Accessors for game screen size
	int32 GetViewportWidth() { return m_nViewportWidth; }
	int32 GetViewportHeight() { return m_nViewportHeight; }

	// Function for drawing text to the screen, dwFormat is a combination of flags like DT_LEFT, DT_VCENTER etc...
	bool BDrawString( HGAMEFONT hFont, RECT rect, DWORD dwColor, DWORD dwFormat, const char *pchText );

	// Create a new font returning our internal handle value for it (0 means failure)
	HGAMEFONT HCreateFont( int nHeight, int nFontWeight, bool bItalic, char * pchFont );

	// Create a new texture returning our internal handle value for it (0 means failure)
	HGAMETEXTURE HCreateTexture( byte *pRGBAData, uint32 uWidth, uint32 uHeight );

	// Draw a line, the engine itself will manage batching these (although you can explicitly flush if you need to)
	bool BDrawLine( float xPos0, float yPos0, DWORD dwColor0, float xPos1, float yPos1, DWORD dwColor1 );

	// Flush the line buffer
	bool BFlushLineBuffer();

	// Draw a point, the engine itself will manage batching these (although you can explicitly flush if you need to)
	bool BDrawPoint( float xPos, float yPos, DWORD dwColor );

	// Flush the point buffer
	bool BFlushPointBuffer();

	// Draw a filled quad
	bool BDrawFilledQuad( float xPos0, float yPos0, float xPos1, float yPos1, DWORD dwColor );

	// Draw a textured rectangle 
	bool BDrawTexturedQuad( float xPos0, float yPos0, float xPos1, float yPos1, 
							float u0, float v0, float u1, float v1, DWORD dwColor, HGAMETEXTURE hTexture );

	// Flush any still cached quad buffers
	bool BFlushQuadBuffer();

	// Track the state of keyboard input
	void RecordKeyDown( DWORD dwVK );
	void RecordKeyUp( DWORD dwVK );

	// Get the current state of a key
	bool BIsKeyDown( DWORD dwVK );

	// Get the first (in some arbitrary order) key down, if any
	bool BGetFirstKeyDown( DWORD *pdwVK );

	// Get current tick count for the game engine
	uint64 GetGameTickCount() { return m_ulGameTickCount; }

	// Get the tick count elapsed since the previous frame
	// bugbug - We use this time to compute things like thrust and acceleration in the game,
	//			so it's important in doesn't jump ahead by large increments... Need a better
	//			way to handle that.  
	uint64 GetGameTicksFrameDelta() { return m_ulGameTickCount - m_ulPreviousGameTickCount; }

	void SetGameTickCount( uint64 ulGameTickCount ) { m_ulPreviousGameTickCount = m_ulGameTickCount; m_ulGameTickCount = ulGameTickCount; }

	// Get the game engines hwnd
	HWND GetGameHWND() { return m_hWnd; }

	// Check if the game engine hwnd currently has focus (and a working d3d device)
	bool BGameEngineHasFocus() { return ::GetForegroundWindow() == m_hWnd && !m_bDeviceLost; }

	// Set the max FPS for the engine to run at (will sleep in EndFrame() to limit the rate)
	void SetMaxFPS( float flMaxFPS ) { m_flMaxFPS = flMaxFPS; }

private:
	// Creates the hwnd for the game
	bool BCreateGameWindow( int nShowCommand );

	// Initializes D3D for the game
	bool BInitializeD3D9();

	// Resets all the render, texture, and sampler states to our defaults
	void ResetRenderStates();

	// Create a new vertex buffer returning our internal handle for it (0 means failure)
	HGAMEVERTBUF HCreateVertexBuffer( uint32 nSizeInBytes, DWORD dwUsage, DWORD dwFVF );

	// Lock an entire vertex buffer with the specified flags
	bool BLockEntireVertexBuffer( HGAMEVERTBUF hVertBuf, void **ppVoid, DWORD dwFlags );

	// Unlock a vertex buffer
	bool BUnlockVertexBuffer( HGAMEVERTBUF hVertBuf );

	// Release a vertex buffer and free its resources
	bool BReleaseVertexBuffer( HGAMEVERTBUF hVertBuf );

	// Set steam source
	bool BSetStreamSource( HGAMEVERTBUF hVertBuf, uint32 uOffset, uint32 uStride );

	// Render primitives out of the current stream source
	bool BRenderPrimitive( D3DPRIMITIVETYPE primType, uint32 uStartVertex, uint32 uCount );

	// Set vertex format
	bool BSetFVF( DWORD dwFormat );

	// Handle losing the d3d device (ie, release resources)
	bool BHandleLostDevice();

	// Handle reseting the d3d device (ie, acquire resources again)
	bool BHandleResetDevice();


private:
	// Tracks whether the engine is ready for use
	bool m_bEngineReadyForUse;

	// Tracks if we are shutting down
	bool m_bShuttingDown;

	// Color we clear the background of the window to each frame
	DWORD m_dwBackgroundColor;

	// HInstance for the application running the engine
	HINSTANCE m_hInstance;

	// HWND for the engine instance
	HWND m_hWnd;

	// IDirect3D9 interface
	IDirect3D9 *m_pD3D9Interface;

	// IDirect3DDevice9 interface
	IDirect3DDevice9 *m_pD3D9Device;

	// Size of the window to display the game in
	int32 m_nWindowWidth;
	int32 m_nWindowHeight;

	// Size of actual d3d viewport (window size minus borders, title, etc)
	int32 m_nViewportWidth;
	int32 m_nViewportHeight;

	// Next font handle value to give out
	HGAMEFONT m_nNextFontHandle;

	// Map of font handles to font objects
	std::map<HGAMEFONT, ID3DXFont *> m_MapFontInstances;

	// Next vertex buffer handle value to give out
	HGAMEVERTBUF m_nNextVertBufferHandle;

	// Map of handles to vertex buffer objects
	struct VertBufData_t
	{
		bool m_bIsLocked;
		IDirect3DVertexBuffer9 * m_pBuffer;
	};
	std::map<HGAMEVERTBUF, VertBufData_t> m_MapVertexBuffers;

	HGAMETEXTURE m_nNextTextureHandle;

	// Map of handles to texture objects
	struct TextureData_t
	{
		byte *m_pRGBAData; // We keep a copy of the raw data so we can rebuild textures after a device is lost
		uint32 m_uWidth;
		uint32 m_uHeight;
		LPDIRECT3DTEXTURE9 m_pTexture;
	};
	std::map<HGAMETEXTURE, TextureData_t> m_MapTextures;

	// Vertex buffer for textured quads
	HGAMEVERTBUF m_hQuadBuffer;

	// Last texture used in drawing a batched quad
	HGAMETEXTURE m_hLastTexture;

	// Pointer to quad vertex data
	TexturedQuadVertex_t *m_pQuadVertexes;

	// How many quads are awaiting flushing
	DWORD m_dwQuadsToFlush;

	// Where does the current batch begin
	DWORD m_dwQuadBufferBatchPos;

	// White texture used when drawing filled quads
	HGAMETEXTURE m_hTextureWhite;

	// Currently set FVF format
	DWORD m_dwCurrentFVF;

	// Map of key state
	std::set<DWORD> m_SetKeysDown;

	// Current game time in milliseconds
	uint64 m_ulGameTickCount;

	// Game time at the start of the previous frame
	uint64 m_ulPreviousGameTickCount;

	// Maximum FPS to run the engine at
	float m_flMaxFPS;

	// Map of engine instances by HWND, used in wndproc to find engine instance for messages
	static std::map<HWND, CGameEngine *> m_MapEngineInstances;

	// Internal vertex buffer for batching line drawing
	HGAMEVERTBUF m_hLineBuffer;

	// Pointer to actual line buffer memory (valid only while locked)
	LineVertex_t *m_pLineVertexes;

	// Track how many lines are awaiting flushing in our line buffer
	DWORD m_dwLinesToFlush;

	// Track where the current batch starts in the vert buffer
	DWORD m_dwLineBufferBatchPos;

	// Internal vertex buffer for batching point drawing
	HGAMEVERTBUF m_hPointBuffer;

	// Pointer to actual point buffer memory (valid only while locked)
	PointVertex_t *m_pPointVertexes;

	// Track how many points are awaiting flushing in our line buffer
	DWORD m_dwPointsToFlush;

	// Track where the current batch starts in the vert buffer
	DWORD m_dwPointBufferBatchPos;

	// Track if we have lost the d3d device
	bool m_bDeviceLost;

	// Presentation parameters, saved in case of lost device needing reset
	D3DPRESENT_PARAMETERS m_d3dpp;

};

#endif // GAMEENGINE_H