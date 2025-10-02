#include "stdafx.h"
#include "Chat.h"
#include "clients.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Misc/Time64.h"
#include "System/XmlSaver.h"

wstring CChatLobby::wszWelcomeText;

CChatLobby::CChatLobby( CClients *_pClients, const string& _szCfgFileName ) 
	: pClients( _pClients ), szCfgFile( _szCfgFileName )
{
	Initialize();

	ReloadConfig();

	WriteMSG( "Chat subsystem initialized.\n" );
}

void CChatLobby::ReloadConfig()
{
	CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
	NI_ASSERT( stream.IsOk(), StrFmt( "Could not open cfg file: %s", szCfgFile ) );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	NI_ASSERT( pSaver.GetPtr(), "Could not create XML saver" );
	pSaver->Add( "Welcome", &wszWelcomeText );
	pSaver->Add( "ChatSegmentLength", &nRefreshTime );
	pSaver->Add( "MaxFriends", &nMaxFriends );
}

void CChatLobby::Initialize()
{
	REGISTER_PACKET_PROCESSOR( ProcessChatPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatAFKPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatChannelPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatChannelsRequestPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatModifyIgnoreFriendListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatGetIgnoreFriendListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatChannelByNickPacket );

	nLastRefreshTime = 0;
}

bool CChatLobby::Segment()
{
	UINT64 nTime = GetLongTickCount();
	if ( nTime < nLastRefreshTime + nRefreshTime )
		return true;
	nLastRefreshTime = nTime;
	
	DeleteOfflineClients();

	return true;
}

void CChatLobby::DeleteOfflineClients()
{
	list< int > deadClients;
	for ( hash_map<int, string>::iterator it = clientNicks.begin(); it != clientNicks.end(); ++it )
	{
		const int nID = it->first;
		if ( ! pClients->IsOnLine( nID ) )
		{
			deadClients.push_back( nID );
		}
	}

	for ( list<int>::iterator it = deadClients.begin(); it != deadClients.end(); ++it )
	{
		const int nID = *it;
		NotifyFriends( nID, OFFLINE, false );
		if ( clientChannel.find( nID ) != clientChannel.end() )
		{
			const string &szChannel = clientChannel[nID];
			list<int> & channelClientList = channelClients[szChannel];
			channelClientList.remove( nID );
			NotifyClientLeaveChannel( nID, szChannel );
			clientChannel.erase( nID );
		}
		clientNicks.erase( nID );
	}
}

void CChatLobby::CloseEmptyChannels()
{
	list< string > emptyChannels;
	for ( hash_map< string, list<int> >::iterator it = channelClients.begin(); it != channelClients.end(); ++it )
	{
		const list<int> &channelClientList = it->second;
		if ( channelClientList.empty() )
		{
			emptyChannels.push_back( it->first );
		}
	}

	for ( list< string>::iterator it = emptyChannels.begin(); it != emptyChannels.end(); ++it )
	{
		const string &szChannel = *it;
		channelClients.erase( szChannel );
		NotifyChannelClosed( szChannel );
	}
}

bool CChatLobby::ProcessChatPacket( CChatPacket *pPacket )
{
	string szFromNick;
	if ( ! pClients->GetNick( pPacket->nClientID, &szFromNick ) )
		return true;

	int nToID = pPacket->nID;
	if ( ( nToID == 0 ) && ( !pPacket->bIsBroadcast ) )
	{
		if ( ! pClients->GetClientID( pPacket->szNick, &nToID ) )
			return true;
	}

	if ( pPacket->bIsBroadcast )
	{
		if ( clientChannel.find( pPacket->nClientID ) == clientChannel.end() )
			return true;
		const string &szChannel = clientChannel[pPacket->nClientID];
		const list<int> &clientsList = channelClients[szChannel];
		for( list<int>::const_iterator it = clientsList.begin(); it != clientsList.end(); ++it )
		{
			const int nClientID = *it;
			SendChatPacket( nClientID, pPacket->wszMessage, szFromNick, pPacket->nClientID, true );
		}
	}
	else
	{
		SendChatPacket( nToID, pPacket->wszMessage, szFromNick, pPacket->nClientID, false );
	}

	return true;
}

bool CChatLobby::ProcessChatAFKPacket( CChatAFKPacket *pPacket )
{
	const int nID = pPacket->nClientID;
	NotifyFriends( nID, AFK, false );
	if ( clientChannel.find( nID ) != clientChannel.end() )
	{
		const string &szChannel = clientChannel[nID];
		list<int> & channelClientList = channelClients[szChannel];
		channelClientList.remove( nID );
		NotifyClientLeaveChannel( nID, szChannel );
		clientChannel.erase( nID );
		string szNick;
		pClients->GetNick( nID, &szNick );
		clientNicks[ nID ] = szNick;
	}
	return true;
}

