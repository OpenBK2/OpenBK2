
#pragma once
#include "MPInterfaceData.h"
#include "Server_Client_Common/LadderStatistics.h"

namespace NDb
{
	struct SMapInfo;
}

enum EMPUIMessageType
{
	EMUI_UNKNOWN,

	// UI-->MP: request for net mode
	EMUI_NO_NET,								// simple
	EMUI_NIVAL_NET,							// simple
	EMUI_LAN_NET,								// simple

	// UI-->MP: request for game actions
	EMUI_CREATE_GAME,
	EMUI_LEAVE_GAME,						// simple, works in mission as well as out of mission
	EMUI_JOIN_GAME,
	EMUI_START_GAME,						// simple
	EMUI_INTERRUPT,							// simple, stop current waiting state (joining game, create game, wait for ladder etc)
	EMUI_MP_PAUSE,							// simple, set/clear mp pause (timed)
	EMUI_BACK_FROM_GAME_LIST,		// simple, request MPManager to issue MainLoop commands to back from GameList (as it differs in different net modes)

	// UI<-->MP: auto updates: UI->MP requests/cancels updates, MP->UI delivers updates
	EMUI_UPDATE_GAME_LIST,

	// UI<-->MP: update player info in Game Room
	EMUI_UPDATE_SLOT,

	// MP-->UI: уведомления и команды об основных состояниях
	EMUI_GAME_ROOM_INIT,				// entering game room: success/fail, host/client, game info, reason for failure
	EMUI_WAITING_FOR_PLAYERS,		// either at the start of game, or when lagging
	EMUI_WAITING_INFO,
	EMUI_GAME_AFTERMATH,				// Hide "Waiting" in MP Statistics

	// UI <--> MP: chat
	EMUI_CHAT_MESSAGE,
	// UI-->MP: in-game chat
	EMUI_IN_GAME_CHAT_MESSAGE,

	// UI-->MP: Nival.Net chat
	EMUI_REQUEST_CHAT_CHANNELS,	// simple
	EMUI_JOIN_CHAT_CHANNEL,

	// UI-->MP: Nival.Net specific
	EMUI_LOGIN_NIVAL_NET,
	EMUI_REGISTER_NIVAL_NET,
	EMUI_NIVAL_NET_CUSTOM_GAME,			// simple
	EMUI_NIVAL_NET_CANCEL_LADDER,		// simple
	EMUI_NIVAL_NET_LADDER_GAME,
	EMUI_REQUEST_INFO,

	// MP-->UI: Nival.Net specific notifications
	EMUI_CONNECT_RESULT,						// also has register results in it
	EMUI_SERVER_MESSAGE,						// AKA "Welcome Message", can happen at any time
	EMUI_CHAT_CHANNELS,
	EMUI_JOINED_CHANNEL,
	EMUI_CHAT_NICKS,
	EMUI_CHAT_NICKS_CHANGE,
	EMUI_SHORT_INFO,
	EMUI_CHANGE_FRIEND_IGNORE,
	EMUI_NIVAL_NET_LADDER_STATS,
	EMUI_NIVAL_NET_LADDER,					// 
};

struct SMPUIMessage : public CObjectBase
{
	ZDATA
	EMPUIMessageType	eMessageType;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eMessageType); return 0; }

	SMPUIMessage() : eMessageType( EMUI_UNKNOWN ) {}
	SMPUIMessage( const EMPUIMessageType _eMessageType ) : eMessageType(_eMessageType) {}
};

// Interface with UI

struct IMPToUIManager : public CObjectBase
{
	enum { tidTypeID = 0x1911A400 };

	virtual void AddUIMessage( SMPUIMessage *pMsg ) = 0;
	virtual void AddUIMessage( EMPUIMessageType eMessageType ) = 0;

	virtual SMPUIMessage* GetUIMessage() = 0;
	virtual SMPUIMessage* PeekUIMessage() = 0;

	virtual void MPUISegment() = 0;
	virtual bool SaveReplay( const std::string &szFileName ) = 0;
};

//*******************************************************************
//*		 									messages from MP to UI and back				  		*
//*******************************************************************

struct SMPUISimpleMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUISimpleMessage );
public:
	SMPUISimpleMessage() {} // serialize only
	SMPUISimpleMessage( const EMPUIMessageType _eMessageType ) : SMPUIMessage ( _eMessageType ) {}
};

struct SMPUILoginNivalNetMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILoginNivalNetMessage );
public:
	ZDATA_(SMPUIMessage)
	std::wstring wszLogin;
	std::wstring wszPassword;
	bool bRememberPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&wszLogin); f.Add(3,&wszPassword); f.Add(4,&bRememberPassword); return 0; }
	
	SMPUILoginNivalNetMessage() {} // serialization only
	SMPUILoginNivalNetMessage( const std::wstring &_wszLogin, const std::wstring &_wszPassword, const bool _bRemember ) :
		SMPUIMessage( EMUI_LOGIN_NIVAL_NET ), wszLogin( _wszLogin ), wszPassword( _wszPassword ), bRememberPassword(_bRemember) {}
};

struct SMPUIJoinGameMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIJoinGameMessage );
public:
	ZDATA_(SMPUIMessage)
	int nGameID;
	std::string szPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&nGameID); f.Add(3,&szPassword); return 0; }
	
	SMPUIJoinGameMessage() {} // serialization only
	SMPUIJoinGameMessage( int _nGameID ) :
	SMPUIMessage( EMUI_JOIN_GAME ), nGameID( _nGameID ) {}
	SMPUIJoinGameMessage( int _nGameID, const std::string &_szPassword ) :
		SMPUIMessage( EMUI_JOIN_GAME ), nGameID( _nGameID ), szPassword( _szPassword ) {}
};

// General info for any game using the same net (same net driver, same net; or on same server)
struct SUIGameInfo
{
	ZDATA
	int					nGameID;
	std::string			szSessionName;
	std::string			szMapName;
	int					nPlayers;
	int					nPlayersMax;
	bool				bPwdReq;
	int					nSizeX;
	int					nSizeY;
	int					nGameType;
	int					nTechLevel;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGameID); f.Add(3,&szSessionName); f.Add(4,&szMapName); f.Add(5,&nPlayers); f.Add(6,&nPlayersMax); f.Add(7,&bPwdReq); f.Add(8,&nSizeX); f.Add(9,&nSizeY); f.Add(10,&nGameType); f.Add(11,&nTechLevel); return 0; }
};

struct SMPUIGameListMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIGameListMessage );

public:
	ZDATA_(SMPUIMessage)
	bool bSendUpdates;
	std::list<SUIGameInfo> gamesAddChange;
	std::list<int> gamesRemoved;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&bSendUpdates); f.Add(3,&gamesAddChange); f.Add(4,&gamesRemoved); return 0; }

	SMPUIGameListMessage() : SMPUIMessage( EMUI_UPDATE_GAME_LIST ), bSendUpdates(false) {}
	SMPUIGameListMessage( const bool _bSendUpdates ) : SMPUIMessage( EMUI_UPDATE_GAME_LIST ), bSendUpdates(_bSendUpdates) {}
};

struct SMPUICreateGameMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUICreateGameMessage );
public:
	ZDATA_(SMPUIMessage)
	SUIGameInfo info;										// General info (name, no. of players, etc)
	SB2GameSpecificData specificInfo;		// Specific info
	std::string szPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&info); f.Add(3,&specificInfo); f.Add(4,&szPassword); return 0; }

	SMPUICreateGameMessage() : SMPUIMessage( EMUI_CREATE_GAME ) {}
};

struct SMPUIUpdateSlotMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIUpdateSlotMessage );

public:
	ZDATA_(SMPUIMessage)
	int			nSlot;
	SMPSlot	info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&nSlot); f.Add(3,&info); return 0; }

	SMPUIUpdateSlotMessage() : SMPUIMessage( EMUI_UPDATE_SLOT ), nSlot(-1) {}
};

struct SMPUIGameRoomInitMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIGameRoomInitMessage );
public:
	enum ERejectReason
	{
		ERR_SUCCESS,
		ERR_GAME_FULL,
		ERR_CHECKSUM,
		ERR_KICKED,
		ERR_GAME_KILLED,
		ERR_CONNECT_FAILED,
	};

	ZDATA_(SMPUIMessage)
	ERejectReason eResult;												// Success in creating/joining?
	bool bHost;
	std::string szSessionName;
	int nOwnSlot;
	SB2GameSpecificData specificInfo;		// Specific info
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&eResult); f.Add(3,&bHost); f.Add(4,&szSessionName); f.Add(5,&nOwnSlot); f.Add(6,&specificInfo); return 0; }

	SMPUIGameRoomInitMessage() : SMPUIMessage( EMUI_GAME_ROOM_INIT ) {}
	SMPUIGameRoomInitMessage( const ERejectReason _eResult ) : SMPUIMessage( EMUI_GAME_ROOM_INIT ), eResult(_eResult) {}
};

struct SMPUILagMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILagMessage );
public:
	ZDATA_(SMPUIMessage)
	DWORD dwLaggingPlayers;
	bool bInitialWait;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&dwLaggingPlayers); f.Add(3,&bInitialWait); return 0; }

	SMPUILagMessage() : SMPUIMessage( EMUI_WAITING_FOR_PLAYERS ), dwLaggingPlayers(0), bInitialWait(false) {}
	SMPUILagMessage( const DWORD dwPlayerMask, const bool bOnStart ) 
		: SMPUIMessage( EMUI_WAITING_FOR_PLAYERS ), dwLaggingPlayers(dwPlayerMask), bInitialWait(bOnStart) {}
};

struct SMPUILagInfoMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILagInfoMessage );
public:
	struct SLagItem
	{
		std::string szName;
		int nSecondsLeft;
	};
	typedef std::list<SLagItem> CLaggerList;
	ZDATA_(SMPUIMessage) 
	bool bOwnLag;					// It is me who is paused, show Resume button
	int nOwnTimeLeft;
	CLaggerList lags;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&bOwnLag); f.Add(3,&nOwnTimeLeft); f.Add(4,&lags); return 0; }

	SMPUILagInfoMessage() : SMPUIMessage( EMUI_WAITING_INFO ), bOwnLag(false) {}
	SMPUILagInfoMessage( const bool _bOwnLag ) 
		: SMPUIMessage( EMUI_WAITING_INFO ), bOwnLag(_bOwnLag) {}
};

struct SMPUILadderGameMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILadderGameMessage );
public:
	ZDATA_(SMPUIMessage)
	int nCountry;
	bool bHistoricity;
	int nTeamSize;
	std::list<int> maps;
	std::list<int> techLevels;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&nCountry); f.Add(3,&bHistoricity); f.Add(4,&nTeamSize); f.Add(5,&maps); f.Add(6,&techLevels); return 0; }

	SMPUILadderGameMessage() : SMPUIMessage( EMUI_NIVAL_NET_LADDER_GAME ) {}
};

struct SMPUIConnectResultMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIConnectResultMessage );
public:
	ZDATA_(SMPUIMessage)
	bool bSuccess;
	std::string szTextTag;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&bSuccess); f.Add(3,&szTextTag); return 0; }

	SMPUIConnectResultMessage() : SMPUIMessage( EMUI_CONNECT_RESULT ), bSuccess(false) {}
	SMPUIConnectResultMessage( const bool _bSuccess, const std::string &_szTextTag )
		: SMPUIMessage( EMUI_CONNECT_RESULT ), bSuccess(_bSuccess), szTextTag(_szTextTag) {}
};

struct SMPUIServerMessageMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIServerMessageMessage );
public:
	ZDATA_(SMPUIMessage)
	std::wstring wszText;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&wszText); return 0; }

	SMPUIServerMessageMessage() : SMPUIMessage( EMUI_SERVER_MESSAGE ) {}
	SMPUIServerMessageMessage( const std::wstring &_wszText ) : SMPUIMessage( EMUI_SERVER_MESSAGE ), wszText(_wszText) {}
};

struct SMPUIChatMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS(SMPUIChatMessage);

public:
	ZDATA_(SMPUIMessage)	
	std::string		szName;
	std::wstring		wszText;
	bool bPrivate;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&szName); f.Add(3,&wszText); f.Add(4,&bPrivate); return 0; }

	SMPUIChatMessage() : SMPUIMessage ( EMUI_CHAT_MESSAGE ) {} // serialize only
	SMPUIChatMessage( const std::wstring &_wszText ): SMPUIMessage( EMUI_CHAT_MESSAGE ), wszText(_wszText), bPrivate(false) {}
	SMPUIChatMessage( const std::string &_szName, const std::wstring &_wszText )
		: SMPUIMessage( EMUI_CHAT_MESSAGE ), szName(_szName), wszText(_wszText), bPrivate(true) {}
};

