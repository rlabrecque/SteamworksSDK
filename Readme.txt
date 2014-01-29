================================================================

Copyright © 1996-2010, Valve Corporation, All rights reserved.

================================================================


Welcome to the Steamworks SDK.  For documentation please see our partner 
website at: http://partner.steamgames.com


Revision History:

----------------------------------------------------------------
v1.09	12th May 2010
----------------------------------------------------------------

Mac Steamworks!
* new binaries in the sdk/redistributable_bin/osx/ folder

Other
* explicit pragma( pack, 8 ) added around all callbacks and structures, for devs who have use a different default packing
* renamed function ISteamGameServer::SetGameType() to the more accurate ISteamGameServer::SetGameTags()


----------------------------------------------------------------
v1.08	27st January 2010
----------------------------------------------------------------

Matchmaking
* added function ISteamMatching::AddRequestLobbyListDistanceFilter(), to specify how far geographically you want to search for other lobbies
* added function ISteamMatching::AddRequestLobbyListResultCountFilter(), to specify how the maximum number of lobby you results you need (less is faster)

Stats & Achievements
* added interface ISteamGameServerStats, which enables access to stats and achievements for users to the game server
* removed function ISteamGameServer::BGetUserAchievementStatus(), now handled by ISteamGameServerStats
* added ISteamUserStats::GetAchievementAndUnlockTime(), which returns if and when a user unlocked an achievement

Other
* added new constant k_cwchPersonaNameMax (32), which is the maximum number of unicode characters a users name can be
* removed ISteamRemoteStorage::FileDelete() - NOTE: it will be back, it's only removed since it hadn't been implemented on the back-end yet
* added function ISteamGameServer::GetServerReputation(), gives returns a game server reputation score based on how long users typically play on the server


----------------------------------------------------------------
v1.07	16th December 2009
----------------------------------------------------------------

* Replaced SteamAPI_RestartApp() with SteamAPI_RestartAppIfNecessary(). This new function detects if the process was started through Steam, and starts the current game through Steam if necessary.
* Added ISteamUtils::BOverlayNeedsPresent() so games with event driven rendering can determine when the Steam overlay needs to draw


----------------------------------------------------------------
v1.06	30th September 2009
----------------------------------------------------------------

Voice
* ISteamUser::GetCompressedVoice() has been replaced with ISteamUser::GetVoice which can be used to retrieve compressed and uncompressed voice data
* Added ISteamUser::GetAvailableVoice() to retrieve the amount of captured audio data that is available

Matchmaking
* Added a new callback LobbyKicked_t that is sent when a user has been disconnected from a lobby
* Through ISteamMatchmakingServers, multiple server list requests of the same type can now be outstanding at the same time

Steamworks Setup Application:
* Streamlined configuration process
* Now supports EULAs greater than 32k bytes

Content Tool
* Added DLC checkbox to depot creation wizard

Other
* Added SteamAPI_IsSteamRunning()
* Added SteamAPI_RestartApp() so CEG users can restart their game through Steam if launched through Windows Games Explorer



----------------------------------------------------------------
v1.05	11th June 2009
----------------------------------------------------------------

Matchmaking
* Added the SteamID of the gameserver to the gameserveritem_t structure (returned only by newer game servers)
* Added ISteamUserStats::GetNumberOfCurrentPlayers(), asyncronously returns the number users currently running this game
* Added k_ELobbyComparisonNotEqual comparision functions for filters
* Added option to use comparison functions for string filters
* Added ISteamMatchmaking::AddRequestLobbyListFilterSlotsAvailable( int nSlotsAvailable ) filter function, so you can find a lobby for a group of users to join
* Extended ISteamMatchmaking::CreateLobby() to take the max number of users in the lobby
* Added ISteamMatchmaking::GetLobbyDataCount(), ISteamMatchmaking::GetLobbyDataByIndex() so you can iterate all the data set on a lobby
* Added ISteamMatchmaking::DeleteLobbyData() so you can clear a key from a lobby
* Added ISteamMatchmaking::SetLobbyOwner() so that ownership of a lobby can be transferred
* Added ISteamMatchmaking::SetLobbyJoinable()
* Added ISteamGameServer::SetGameData(), so game server can set more information that can be filtered for in the server pinging API

Networking
* Added a set of connectionless networking functions for easy use for making peer-to-peer (NAT traversal) connections. Includes supports for windowed reliable sendsand fragementation/re-assembly of large packets. See ISteamNetworking.h for more details.

Leaderboards
* Added enum ELeaderboardUploadScoreMethod and changed ISteamUserStats::UploadLeaderboardScore() to take this - lets you force a score to be changed even if it's worse than the prior score

Callbacks
* Added CCallbackManual<> class to steam_api.h, a version of CCallback<> that doesn't register itself automatically in it's the constructor

Downloadable Content
* Added ISteamUser::UserHasLicenseForApp() and ISteamGameServer::UserHasLicenseForApp() to enable checking if a user owns DLC in multiplayer. See the DLC documentation for more info.