bool CChatLobby::ProcessChatChannelPacket( CChatChannelPacket *pPacket )
{
	const int nID = pPacket->nClientID;
	const string &szChannelName = pPacket->szChannelName;
	if ( szChannelName == "" ) 
		return true;

	if ( channelClients.find( szChannelName ) == channelClients.end() )
	{ // New channel creation
		NotifyChannelOpened( szChannelName );
	}

	const bool bNewClient = ( clientNicks.find( nID ) == clientNicks.end() );

	// Leaving current channel
	if ( clientChannel.find( nID ) != clientChannel.end() )
	{
		const string szOldChannelName = clientChannel[nID];
		if ( szOldChannelName == szChannelName )
			return true;
		channelClients[szOldChannelName].remove( nID );
	// Важно, чтобы сам клиент не получал ни одно из уведомлений
		NotifyClientLeaveChannel( nID, szOldChannelName );
		// Joining new channel
		NotifyClientJoinChannel( nID, szChannelName );
		channelClients[szChannelName].push_back( nID );
		clientChannel[nID] = szChannelName;
	}
	else
	{
		NotifyClientJoinChannel( nID, szChannelName );
		if ( bNewClient )
		{
			// Joining new channel
			SendIgnoreFriendList( nID );
			if ( wszWelcomeText.size() > 0 )
			{
				CSystemBroadcastPacket *pWelcomePacket = new CSystemBroadcastPacket( nID, wszWelcomeText );
				PushPacket( pWelcomePacket );
			}
		}
		channelClients[szChannelName].push_back( nID );
		clientChannel[nID] = szChannelName;
		NotifyFriends( nID, ONLINE, bNewClient );
	}

	return true;
}

bool CChatLobby::ProcessChatChannelsRequestPacket( CChatChannelsListRequestPacket *pPacket )
{
	DWORD dwVersion = pPacket->dwVersion;
	list<string> addedChannelsList = channels.GetAddDiff( dwVersion );
	list<string> removedChannelsList = channels.GetRemoveDiff( dwVersion );
	PushPacket( new CChatChannelsListPacket( pPacket->nClientID, channels.GetVersion(), addedChannelsList,
		removedChannelsList, channels.NeedFullUpdate( dwVersion ) ) );
	return true;
}

bool CChatLobby::ProcessChatModifyIgnoreFriendListPacket( CChatModifyIgnoreFriendListPacket *pPacket )
{
	if ( !pClients->IsOnLine( pPacket->nClientID ) )
		return true;
	if ( pPacket->eChange == CChatModifyIgnoreFriendListPacket::ADD_IGNORE )
	{
		if ( ! pClients->InIgnoreFriendList( pPacket->nClientID, pPacket->szPlayer, IGNORE_LIST ) )
			pClients->AddIgnoreFriendPair( pPacket->nClientID, pPacket->szPlayer, IGNORE_LIST );
	}
	else if ( pPacket->eChange == CChatModifyIgnoreFriendListPacket::REMOVE_IGNORE )
	{
		pClients->DeleteIgnoreFriendPair( pPacket->nClientID, pPacket->szPlayer, IGNORE_LIST );
	}
	else if ( pPacket->eChange == CChatModifyIgnoreFriendListPacket::ADD_FRIEND )
	{
		list<string> currentFriendList = pClients->GetIgnoreFriendList( pPacket->nClientID, FRIEND_LIST );
		if ( currentFriendList.size() > nMaxFriends )
			return true;
		if ( ! pClients->InIgnoreFriendList( pPacket->nClientID, pPacket->szPlayer, FRIEND_LIST ) )
		{
			pClients->AddIgnoreFriendPair( pPacket->nClientID, pPacket->szPlayer, FRIEND_LIST );
			char cStatus = OFFLINE;
			int nNotifierID;
			if ( pClients->GetClientID( pPacket->szPlayer, &nNotifierID ) )
			{
				if ( clientChannel.find( nNotifierID ) == clientChannel.end() )
					cStatus = AFK;
				else
					cStatus = ONLINE;
			}
			PushPacket( new CChatFriendNotifyPacket( pPacket->nClientID, pPacket->szPlayer, cStatus ) );
		}
	}
	else if ( pPacket->eChange == CChatModifyIgnoreFriendListPacket::REMOVE_FRIEND )
	{
		pClients->DeleteIgnoreFriendPair( pPacket->nClientID, pPacket->szPlayer, FRIEND_LIST );
	}

	return true;
}

bool CChatLobby::ProcessChatGetIgnoreFriendListPacket( CChatGetIgnoreFriendListPacket *pPacket )
{
	SendIgnoreFriendList( pPacket->nClientID );
	return true;
}

