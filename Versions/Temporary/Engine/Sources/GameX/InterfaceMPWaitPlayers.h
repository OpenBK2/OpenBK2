
#pragma once

#include "InterfaceMPBase.h"

class CInterfaceMPWaitPlayers : public CInterfaceMPScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPWaitPlayers );

	bool bOwnLag;
	CPtr<IButton> pResumeButton;
	CPtr<ITextView> pOwnTimeLabel;
	CPtr<ITextView> pOwnTime;
	CPtr<IScrollableContainer> pPlayerList;
	CPtr<IWindow> pPlayerListItem;

private:
	void RegisterObservers();
	void MsgOnMultiplayerPause( const SGameMessage &msg );

	bool OnLagInfoMessage( SMPUILagInfoMessage *pMsg );
	void AddPlayerLine( const string &szName, const int nTime );
public:
	CInterfaceMPWaitPlayers();

	bool Init();

	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
	int operator&( IBinSaver &f ) { return 0; }
};

class CICMPWaitPlayers : public CInterfaceCommandBase<CInterfaceMPWaitPlayers>
{
	OBJECT_BASIC_METHODS( CICMPWaitPlayers );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


