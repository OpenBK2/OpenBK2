
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
	void AddPlayerLine( const std::string &szName, const int nTime );

public:
	CInterfaceMPWaitPlayers();

	bool Init() override;

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;

	//}
	int operator&( IBinSaver &f ) override
	{
		return 0;
	}
};

class CICMPWaitPlayers : public CInterfaceCommandBase<CInterfaceMPWaitPlayers>
{
	OBJECT_BASIC_METHODS( CICMPWaitPlayers );
	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
