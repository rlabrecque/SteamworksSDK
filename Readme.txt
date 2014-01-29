This is the Steamworks SDK v1.03

================================================================

Copyright © 1996-2009, Valve Corporation, All rights reserved.

================================================================


Welcome to the Steamworks SDK.  For documentation please see our partner 
website at: http://partner.steamgames.com


Revision History:

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