struct SMPUIJoinChannelMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIJoinChannelMessage );
public:
	ZDATA_(SMPUIMessage)
	std::string szChannel;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&szChannel); return 0; }

	SMPUIJoinChannelMessage(): SMPUIMessage ( EMUI_JOIN_CHAT_CHANNEL ) {} // serialization only
	SMPUIJoinChannelMessage( const std::string &_szChannel ) : SMPUIMessage ( EMUI_JOIN_CHAT_CHANNEL ), szChannel(_szChannel) { }
};

struct SMPUIChatChannelListMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIChatChannelListMessage );
public:
	ZDATA_(SMPUIMessage)
	std::list<std::string> channels;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&channels); return 0; }

	SMPUIChatChannelListMessage(): SMPUIMessage ( EMUI_CHAT_CHANNELS ) {} // serialization only
};

struct SMPUIChatChannelNicksMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIChatChannelNicksMessage );
public:
	enum EListType {
		ELT_CHAT,
		ELT_FRIEND,
		ELT_IGNORE,
	};
	ZDATA_(SMPUIMessage)
	std::list<std::string> nicks;
	EListType eType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&nicks); f.Add(3,&eType); return 0; }

	SMPUIChatChannelNicksMessage(): SMPUIMessage ( EMUI_CHAT_NICKS ) {} // serialization only
};

enum EMPChatStatus { EMPS_ONLINE,	EMPS_OFFLINE,	EMPS_BUSY };

struct SMPUIChatChannelNicksChangeMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIChatChannelNicksChangeMessage );
public:
	ZDATA_(SMPUIMessage)
	std::string szNick;
	EMPChatStatus eStatus;
	bool bFriend;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&szNick); f.Add(3,&eStatus); f.Add(4,&bFriend); return 0; }

	SMPUIChatChannelNicksChangeMessage(): SMPUIMessage ( EMUI_CHAT_NICKS_CHANGE ) {} // serialization only
	SMPUIChatChannelNicksChangeMessage( const std::string &_szNick, const EMPChatStatus _eStatus, const bool _bFriend )
		: SMPUIMessage ( EMUI_CHAT_NICKS_CHANGE ), szNick(_szNick), eStatus(_eStatus), bFriend(_bFriend) {}
};

struct SMPUILadderInfoRequestMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILadderInfoRequestMessage );
public:
	ZDATA_(SMPUIMessage)
	std::string szNick;
	bool bShort;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&szNick); f.Add(3,&bShort); return 0; }

	SMPUILadderInfoRequestMessage(): SMPUIMessage ( EMUI_REQUEST_INFO ) {} // serialization only
	SMPUILadderInfoRequestMessage( const std::string &_szNick, const bool _bShort )
		: SMPUIMessage ( EMUI_REQUEST_INFO ), szNick(_szNick), bShort(_bShort) {}
};

struct SMPUIShortInfoMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIShortInfoMessage );
public:
	ZDATA_(SMPUIMessage)
	int nLevel;
	std::wstring wszRank;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&nLevel); f.Add(3,&wszRank); return 0; }

	SMPUIShortInfoMessage(): SMPUIMessage ( EMUI_SHORT_INFO ) {} // serialization only
};

struct SMPUIChangeFriendIgnoreStatusMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIChangeFriendIgnoreStatusMessage );
public:
	enum EAction {
		EA_ADD_IGNORE,
		EA_REMOVE_IGNORE,
		EA_ADD_FRIEND,
		EA_REMOVE_FRIEND,
	};
	ZDATA_(SMPUIMessage)
	std::string szNick;
	EAction eAction;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&szNick); f.Add(3,&eAction); return 0; }

	SMPUIChangeFriendIgnoreStatusMessage(): SMPUIMessage ( EMUI_CHANGE_FRIEND_IGNORE ) {} // serialization only
	SMPUIChangeFriendIgnoreStatusMessage( const std::string &_szNick, const EAction _eAction )
		: SMPUIMessage ( EMUI_CHANGE_FRIEND_IGNORE ), szNick(_szNick), eAction(_eAction) {}
};

