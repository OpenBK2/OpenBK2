#pragma once

#include "Server_Client_Common/PacketProcessor.h"
#include "VersionBaseList.h"
#include "Server_Client_Common/ChatPackets.h"

class CClients;

class CChatLobby : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CChatLobby );
	
	CPtr<CClients> pClients;
	std::unordered_map<int, std::string> clientChannel;
	std::unordered_map<std::string, std::list<int> > channelClients;
	TVersionBaseList<std::string> channels;
	std::unordered_map<int,std::string> clientNicks;

	std::string szCfgFile;
	static std::wstring wszWelcomeText;
	UINT64 nLastRefreshTime;
	int nRefreshTime; // time interval between refreshes
	int nMaxFriends;

	void DeleteOfflineClients();
	void CloseEmptyChannels();
protected:
	void NotifyClientJoinChannel( const int nID, const std::string &szChannelName );
	void NotifyClientLeaveChannel( const int nID, const std::string &szChannelName );
	void NotifyChannelOpened( const std::string &szChannelName );
	void NotifyChannelClosed( const std::string &szChannelName );
	void SendChatPacket( const int nClientID, const std::wstring &wszMessage, const std::string &szFromNick, const int nFromID, bool bIsBroadcast );
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
	CChatLobby( CClients *_pClients, const std::string& szCfgFileName );
	bool CanBePaused() { return true; }

	static void SetWelcomeText( const std::wstring& _wszWelcomeText ) { wszWelcomeText = _wszWelcomeText; }
};



