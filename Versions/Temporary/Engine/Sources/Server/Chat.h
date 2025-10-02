#pragma once

#include "../Server_Client_Common/PacketProcessor.h"
#include "VersionBaseList.h"
#include "../Server_Client_Common/ChatPackets.h"

class CClients;

class CChatLobby : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CChatLobby );
	
	CPtr<CClients> pClients;
	hash_map<int, string> clientChannel;
	hash_map<string, list<int> > channelClients;
	TVersionBaseList<string> channels;
	hash_map<int,string> clientNicks;

	string szCfgFile;
	static wstring wszWelcomeText;
	UINT64 nLastRefreshTime;
	int nRefreshTime; // time interval between refreshes
	int nMaxFriends;

	void DeleteOfflineClients();
	void CloseEmptyChannels();
protected:
	void NotifyClientJoinChannel( const int nID, const string &szChannelName );
	void NotifyClientLeaveChannel( const int nID, const string &szChannelName );
	void NotifyChannelOpened( const string &szChannelName );
	void NotifyChannelClosed( const string &szChannelName );
	void SendChatPacket( const int nClientID, const wstring &wszMessage, const string &szFromNick, const int nFromID, bool bIsBroadcast );
	void SendIgnoreFriendList( const int nClientID );
	void NotifyFriends( const int nClientID, const EChatStatus eStatus, const bool bNewClient );
public:
	bool Segment();
	
	bool ProcessChatPacket( class CChatPacket *pPacket );
	bool ProcessChatAFKPacket( class CChatAFKPacket *pPacket );
	bool ProcessChatChannelPacket( class CChatChannelPacket *pPacket );
	bool ProcessChatChannelsRequestPacket( class CChatChannelsListRequestPacket *pPacket );
	bool ProcessChatModifyIgnoreFriendListPacket( class CChatModifyIgnoreFriendListPacket *pPacket );
	bool ProcessChatGetIgnoreFriendListPacket( class CChatGetIgnoreFriendListPacket *pPacket );
	bool ProcessChatChannelByNickPacket( class CChatChannelByNickPacket *pPacket );

	void Initialize();	
	void ReloadConfig();
	CChatLobby() { Initialize(); }
	CChatLobby( CClients *_pClients, const string& szCfgFileName );
	bool CanBePaused() { return true; }

	static void SetWelcomeText( const wstring& _wszWelcomeText ) { wszWelcomeText = _wszWelcomeText; }
};


