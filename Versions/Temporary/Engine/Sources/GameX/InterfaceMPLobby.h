#pragma once
 
#include "InterfaceMPBase.h" 
#include "../UI/UIComponents.h" 
#include "ChatControl.h" 
#include "MPLobbyData.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
class CInterfaceMPLobby : public CInterfaceMPScreenBase, public 
IProgrammedReactionsAndChecks 
{ 
	friend class CClientListViewer;
	OBJECT_NOCOPY_METHODS( CInterfaceMPLobby ); 

	typedef hash_map< string, CPtr<CClientListData> > CChatClientsList;

	ZDATA_(CInterfaceMPScreenBase) 
	CPtr<IWindow> pMain; 
	// Chat { 
	CPtr<IEditLine>						pChatInput; 
	CPtr<CChatControlWrapper> pChatOutput; 
	CPtr<IEditLine>						pChannelNameEdit; 
	CPtr<IListControl>				pChannelsList; 
	CPtr<IWindow>							pChannelsButtonDisabler; 
	CPtr<IButton>							pChannelsButton; 
 
	bool bJoiningChannel; 
	string szCurrentChannel; 
	// } Chat 
 
	// Nicks { 
	CPtr<IListControl> pNicksList;
	CChatClientsList chatClients;
	CChatClientsList chatFriends;
	CChatClientsList chatIgnores;
	CPtr<IWindow> pClientInfoWindow;
	string szSelectedClient;
	CPtr<CClientListData> pSelection;
	bool bShowingFriends;
	// } Nicks 

	CPtr<IButton> pLadderButton;
	CPtr<IButton> pCustomButton;
	CPtr<IButton> pCancelButton;
	CPtr<IButton> pStatsButton;
	CPtr<IButton> pBackButton;
	CPtr<ITextView> pWaitingForLadder;
	NTimer::STime timeStartWaiting;
	bool bLadderGameFound;
	CPtr<IButton> pPostMessage;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceMPScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pChatInput); f.Add(4,&pChatOutput); f.Add(5,&pChannelNameEdit); f.Add(6,&pChannelsList); f.Add(7,&pChannelsButtonDisabler); f.Add(8,&pChannelsButton); f.Add(9,&bJoiningChannel); f.Add(10,&szCurrentChannel); f.Add(11,&pNicksList); f.Add(12,&chatClients); f.Add(13,&chatFriends); f.Add(14,&chatIgnores); f.Add(15,&pClientInfoWindow); f.Add(16,&szSelectedClient); f.Add(17,&pLadderButton); f.Add(18,&pCustomButton); f.Add(19,&pCancelButton); f.Add(20,&pStatsButton); f.Add(21,&pWaitingForLadder); f.Add(22,&timeStartWaiting); return 0; }
private: 
	bool OnChatMessage( struct SMPUIChatMessage *pMsg ); 
	bool OnChatChannelsListMessage( struct SMPUIChatChannelListMessage *pMsg ); 
	bool OnChatChannelNicksMessage( struct SMPUIChatChannelNicksMessage *pMsg ); 
	bool OnChatChannelNicksChangeMessage( struct SMPUIChatChannelNicksChangeMessage *pMsg ); 
	bool OnShortInfoMessage( struct SMPUIShortInfoMessage *pMsg ); 
	bool OnLadderMessage( struct SMPUILadderStatusChangeMessage *pMsg ); 
 
	//{ Reactions 
	//  { Buttons 
	bool OnBackReaction(); 
	bool OnLadderGameReaction(); 
	bool OnCustomGameReaction(); 
	bool OnLadderCancelReaction(); 
	bool OnLadderStatsReaction(); 
	bool OnFriendsSwitch( bool bShowFriends );
	//  } Buttons 
	//  { Chat 
	bool OnPostChatMessageReaction(); 
	bool OnChatEscape(); 
	bool OnChatJoinNamedChannel(); 
	bool OnChatSelectChannel(); 
	bool OnChatCancelJoinChannel(); 
	bool OnChatShowChannels(); 
	bool OnChatHideChannels(); 
	//  } Chat 
	//  { Clients
	bool OnClientMenu( const string &szSender );
	bool OnClientInfoClose( const bool bApply );
	bool OnClientInfoFriendChange();
	bool OnClientInfoIgnoreChange();
	bool OnClientSelected();
	//  } Clients
	//}  
 
	void InitControls(); 
	void TryToJoinChatChannel( const string &szChannel );
	void RebuildClientList();
	int GetButtonState( const string &szNick );

protected: 
	//{ IProgrammedReactonsAndChecks 
	bool Execute( const string &szSender, const string &szReaction ); 
	int Check( const string &szCheckName ) const;	 
	//} 
 
public: 
	CInterfaceMPLobby(); 
	bool Init(); 

	bool StepLocal( bool bAppActive );
}; 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
#ifndef _SINGLE_DEMO
class CICMPLobby : public CInterfaceCommandBase<CInterfaceMPLobby> 
{ 
	OBJECT_BASIC_METHODS( CICMPLobby ); 
	 
	void PreCreate(); 
	void PostCreate( IInterface *pInterface ); 
public: 
	void Configure( const char *pszConfig ); 
}; 
#endif // _SINGLE_DEMO
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