Game Overlay
* ISteamFriends::ActivateGameOverlay() now accepts "Stats" and "Achievements"



----------------------------------------------------------------
v1.04	9th Mar 2009
----------------------------------------------------------------

Added Peer To Peer Multi-Player Authentication/Authorization:
* Allows each peer to verify the unique identity of the peers ( by steam account id ) in their game and determine if that user is allowed access to the game.
* Added to the ISteamUser interface: GetAuthSessionTicket(), BeginAuthSession(), EndAuthSession() and CancelAuthTicket()
* Additional information can be found in the API Overview on the Steamworks site

Added support for purchasing downloadable content in game:
* Added ISteamApps::BIsDlcInstalled() and the DlcInstalled_t callback, which enable a game to check if downloadable content is owned and installed
* Added ISteamFriends::ActivateGameOverlayToStore(), which opens the Steam game overlay to the store page for an appID (can be a game or DLC)

Gold Master Creation:
* It is no longer optional to encrypt depots on a GM
* The GM configuration file now supports an included_depots key, which along with the excluded_depots key, allows you to specify exactly which depots are placed on a GM
* Simplified the configuration process for the setup application
* The documentation for creating a Gold Master has been rewritten and extended. See the Steamworks site for more information.

Added Leaderboards:
* 10k+ leaderboards can now be created programmatically per game, and queried globally or compared to friends
* Added to ISteamUserStats interface
* See SteamworksExample for a usage example

Other:
* Added SteamShutdown_t callback, which will alert the game when Steam wants to shut down
* Added ISteamUtils::IsOverlayEnabled(), which can be used to detect if the user has disabled the overlay in the Steam settings
* Added ISteamUserStats::ResetAllStats(), which can be used to reset all stats (and optionally achievements) for a user
* Moved SetWarningMessageHook() from ISteamClient to ISteamUtils
* Added SteamAPI_SetTryCatchCallbacks,  sets whether or not Steam_RunCallbacks() should do a try {} catch (...) {} around calls to issuing callbacks
* In CCallResult callback, CCallResult::IsActive() will return false and can now reset the CCallResult
* Added support for zero-size depots
* Properly strip illegal characters from depot names



----------------------------------------------------------------
v1.03	16th Jan 2009
----------------------------------------------------------------

Major changes:
* ISteamRemoteStorage interface has been added, which contains functions to store per-user data in the Steam Cloud back-end. 
** To use this, you must first use the partner web site to enable Cloud for your game. 
** The current setting is allowing 1MB of storage per-game per-user (we hope to increase this over time).

Lobby & Matchmaking related changes:
* ISteamFriends::GetFriendGamePlayed() now also return the steamID of the lobby the friend is in, if any. It now takes a pointer to a new FriendGameInfo_t struct, which it fills 
* Removed ISteamFriends::GetFriendsLobbies(), since this is now redundant to ISteamFriends::GetFriendGamePlayed()
* Added enum ELobbyComparison, to set the comparison operator in ISteamMatchmaking::AddRequestLobbyListNumericalFilter()
* Changed ISteamMatchmaking::CreateLobby(), JoinLobby() and RequestLobbyList() to now return SteamAPICall_t handles, so you can easily track if a particular call has completed (see below)
* Added ISteamMatchmaking::SetLobbyType(), which can switch a lobby between searchable (public) and friends-only
* Added ISteamMatchmaking::GetLobbyOwner(), which returns the steamID of the user who is currently the owner of the lobby. The back-end ensures that one and only one user is ever the owner. If that user leaves the lobby, another user will become the owner.

Steam game-overlay interaction:
* Added a new callback GameLobbyJoinRequested_t, which is sent to the game if the user selects 'Join friends game' from the Steam friends list, and that friend is in a lobby. The game should initiate connection to that lobby.
* Changed ISteamFriends::ActivateGameOverlay() can now go to "Friends", "Community", "Players", "Settings", "LobbyInvite", "OfficialGameGroup"
* Added ISteamFriends::ActivateGameOverlayToUser(), which can open a either a chat dialog or another users Steam community profile
* Added ISteamFriends::ActivateGameOverlayToWebPage(), which opens the Steam game-overlay web browser to the specified url

Stats system changes:
* Added ISteamUserStats::RequestUserStats(), to download the current game stats of another user
* Added ISteamUserStats::GetUserStat() and ISteamUserStats::GetUserAchievement() to access the other users stats, once they've been downloaded

Callback system changes:
* Added new method for handling asynchronous call results, currently used by CreateLobby(), JoinLobby(), RequestLobbyList(), and RequestUserStats(). Each of these functions returns a handle, SteamAPICall_t, that can be used to track the completion state of a call.
* Added new object CCallResult<>, which can map the completion of a SteamAPICall_t to a function, and include the right data. See SteamworksExample for a usage example.
* Added ISteamUtils::IsAPICallCompleted(), GetAPICallFailureReason(), and GetAPICallResult(), which can be used to track the state of a SteamAPICall_t (although it is recommended to use CCallResult<>, which wraps these up nicely)

