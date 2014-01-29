This is the Steamworks SDK v1.02

================================================================

Copyright © 1996-2008, Valve Corporation, All rights reserved.

================================================================


Welcome to the Steamworks SDK.  For documentation please see our partner 
website at: http://partner.steamgames.com


Revision History:


----------------------------------------------------------------
v1.02	09/04/08
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
v1.01  	08/08/08
----------------------------------------------------------------

The Steamworks SDK has been updated to simplfy game server authentication and better expose application state


----------------------------------------------------------------
v1.0:
----------------------------------------------------------------

- Initial Steamworks SDK release