bool CChatLobby::ProcessChatChannelByNickPacket( CChatChannelByNickPacket *pPacket )
{
	int nClientID;
	pClients->GetClientID( pPacket->szNick, &nClientID );
	if ( pClients->IsOnLine( nClientID ) && clientChannel.find( nClientID ) != clientChannel.end() )
	{
		const string &szChannel = clientChannel[ nClientID ];
    PushPacket( new CChatChannelByNickPacket( pPacket->nClientID, pPacket->szNick, szChannel ) );
	}
	else
		PushPacket( new CChatChannelByNickPacket( pPacket->nClientID, pPacket->szNick, "" ) );
	return true;
}

void CChatLobby::NotifyClientJoinChannel( const int nID, const string &szChannelName ) 
{
	const list<int> &clientsList = channelClients[szChannelName];
	list<SIDNickPair> listToSend;
	string szNick;
	pClients->GetNick( nID, &szNick );
	clientNicks[ nID ] = szNick;
	for ( list<int>::const_iterator it = clientsList.begin(); it != clientsList.end(); ++it )
	{
		const int nClientID = *it;

		if ( pClients->IsOnLine( nClientID ) )
		{
			PushPacket( new CChatClientListChangeNotifyPacket( nClientID, nID, szNick, true ) );
			string szInChannelClientNick = clientNicks[ nClientID ];
			SIDNickPair sendPair;
			sendPair.nID = nClientID;
			sendPair.szNick = szInChannelClientNick;
			listToSend.push_back( sendPair );
		}
	}
	SIDNickPair sendPair;
	sendPair.nID = nID;
	sendPair.szNick = szNick;

	listToSend.push_front( sendPair );
	// Sending channel clients list to joined one
	PushPacket( new CChatChannelClientsListPacket( nID, listToSend ) );

}

void CChatLobby::NotifyClientLeaveChannel( const int nID, const string &szChannelName ) 
{
	string szNick = clientNicks[ nID ];
	clientNicks.erase( nID );
	const list<int> &clientsList = channelClients[szChannelName];
	if ( clientsList.empty() )
	{
		channelClients.erase( szChannelName );
		NotifyChannelClosed( szChannelName );
	}
	else
	{
		for ( list<int>::const_iterator it = clientsList.begin(); it != clientsList.end(); ++it )
		{
			const int nClientID = *it;
			PushPacket( new CChatClientListChangeNotifyPacket( nClientID, nID, szNick, false ) );
		}
	}

}

void CChatLobby::NotifyChannelOpened( const string &szChannelName )
{
	channels.Add( szChannelName );
}

void CChatLobby::NotifyChannelClosed( const string &szChannelName ) 
{
	channels.Remove( szChannelName );
}

void CChatLobby::SendChatPacket( const int nClientID, const wstring &wszMessage, const string &szFromNick, const int nFromID, bool bIsBroadcast )
{
	if ( !pClients->InIgnoreFriendList( nClientID, szFromNick, IGNORE_LIST ) )
	{
		PushPacket( new CChatPacket( nClientID, wszMessage, szFromNick, 0, bIsBroadcast ) );
		if ( clientChannel.find( nClientID ) == clientChannel.end() )
		{
			string szToNick;
			if ( pClients->GetNick( nClientID, &szToNick ) )
				PushPacket( new CChatAFKResponsePacket( nFromID, szToNick ) );
		}
	}
}

void CChatLobby::SendIgnoreFriendList( const int nClientID )
{
	const list<string> ignoreList = pClients->GetIgnoreFriendList( nClientID, IGNORE_LIST );
	const list<string> friendList = pClients->GetIgnoreFriendList( nClientID, FRIEND_LIST );
	PushPacket( new CChatIgnoreFriendListPacket( nClientID, ignoreList, friendList ) );
}

void CChatLobby::NotifyFriends( const int nClientID, const EChatStatus eStatus, const bool bNewClient )
{
	const string szMyNick = clientNicks[nClientID];
	for ( hash_map<int,string>::iterator it = clientNicks.begin(); it != clientNicks.end(); ++it )
	{
		const int nNotifierID = it->first;
		if ( pClients->InIgnoreFriendList( nNotifierID, szMyNick, FRIEND_LIST ) )
		{
			PushPacket( new CChatFriendNotifyPacket( nNotifierID, szMyNick, eStatus ) );
		}
	}
	if ( bNewClient )
	{
		list<string> friendList = pClients->GetIgnoreFriendList( nClientID, FRIEND_LIST );
		for ( list<string>::iterator it = friendList.begin(); it != friendList.end(); ++it )
		{
			const string &szNotifierName = *it;
			EChatStatus eChatStatus;
			int nNotifierID;
			if ( !pClients->GetClientID( szNotifierName, &nNotifierID ) )
				eChatStatus = OFFLINE;
			else 
			{
				if ( clientChannel.find( nNotifierID ) != clientChannel.end() )
					eChatStatus = ONLINE;
				else
					eChatStatus = AFK;
			}
			PushPacket( new CChatFriendNotifyPacket( nClientID, szNotifierName, eChatStatus ) );
		}
	}
}


