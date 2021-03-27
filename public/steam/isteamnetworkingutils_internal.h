//====== Copyright Valve Corporation, All rights reserved. ====================
//
// Purpose: Internal implementation details for ISteamNetworkingUtils5
//
//=============================================================================

#ifndef ISTEAMNETWORKINGUTILS_INTERNAL
#define ISTEAMNETWORKINGUTILS_INTERNAL
#pragma once

#ifndef ISTEAMNETWORKINGUTILS
#error "This file should only be included from steamnetworkingutils.h"
#endif

inline void ISteamNetworkingUtils::InitRelayNetworkAccess() { CheckPingDataUpToDate( 1e10f ); }
inline bool ISteamNetworkingUtils::SetGlobalConfigValueInt32( ESteamNetworkingConfigValue eValue, int32 val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Global, 0, k_ESteamNetworkingConfig_Int32, &val ); }
inline bool ISteamNetworkingUtils::SetGlobalConfigValueFloat( ESteamNetworkingConfigValue eValue, float val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Global, 0, k_ESteamNetworkingConfig_Float, &val ); }
inline bool ISteamNetworkingUtils::SetGlobalConfigValueString( ESteamNetworkingConfigValue eValue, const char *val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Global, 0, k_ESteamNetworkingConfig_String, val ); }
inline bool ISteamNetworkingUtils::SetGlobalConfigValuePtr( ESteamNetworkingConfigValue eValue, void *val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Global, 0, k_ESteamNetworkingConfig_Ptr, &val ); } // Note: passing pointer to pointer.
inline bool ISteamNetworkingUtils::SetConnectionConfigValueInt32( HSteamNetConnection hConn, ESteamNetworkingConfigValue eValue, int32 val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Connection, hConn, k_ESteamNetworkingConfig_Int32, &val ); }
inline bool ISteamNetworkingUtils::SetConnectionConfigValueFloat( HSteamNetConnection hConn, ESteamNetworkingConfigValue eValue, float val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Connection, hConn, k_ESteamNetworkingConfig_Float, &val ); }
inline bool ISteamNetworkingUtils::SetConnectionConfigValueString( HSteamNetConnection hConn, ESteamNetworkingConfigValue eValue, const char *val ) { return SetConfigValue( eValue, k_ESteamNetworkingConfig_Connection, hConn, k_ESteamNetworkingConfig_String, val ); }
inline bool ISteamNetworkingUtils::SetGlobalCallback_SteamNetConnectionStatusChanged( FnSteamNetConnectionStatusChanged fnCallback ) { return SetGlobalConfigValuePtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)fnCallback ); }
inline bool ISteamNetworkingUtils::SetGlobalCallback_SteamNetAuthenticationStatusChanged( FnSteamNetAuthenticationStatusChanged fnCallback ) { return SetGlobalConfigValuePtr( k_ESteamNetworkingConfig_Callback_AuthStatusChanged, (void*)fnCallback ); }
inline bool ISteamNetworkingUtils::SetGlobalCallback_SteamRelayNetworkStatusChanged( FnSteamRelayNetworkStatusChanged fnCallback ) { return SetGlobalConfigValuePtr( k_ESteamNetworkingConfig_Callback_RelayNetworkStatusChanged, (void*)fnCallback ); }
inline bool ISteamNetworkingUtils::SetGlobalCallback_MessagesSessionRequest( FnSteamNetworkingMessagesSessionRequest fnCallback ) { return SetGlobalConfigValuePtr( k_ESteamNetworkingConfig_Callback_MessagesSessionRequest, (void*)fnCallback ); }
inline bool ISteamNetworkingUtils::SetGlobalCallback_MessagesSessionFailed( FnSteamNetworkingMessagesSessionFailed fnCallback ) { return SetGlobalConfigValuePtr( k_ESteamNetworkingConfig_Callback_MessagesSessionFailed, (void*)fnCallback ); }

inline bool ISteamNetworkingUtils::SetConfigValueStruct( const SteamNetworkingConfigValue_t &opt, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj )
{
	// Locate the argument.  Strings are a special case, since the
	// "value" (the whole string buffer) doesn't fit in the struct
	// NOTE: for pointer values, we pass a pointer to the pointer,
	// we do not pass the pointer directly.
	const void *pVal = ( opt.m_eDataType == k_ESteamNetworkingConfig_String ) ? (const void *)opt.m_val.m_string : (const void *)&opt.m_val;
	return SetConfigValue( opt.m_eValue, eScopeType, scopeObj, opt.m_eDataType, pVal );
}

// How to get helper functions.
#if defined( STEAMNETWORKINGSOCKETS_STATIC_LINK ) || defined( STEAMNETWORKINGSOCKETS_STANDALONELIB )

	// Call direct to static functions
	STEAMNETWORKINGSOCKETS_INTERFACE void SteamNetworkingIPAddr_ToString( const SteamNetworkingIPAddr *pAddr, char *buf, size_t cbBuf, bool bWithPort );
	STEAMNETWORKINGSOCKETS_INTERFACE bool SteamNetworkingIPAddr_ParseString( SteamNetworkingIPAddr *pAddr, const char *pszStr );
	STEAMNETWORKINGSOCKETS_INTERFACE void SteamNetworkingIdentity_ToString( const SteamNetworkingIdentity *pIdentity, char *buf, size_t cbBuf );
	STEAMNETWORKINGSOCKETS_INTERFACE bool SteamNetworkingIdentity_ParseString( SteamNetworkingIdentity *pIdentity, size_t sizeofIdentity, const char *pszStr );
	inline void SteamNetworkingIPAddr::ToString( char *buf, size_t cbBuf, bool bWithPort ) const { SteamNetworkingIPAddr_ToString( this, buf, cbBuf, bWithPort ); }
	inline bool SteamNetworkingIPAddr::ParseString( const char *pszStr ) { return SteamNetworkingIPAddr_ParseString( this, pszStr ); }
	inline void SteamNetworkingIdentity::ToString( char *buf, size_t cbBuf ) const { SteamNetworkingIdentity_ToString( this, buf, cbBuf ); }
	inline bool SteamNetworkingIdentity::ParseString( const char *pszStr ) { return SteamNetworkingIdentity_ParseString( this, sizeof(*this), pszStr ); }

#elif defined( STEAMNETWORKINGSOCKETS_STEAMAPI )
	// Using steamworks SDK - go through SteamNetworkingUtils()
	inline void SteamNetworkingIPAddr::ToString( char *buf, size_t cbBuf, bool bWithPort ) const { SteamNetworkingUtils()->SteamNetworkingIPAddr_ToString( *this, buf, cbBuf, bWithPort ); }
	inline bool SteamNetworkingIPAddr::ParseString( const char *pszStr ) { return SteamNetworkingUtils()->SteamNetworkingIPAddr_ParseString( this, pszStr ); }
	inline void SteamNetworkingIdentity::ToString( char *buf, size_t cbBuf ) const { SteamNetworkingUtils()->SteamNetworkingIdentity_ToString( *this, buf, cbBuf ); }
	inline bool SteamNetworkingIdentity::ParseString( const char *pszStr ) { return SteamNetworkingUtils()->SteamNetworkingIdentity_ParseString( this, pszStr ); }
#else
	#error "Invalid config"
#endif

#endif // #ifndef ISTEAMNETWORKINGUTILS_INTERNAL
