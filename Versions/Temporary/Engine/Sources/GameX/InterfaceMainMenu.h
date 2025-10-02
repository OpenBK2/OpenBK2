#pragma once

#include "InterfaceScreenBase.h"
#include "PreprogrammedReactionsAndChecks.h"

class CInterfaceMainMenu : public CInterfaceScreenBase, public CProgrammedReactionsAndChecks<CInterfaceMainMenu>
{
	OBJECT_NOCOPY_METHODS( CInterfaceMainMenu );

	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMainWnd;
	CPtr<IWindow> pSinglePlayerMenuWnd;
	CPtr<IButton> pTutorialBtn;
	CPtr<IButton> pLoadGameBtn;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMainWnd); f.Add(3,&pSinglePlayerMenuWnd); f.Add(4,&pTutorialBtn); f.Add(5,&pLoadGameBtn); return 0; }
private:
	void MsgOptionsMenu( const SGameMessage &msg );
	void MsgCreditsScreen( const SGameMessage &msg );
	void MsgCampaignSelectionMenu( const SGameMessage &msg );
	void MsgProfileManagerMenu( const SGameMessage &msg );
	void MsgMPGamesListMenu( const SGameMessage &msg );
	void MsgExit( const SGameMessage &msg );
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgLoadMenu( const SGameMessage &msg );
	void MsgMenuCustomMission( const SGameMessage &msg );
	void MsgEncyclopedia( const SGameMessage &msg );
	void MsgTutorialMenu( const SGameMessage &msg );
	void MsgSinglePlayer( const SGameMessage &msg );
	void MsgQuickLoad( const SGameMessage &msg );
	
	void OnMultiplayer( const string &szSender );
	void OnCustomCampaign( const string &szSender );
	void OnLoadMod( const string &szSender );
	void OnHallOfFame( const string &szSender );

public:	
	CInterfaceMainMenu();

	//
	bool Init();
	
	void OnGetFocus( bool bFocus );
	bool StepLocal( bool bAppActive );
	
	void RegisterObservers();

	bool IsModal();
};

class CICMainMenu : public CInterfaceCommandBase<CInterfaceMainMenu>
{
	OBJECT_BASIC_METHODS( CICMainMenu );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceMainMenu>)
	string szConfig;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceMainMenu>*)this); f.Add(2,&szConfig); return 0; }
private:
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

//INTERFACE_COMMAND_DECLARE( CICMainMenu, CInterfaceMainMenu );