namespace NDb
{
	struct SMedal;
}
struct SMPUIGameAftemathMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIGameAftemathMessage );
public:
	ZDATA_(SMPUIMessage)
	bool bShowLadderInfo;
	int nLevel;
	int nOldLevel;
	int nCountry;
	int nRank;
	int nOldRank;
	int nExpEarned;
	int nExpTotal1;
	int nExpTotal2;
	std::vector< CDBPtr<NDb::SMedal> > medals;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&bShowLadderInfo); f.Add(3,&nLevel); f.Add(4,&nOldLevel); f.Add(5,&nCountry); f.Add(6,&nRank); f.Add(7,&nOldRank); f.Add(8,&nExpEarned); f.Add(9,&nExpTotal1); f.Add(10,&nExpTotal2); f.Add(11,&medals); return 0; }

	SMPUIGameAftemathMessage(): SMPUIMessage ( EMUI_GAME_AFTERMATH ) {} // serialization only
	SMPUIGameAftemathMessage( const bool &_bIsLadder ) : SMPUIMessage ( EMUI_GAME_AFTERMATH ), bShowLadderInfo(_bIsLadder) { }
};

struct SMPUIRegisterMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUIRegisterMessage );
public:
	ZDATA_(SMPUIMessage)
	std::wstring wszName;
	std::wstring wszPassword;
	std::wstring wszCDKey;
	std::wstring wszEmail;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&wszName); f.Add(3,&wszPassword); f.Add(4,&wszCDKey); f.Add(5,&wszEmail); return 0; }

	SMPUIRegisterMessage(): SMPUIMessage ( EMUI_REGISTER_NIVAL_NET ) {} // serialization only
	SMPUIRegisterMessage( const std::wstring &_wszName, const std::wstring &_wszPassword, const std::wstring &_wszCDKey, const std::wstring &_wszEmail )
		: SMPUIMessage ( EMUI_REGISTER_NIVAL_NET ), wszName( _wszName ), wszPassword( _wszPassword ), wszCDKey( _wszCDKey ), wszEmail( _wszEmail ) { }
};

struct SMPUILadderStatsMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILadderStatsMessage );
public:
	ZDATA_(SMPUIMessage)
	SLadderStatistics info;
	int nRank;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&info); f.Add(3,&nRank); return 0; }

	SMPUILadderStatsMessage(): SMPUIMessage ( EMUI_NIVAL_NET_LADDER_STATS ) {} // serialization only
	SMPUILadderStatsMessage( const SLadderStatistics &_info ): SMPUIMessage ( EMUI_NIVAL_NET_LADDER_STATS ), info( _info ) {}
};

struct SMPUILadderStatusChangeMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS( SMPUILadderStatusChangeMessage );
public:
	enum EState
	{
		ELS_SEARCH_STARTED,
		ELS_GAME_FOUND,
		ELS_CANCELLED,
	};
	ZDATA_(SMPUIMessage)
	EState eState;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&eState); return 0; }

	SMPUILadderStatusChangeMessage(): SMPUIMessage ( EMUI_NIVAL_NET_LADDER ) {} // serialization only
	SMPUILadderStatusChangeMessage( const EState _eState ): SMPUIMessage ( EMUI_NIVAL_NET_LADDER ), eState( _eState ) {} // serialization only
};

struct SMPUIInGameChatMessage : public SMPUIMessage
{
	OBJECT_NOCOPY_METHODS(SMPUIInGameChatMessage);

public:
	ZDATA_(SMPUIMessage)	
	std::wstring wszText;
	bool bTeamOnly;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SMPUIMessage*)this); f.Add(2,&wszText); f.Add(3,&bTeamOnly); return 0; }

	SMPUIInGameChatMessage() : SMPUIMessage ( EMUI_IN_GAME_CHAT_MESSAGE ) {} // serialize only
	SMPUIInGameChatMessage( const std::wstring &_wszText, bool _bTeamOnly ):
		SMPUIMessage( EMUI_IN_GAME_CHAT_MESSAGE ), wszText(_wszText), bTeamOnly( _bTeamOnly ) {}
};