Other:
* Added ISteamGameServer::GetPublicIP(), which is the IP address of a game server as seen by the Steam back-end
* Added "allow relay" parameter to ISteamNetworking::CreateP2PConnectionSocket() and CreateListenSocket(), which specified if being bounced through Steam relay servers is OK if a direct p2p connection fails (will have a much higher latency, but increases chance of making a connection)
* Added IPCFailure_t callback, which will be posted to the game if Steam itself has crashed, or if Steam_RunCallbacks() hasn't been called in a long time



----------------------------------------------------------------
v1.02	4th Sep 2008
----------------------------------------------------------------

The following interfaces have been updated:

ISteamUser

	// Starts voice recording. Once started, use GetCompressedVoice() to get the data
	virtual void StartVoiceRecording( ) = 0;

	// Stops voice recording. Because people often release push-to-talk keys early, the system will keep recording for
	// a little bit after this function is called. GetCompressedVoice() should continue to be called until it returns
	// k_eVoiceResultNotRecording
	virtual void StopVoiceRecording( ) = 0;

	// Gets the latest voice data. It should be called as often as possible once recording has started.
	// nBytesWritten is set to the number of bytes written to pDestBuffer. 
	virtual EVoiceResult GetCompressedVoice( void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten ) = 0;

	// Decompresses a chunk of data produced by GetCompressedVoice(). nBytesWritten is set to the 
	// number of bytes written to pDestBuffer. The output format of the data is 16-bit signed at 
	// 11025 samples per second.
	virtual EVoiceResult DecompressVoice( void *pCompressed, uint32 cbCompressed, void *pDestBuffer, uint32 cbDestBufferSize, uint32 *nBytesWritten ) = 0;

virtual int InitiateGameConnection( void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer, bool bSecure ) = 0;

This has been extended to be usable for games that don't use the other parts of Steamworks matchmaking. This allows any multiplayer game to easily notify the Steam client of the IP:Port of the game server the user is connected to, so that their friends can join them via the Steam friends list. Empty values are taken for auth blob.

            virtual bool GetUserDataFolder( char *pchBuffer, int cubBuffer ) = 0;

This function returns a hint as a good place to store per- user per-game data.



ISteamMatchmaking

Added a set of server-side lobby filters, as well as voice chat, lobby member limits, and a way of quickly accessing the list of lobbies a users friends are in.

      // filters for lobbies
      // this needs to be called before RequestLobbyList() to take effect
      // these are cleared on each call to RequestLobbyList()
      virtual void AddRequestLobbyListFilter( const char *pchKeyToMatch, const char *pchValueToMatch ) = 0;
      // numerical comparison - 0 is equal, -1 is the lobby value is less than nValueToMatch, 1 is the lobby value is greater than nValueToMatch
      virtual void AddRequestLobbyListNumericalFilter( const char *pchKeyToMatch, int nValueToMatch, int nComparisonType /* 0 is equal, -1 is less than, 1 is greater than */ ) = 0;
      // sets RequestLobbyList() to only returns lobbies which aren't yet full - needs SetLobbyMemberLimit() called on the lobby to set an initial limit
      virtual void AddRequestLobbyListSlotsAvailableFilter() = 0;

      // returns the details of a game server set in a lobby - returns false if there is no game server set, or that lobby doesn't exist
      virtual bool GetLobbyGameServer( CSteamID steamIDLobby, uint32 *punGameServerIP, uint16 *punGameServerPort, CSteamID *psteamIDGameServer ) = 0;

      // set the limit on the # of users who can join the lobby
      virtual bool SetLobbyMemberLimit( CSteamID steamIDLobby, int cMaxMembers ) = 0;
      // returns the current limit on the # of users who can join the lobby; returns 0 if no limit is defined
      virtual int GetLobbyMemberLimit( CSteamID steamIDLobby ) = 0;

      // asks the Steam servers for a list of lobbies that friends are in
      // returns results by posting one RequestFriendsLobbiesResponse_t callback per friend/lobby pair
      // if no friends are in lobbies, RequestFriendsLobbiesResponse_t will be posted but with 0 results
      // filters don't apply to lobbies (currently)
      virtual bool RequestFriendsLobbies() = 0;


ISteamUtils
      // Sets the position where the overlay instance for the currently calling game should show notifications.
      // This position is per-game and if this function is called from outside of a game context it will do nothing.
      virtual void SetOverlayNotificationPosition( ENotificationPosition eNotificationPosition ) = 0;


ISteamFriends
            virtual int GetFriendAvatar( CSteamID steamIDFriend, int eAvatarSize ) = 0;

This function now takes an eAvatarSize parameter, which can be k_EAvatarSize32x32 or k_EAvatarSize64x64 (previously it always just returned a handle to the 32x32 image)


----------------------------------------------------------------
v1.01  	8th Aug 2008
----------------------------------------------------------------

The Steamworks SDK has been updated to simplfy game server authentication and better expose application state


----------------------------------------------------------------
v1.0:
----------------------------------------------------------------

- Initial Steamworks SDK release