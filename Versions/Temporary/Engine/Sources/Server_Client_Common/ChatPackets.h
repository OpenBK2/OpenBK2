#pragma once
#include "NetPacket.h"

/** Send/receive chat and lobby broadcast*/
class CChatPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatPacket )
public:
	ZDATA
		// chat message
		wstring wszMessage;	
	//  To/From Nick 
	string szNick;
	//  To ID 
	int nID;
	bool bIsBroadcast;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wszMessage); f.Add(3,&szNick); f.Add(4,&nID); f.Add(5,&bIsBroadcast); return 0; }

	CChatPacket() { }
	CChatPacket( const int nClientID, const wstring &_wszMessage, const string &_szNick, const int _nID, bool _bIsBroadcast )
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
		string szAFKNick;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szAFKNick); return 0; }

	CChatAFKResponsePacket() {}
	CChatAFKResponsePacket( const int nClientID, const string& _szAFKNick ) : CNetPacket( nClientID ), szAFKNick( _szAFKNick ) {}
};

//  Select chat channel, create if not exist
class CChatChannelPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelPacket )
public:
	ZDATA
		string szChannelName;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szChannelName); return 0; }

	CChatChannelPacket() {}
	CChatChannelPacket( const int nClientID, const string &_szChannelName ) 
		: CNetPacket( nClientID ), szChannelName( _szChannelName ) {}
};

//  Chat channel clients list ( for just-joined clients )

struct SIDNickPair
{
	ZDATA
		int nID;
		string szNick;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szNick); return 0; }
};

class CChatChannelClientsListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelClientsListPacket )
public:
	ZDATA
		list<SIDNickPair> clientsList;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&clientsList); return 0; }

	CChatChannelClientsListPacket() {}
	CChatChannelClientsListPacket( const int nClientID, const list<SIDNickPair> &_clientsList ) 
		:	CNetPacket( nClientID ), clientsList( _clientsList ) {}
};

//  client joined/left notification
class CChatClientListChangeNotifyPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatClientListChangeNotifyPacket )
public:
	ZDATA
		int nID;
		string szNick;
		bool bJoined; // true for joining clients, false for leaving
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nID); f.Add(3,&szNick); f.Add(4,&bJoined); return 0; }

	CChatClientListChangeNotifyPacket() {}
	CChatClientListChangeNotifyPacket( const int nClientID, const int _nID, const string &_szNick, bool _bJoined )
		:	CNetPacket( nClientID ), nID( _nID ), szNick( _szNick ), bJoined( _bJoined ) {}	
};

class CChatChannelsListRequestPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelsListRequestPacket )
public:
	ZDATA
		DWORD dwVersion;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); return 0; }

	CChatChannelsListRequestPacket() {}
	CChatChannelsListRequestPacket( const int nClientID, const DWORD _dwVersion )
		: CNetPacket( nClientID ), dwVersion( _dwVersion ) {}
};

class CChatChannelsListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelsListPacket )
public:
	ZDATA
		DWORD dwVersion;
		list<string> added;
		list<string> removed;
		bool bIsFullUpdate;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&dwVersion); f.Add(3,&added); f.Add(4,&removed); f.Add(5,&bIsFullUpdate); return 0; }

	CChatChannelsListPacket() {}
	CChatChannelsListPacket( const int nClientID, const DWORD _dwVersion, const list<string> &_added, 
		const list<string> &_removed, bool _bIsFullUpdate )
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
		string szPlayer;
		EFriendIgnore eChange;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szPlayer); f.Add(3,&eChange); return 0; }

	CChatModifyIgnoreFriendListPacket() {}
	CChatModifyIgnoreFriendListPacket( const int nClientID, const string &_szPlayer, const EFriendIgnore _eChange )
		: CNetPacket( nClientID ), szPlayer( _szPlayer ), eChange( _eChange ) {}
};

class CChatIgnoreFriendListPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatIgnoreFriendListPacket )
public:
	ZDATA
		list< string > ignoreList;
		list< string > friendList;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&ignoreList); f.Add(3,&friendList); return 0; }

	CChatIgnoreFriendListPacket() {}
	CChatIgnoreFriendListPacket( const int nClientID, const list<string> & _ignoreList, const list<string> & _friendList ) 
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
		string szNick;
		char cChatStatus; // см. EChatStatus
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&cChatStatus); return 0; }

	CChatFriendNotifyPacket() {}
	CChatFriendNotifyPacket( const int nClientID, const string &_szNick, const char _cStatus )
		: CNetPacket( nClientID ), szNick( _szNick ), cChatStatus( _cStatus ) {}
};

class CChatChannelByNickPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CChatChannelByNickPacket )
public:
	ZDATA
		string szNick;
		string szChannel;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szNick); f.Add(3,&szChannel); return 0; }
	CChatChannelByNickPacket() {}
	CChatChannelByNickPacket( const int nClientID, const string &_szNick, const string &_szChannel )
		: CNetPacket( nClientID ), szNick( _szNick ), szChannel( _szChannel ) {}
} ;


