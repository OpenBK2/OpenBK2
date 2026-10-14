
#pragma once
#include "InterfaceScreenBase.h"

class CInterfaceEscMenu : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceEscMenu );

public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS( CReactions );
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceEscMenu> pInterface;

	public:
		CReactions() {}

		~CReactions() override {}

		CReactions( IWindow *_pScreen, CInterfaceEscMenu *_pInterface )
			: pScreen( _pScreen )
			, pInterface( _pInterface )
		{
		}

		bool Execute( const std::string &szSender, const std::string &szReaction ) override;
		int Check( const std::string &szCheckName ) const override;

		int operator&( IBinSaver &saver ) override
		{
			saver.Add( 1, &pScreen );
			saver.Add( 2, &pInterface );
			return 0;
		}
	};

private:
	enum EMode
	{
		EMODE_UNKNOWN,
		EMODE_EXIT_MAIN_MENU,
		EMODE_EXIT_WINDOWS,
		EMODE_RESTART_MISSION,
		EMODE_FINISH_REPLAY,
	};

	ZDATA_( CInterfaceScreenBase )
	CObj<CReactions> pReactions;
	EMode eMode;
	bool bClosed;
	CPtr<IWindow> pEscMenu;
	CPtr<IWindow> pMPEscMenu;
	CPtr<IWindow> pEndMissionSubMenu;
	CPtr<IWindow> pMPEndMissionSubMenu;

	CPtr<IButton> pEndMissionExitToMainMenuBtn;
	CPtr<IButton> pEndMissionExitToChapterMapBtn;
	CPtr<IButton> pLoadBtn;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceScreenBase *)this );
		f.Add( 2, &pReactions );
		f.Add( 3, &eMode );
		f.Add( 4, &bClosed );
		f.Add( 5, &pEscMenu );
		f.Add( 6, &pMPEscMenu );
		f.Add( 7, &pEndMissionSubMenu );
		f.Add( 8, &pMPEndMissionSubMenu );
		f.Add( 9, &pEndMissionExitToMainMenuBtn );
		f.Add( 10, &pEndMissionExitToChapterMapBtn );
		f.Add( 11, &pLoadBtn );
		return 0;
	}

private:
	void MsgTryExitMainMenu( const SGameMessage &msg );
	void MsgTryExitWindows( const SGameMessage &msg );
	void MsgGoMainMenu( const SGameMessage &msg );
	void MsgOptionsMenu( const SGameMessage &msg );
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgWinned( const SGameMessage &msg );
	void OnSave( const std::string &szSender );
	void OnLoad( const std::string &szSender );
	bool OnRestartMission( const std::string &szSender );
	bool OnEndMissionSubMenu( const std::string &szSender );
	bool OnEndMissionReturnToGame( const std::string &szSender );
	bool OnMenuBack( const std::string &szSender );
	bool OnMPMenuBack( const std::string &szSender );
	bool OnFinishReplay();

	void MsgMultiplayerEndMission( const SGameMessage &msg );
	void MsgHelpButton( const SGameMessage &msg );

	void DoRestartMission();

	void GoMainMenu();
	void GoChapterMap();

protected:
	~CInterfaceEscMenu() override;

public:
	CInterfaceEscMenu();

	bool Init() override;

	void OnGetFocus( bool bFocus ) override;

	bool ProcessEvent( const SGameMessage &msg ) override;

	bool StepLocal( bool bAppActive ) override;
};

class CICEscMenu : public CInterfaceCommandBase<CInterfaceEscMenu>
{
	OBJECT_BASIC_METHODS( CICEscMenu );
	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};

//INTERFACE_COMMAND_DECLARE( CICEscMenu, CInterfaceEscMenu );
