#pragma once
#include "NetPacket.h"

/** Send/receive chat and lobby broadcast*/
class CChatPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatPacket )
public:
	ZDATA
		// chat message
		std::wstring wszMessage;
	//  To/From Nick 
	std::string szNick;
	//  To ID 
	int nID;
	bool bIsBroadcast;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszMessage); f.Add(3,&szNick); f.Add(4,&nID); f.Add(5,&bIsBroadcast); return 0; }

	CChatPacket() { }
	CChatPacket( const int nClientID, const std::wstring &_wszMessage, const std::string &_szNick, const int _nID, bool _bIsBroadcast )
		: CNetPacket( nClientID ), wszMessage( _wszMessage ), szNick( _szNick ), nID( _nID ), bIsBroadcast( _bIsBroadcast ) { }
};

/** set don't receive lobby chat */
class CChatAFKPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatAFKPacket )
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CChatAFKPacket() { }
	CChatAFKPacket( const int nClientID ) : CNetPacket( nClientID ) {}
};

class CChatAFKResponsePacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatAFKResponsePacket )
public:
	ZDATA
		std::string szAFKNick;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szAFKNick); return 0; }

	CChatAFKResponsePacket() {}
	CChatAFKResponsePacket( const int nClientID, const std::string& _szAFKNick ) : CNetPacket( nClientID ), szAFKNick( _szAFKNick ) {}
};

//  Select chat channel, create if not exist
class CChatChannelPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelPacket )
public:
	ZDATA
		std::string szChannelName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szChannelName); return 0; }

	CChatChannelPacket() {}
	CChatChannelPacket( const int nClientID, const std::string &_szChannelName )
		: CNetPacket( nClientID ), szChannelName( _szChannelName ) {}
};

//  Chat channel clients list ( for just-joined clients )

struct SIDNickPair
{
	ZDATA
		int nID;
		std::string szNick;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szNick); return 0; }
};

class CChatChannelClientsListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelClientsListPacket )
public:
	ZDATA
		std::list<SIDNickPair> clientsList;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&clientsList); return 0; }

	CChatChannelClientsListPacket() {}
	CChatChannelClientsListPacket( const int nClientID, const std::list<SIDNickPair> &_clientsList )
		:	CNetPacket( nClientID ), clientsList( _clientsList ) {}
};

//  client joined/left notification
class CChatClientListChangeNotifyPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatClientListChangeNotifyPacket )
public:
	ZDATA
		int nID;
		std::string szNick;
		bool bJoined; // true for joining clients, false for leaving
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szNick); f.Add(4,&bJoined); return 0; }

	CChatClientListChangeNotifyPacket() {}
	CChatClientListChangeNotifyPacket( const int nClientID, const int _nID, const std::string &_szNick, bool _bJoined )
		:	CNetPacket( nClientID ), nID( _nID ), szNick( _szNick ), bJoined( _bJoined ) {}	
};

class CChatChannelsListRequestPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelsListRequestPacket )
public:
	ZDATA
		uint32_t dwVersion;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); return 0; }

	CChatChannelsListRequestPacket() {}
	CChatChannelsListRequestPacket( const int nClientID, const uint32_t _dwVersion )
		: CNetPacket( nClientID ), dwVersion( _dwVersion ) {}
};

class CChatChannelsListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelsListPacket )
public:
	ZDATA
		uint32_t dwVersion;
		std::list<std::string> added;
		std::list<std::string> removed;
		bool bIsFullUpdate;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); f.Add(3,&added); f.Add(4,&removed); f.Add(5,&bIsFullUpdate); return 0; }

	CChatChannelsListPacket() {}
	CChatChannelsListPacket( const int nClientID, const uint32_t _dwVersion, const std::list<std::string> &_added,
		const std::list<std::string> &_removed, bool _bIsFullUpdate )
		: CNetPacket( nClientID ), dwVersion( _dwVersion ), added( _added ), removed( _removed ), bIsFullUpdate( _bIsFullUpdate ) {}
};

// Add 'nSender' to ignore list
class CChatModifyIgnoreFriendListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatModifyIgnoreFriendListPacket )
public:
	enum EFriendIgnore
	{
		ADD_IGNORE,
		ADD_FRIEND,
		REMOVE_IGNORE,
		REMOVE_FRIEND
	};
	ZDATA
		std::string szPlayer;
		EFriendIgnore eChange;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szPlayer); f.Add(3,&eChange); return 0; }

	CChatModifyIgnoreFriendListPacket() {}
	CChatModifyIgnoreFriendListPacket( const int nClientID, const std::string &_szPlayer, const EFriendIgnore _eChange )
		: CNetPacket( nClientID ), szPlayer( _szPlayer ), eChange( _eChange ) {}
};

class CChatIgnoreFriendListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatIgnoreFriendListPacket )
public:
	ZDATA
		std::list< std::string > ignoreList;
		std::list< std::string > friendList;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&ignoreList); f.Add(3,&friendList); return 0; }

	CChatIgnoreFriendListPacket() {}
	CChatIgnoreFriendListPacket( const int nClientID, const std::list<std::string> & _ignoreList, const std::list<std::string> & _friendList )
		: CNetPacket( nClientID ), ignoreList( _ignoreList ), friendList( _friendList ) {}
};

class CChatGetIgnoreFriendListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatGetIgnoreFriendListPacket )
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CChatGetIgnoreFriendListPacket() {}
	CChatGetIgnoreFriendListPacket( const int nClientID ) : CNetPacket( nClientID ) {}
};

enum EChatStatus
{
	OFFLINE = 0,
	ONLINE = 1,
	AFK = 2,
};

class CChatFriendNotifyPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatFriendNotifyPacket )
public:
	ZDATA
		std::string szNick;
		char cChatStatus; // см. EChatStatus
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&cChatStatus); return 0; }

	CChatFriendNotifyPacket() {}
	CChatFriendNotifyPacket( const int nClientID, const std::string &_szNick, const char _cStatus )
		: CNetPacket( nClientID ), szNick( _szNick ), cChatStatus( _cStatus ) {}
};

class CChatChannelByNickPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelByNickPacket )
public:
	ZDATA
		std::string szNick;
		std::string szChannel;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&szChannel); return 0; }
	CChatChannelByNickPacket() {}
	CChatChannelByNickPacket( const int nClientID, const std::string &_szNick, const std::string &_szChannel )
		: CNetPacket( nClientID ), szNick( _szNick ), szChannel( _szChannel ) {}
} ;


