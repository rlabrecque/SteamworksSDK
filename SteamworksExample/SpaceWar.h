//========= Copyright © 1996-2008, Valve LLC, All rights reserved. ============
//
// Purpose: Shared definitions for the communication between the server/client
//
// $NoKeywords: $
//=============================================================================

#ifndef SPACEWAR_H
#define SPACEWAR_H

// The Steamworks API's are modular, you can use some subsystems without using others
// When USE_GS_AUTH_API is defined you get the following Steam features:
// - Strong user authentication and authorization
// - Game server matchmaking
// - VAC cheat protection
// - Access to achievement/community API's
// - P2P networking capability

// Remove this define to disable using the native Steam authentication and matchmaking system
// You can use this as a sample of how to integrate your game without replacing an existing matchmaking system
// When you un-define USE_GS_AUTH_API you get:
// - Access to achievement/community API's
// - P2P networking capability
// You CANNOT use:
// - VAC cheat protection
// - Game server matchmaking
// as these function depend on using Steam authentication
#define USE_GS_AUTH_API 


// Current game server version
#define SPACEWAR_SERVER_VERSION "1.0.0.0"

// UDP port for the spacewar server to do authentication on (ie, talk to Steam on)
#define SPACEWAR_AUTHENTICATION_PORT 8766

// UDP port for the spacewar server to listen on
#define SPACEWAR_SERVER_PORT 27015

// UDP port for the master server updater to listen on
#define SPACEWAR_MASTER_SERVER_UPDATER_PORT 27016

// How long to wait for a response from the server before resending our connection attempt
#define SERVER_CONNECTION_RETRY_MILLISECONDS 350

// How long to wait for a client to send an update before we drop its connection server side
#define SERVER_TIMEOUT_MILLISECONDS 5000

// Maximum packet size in bytes
#define MAX_SPACEWAR_PACKET_SIZE 1024*512

// Maximum number of players who can join a server and play simultaneously
#define MAX_PLAYERS_PER_SERVER 4

// Time to pause wait after a round ends before starting a new one
#define MILLISECONDS_BETWEEN_ROUNDS 4000

// How long photon beams live before expiring
#define PHOTON_BEAM_LIFETIME_IN_TICKS 1750

// How fast can photon beams be fired?
#define PHOTON_BEAM_FIRE_INTERVAL_TICKS 250

// Amount of space needed for beams per ship
#define MAX_PHOTON_BEAMS_PER_SHIP PHOTON_BEAM_LIFETIME_IN_TICKS/PHOTON_BEAM_FIRE_INTERVAL_TICKS

// Time to timeout a connection attempt in
#define MILLISECONDS_CONNECTION_TIMEOUT 30000

// How many times a second does the server send world updates to clients
#define SERVER_UPDATE_SEND_RATE 60

// How many times a second do we send our updated client state to the server
#define CLIENT_UPDATE_SEND_RATE 30

// How fast does the server internally run at?
#define MAX_CLIENT_AND_SERVER_FPS 86


// Player colors
DWORD const g_rgPlayerColors[ MAX_PLAYERS_PER_SERVER ] = 
{ 
	D3DCOLOR_ARGB( 255, 255, 150, 150 ), // red 
	D3DCOLOR_ARGB( 255, 200, 200, 255 ), // blue
	D3DCOLOR_ARGB( 255, 255, 204, 102 ), // orange
	D3DCOLOR_ARGB( 255, 153, 255, 153 ), // green
};


// Enum for possible game states on the client
enum EClientGameState
{
	k_EClientGameStartServer,
	k_EClientGameActive,
	k_EClientGameWaitingForPlayers,
	k_EClientGameMenu,
	k_EClientGameQuitMenu,
	k_EClientGameExiting,
	k_EClientGameInstructions,
	k_EClientGameDraw,
	k_EClientGameWinner,
	k_EClientGameConnecting,
	k_EClientGameConnectionFailure,
	k_EClientFindInternetServers,
	k_EClientStatsAchievements,
	k_EClientCreatingLobby,
	k_EClientInLobby,
	k_EClientFindLobby,
	k_EClientJoiningLobby,
	k_EClientFindLANServers,
};


// Enum for possible game states on the server
enum EServerGameState
{
	k_EServerWaitingForPlayers,
	k_EServerActive,
	k_EServerDraw,
	k_EServerWinner,
	k_EServerExiting,
};


// Data sent per photon beam from the server to update clients photon beam positions
struct ServerPhotonBeamUpdateData_t
{
	// Does the photon beam exist right now?
	bool m_bIsActive; 

	// The current rotation 
	float m_flCurrentRotation;

	// The current velocity
	float m_flXVelocity;
	float m_flYVelocity;

	// The current position
	float m_flXPosition;
	float m_flYPosition;
};


// This is the data that gets sent per ship in each update, see below for the full update data
struct ServerShipUpdateData_t
{
	// The current rotation of the ship
	float m_flCurrentRotation;

	// The delta in rotation for the last frame (client side interpolation will use this)
	float m_flRotationDeltaLastFrame;

	// The current thrust for the ship
	float m_flXAcceleration;
	float m_flYAcceleration;

	// The current velocity for the ship
	float m_flXVelocity;
	float m_flYVelocity;

	// The current position for the ship
	float m_flXPosition;
	float m_flYPosition;

	// Is the ship exploding?
	bool m_bExploding;

	// Is the ship disabled?
	bool m_bDisabled;

	// Are the thrusters to be drawn?
	bool m_bForwardThrustersActive;
	bool m_bReverseThrustersActive;

	// Photon beam positions and data
	ServerPhotonBeamUpdateData_t m_PhotonBeamData[MAX_PHOTON_BEAMS_PER_SHIP];
};


// This is the data that gets sent from the server to each client for each update
struct ServerSpaceWarUpdateData_t
{
	// What state the game is in
	EServerGameState m_eCurrentGameState;

	// Who just won the game? -- only valid when m_eCurrentGameState == k_EGameWinner
	uint32 m_uPlayerWhoWonGame;

	// which player slots are in use
	bool m_rgPlayersActive[MAX_PLAYERS_PER_SERVER];

	// what are the scores for each player?
	uint32 m_rgPlayerScores[MAX_PLAYERS_PER_SERVER];

	// array of ship data
	ServerShipUpdateData_t m_rgShipData[MAX_PLAYERS_PER_SERVER];

	// array of players steamids for each slot, serialized to uint64
	uint64 m_rgPlayerSteamIDs[MAX_PLAYERS_PER_SERVER];
};


// This is the data that gets sent from each client to the server for each update
struct ClientSpaceWarUpdateData_t
{
	// Key's which are done
	bool m_bFirePressed;
	bool m_bTurnLeftPressed;
	bool m_bTurnRightPressed;
	bool m_bForwardThrustersPressed;
	bool m_bReverseThrustersPressed;

	// Name of the player (needed server side to tell master server about)
	// bugbug jmccaskey - Really lame to send this every update instead of event driven...
	char m_rgchPlayerName[64];
};

#endif // SPACEWAR_H