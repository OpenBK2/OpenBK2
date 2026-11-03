
#pragma once

#include "CommandsHistory.h"
#include "InterfaceScreenBase.h"
#include "port/time.h"

#include <string>
#include <vector>

namespace NGameX
{
struct SReplayPlayerInfo;
}

class CInterfaceReplaySaveLoad : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceReplaySaveLoad );

private:
	struct SReplayEntry
	{
		ZDATA
		std::string szName;
		SSystemTime time;
		CPtr<IWindow> pItem;
		SMultiplayerReplayInfo info;

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &szName );
			f.Add( 3, &time );
			f.Add( 4, &pItem );
			f.Add( 5, &info );
			return 0;
		}
	};

	typedef std::vector<SReplayEntry> CReplayEntries;

	struct SPlayerEntry
	{
		CPtr<IWindow> pItem;
		int nPlayerIndex = -1;

		int operator&( IBinSaver &f )
		{
			f.Add( 2, &pItem );
			f.Add( 3, &nPlayerIndex );
			return 0;
		}
	};

	enum EQuestionMode
	{
		EQM_NONE,
		EQM_OVERWRITE,
		EQM_DELETE,
	};

	ZDATA
	bool bSaveMode = false;
	CReplayEntries replays;
	int nSelected = -1;
	EQuestionMode eQuestion = EQM_NONE;

	// Controls
	CPtr<IWindow> pMain;
	CPtr<IButton> pLoadBtn;
	CPtr<IButton> pSaveBtn;
	CPtr<IButton> pDeleteBtn;
	CPtr<IEditLine> pNameEdit;
	CPtr<ITextView> pHeaderSaveView;
	CPtr<ITextView> pHeaderLoadView;
	CPtr<IScrollableContainer> pReplayList;
	CPtr<IWindow> pReplayItemTemplate;
	CPtr<IWindow> pMinimapPanel;
	CPtr<IMiniMap> pMinimapWnd;
	CPtr<IWindow> pMinimapFlagTemplate;
	CPtr<IWindow> pMinimapNameTemplate;
	CPtr<IScrollableContainer> pPlayerList;
	CPtr<IWindow> pPlayerItemTemplate;
	std::vector<SPlayerEntry> playerEntries;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &bSaveMode );
		f.Add( 3, &replays );
		f.Add( 4, &nSelected );
		f.Add( 5, &pMain );
		f.Add( 6, &pLoadBtn );
		f.Add( 7, &pSaveBtn );
		f.Add( 8, &pDeleteBtn );
		f.Add( 9, &pNameEdit );
		f.Add( 10, &pHeaderSaveView );
		f.Add( 11, &pHeaderLoadView );
		f.Add( 12, &pReplayList );
		f.Add( 13, &pReplayItemTemplate );
		f.Add( 14, &pMinimapPanel );
		f.Add( 15, &pMinimapWnd );
		f.Add( 16, &pMinimapFlagTemplate );
		f.Add( 17, &pPlayerList );
		f.Add( 18, &pPlayerItemTemplate );
		f.Add( 19, &pMinimapNameTemplate );
		f.Add( 20, &playerEntries );
		return 0;
	}

	void MakeInterior();
	void ShowHideControls();
	void GetReplays();
	void PopulateReplayList();
	void CleanSelectionInfo();
	void MakeInfo( const SReplayEntry &entry );
	void MakePlayers( const SMultiplayerReplayInfo &info );
	void AddPlayer( IWindow *pWnd, const NGameX::SReplayPlayerInfo &player );
	int GetSelectedPlayer() const;
	void MakeMinimap( const SMultiplayerReplayInfo &info );
	void DoSaveReplay();

	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	bool OnBack();
	bool OnSave();
	bool OnLoad();
	bool OnDelete();
	bool OnSelect();

public:
	CInterfaceReplaySaveLoad();
	bool Init() override;
	void SetMode( const std::string &szMode );

	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction ) override;
	int Check( const std::string &szCheckName ) const override;
	//}
};

class CICInterfaceReplaySaveLoad : public CInterfaceCommandBase<CInterfaceReplaySaveLoad>
{
	OBJECT_BASIC_METHODS( CICInterfaceReplaySaveLoad );

	ZDATA_( CInterfaceCommandBase<CInterfaceReplaySaveLoad> )
	std::string szMode;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CInterfaceCommandBase<CInterfaceReplaySaveLoad> *)this );
		f.Add( 2, &szMode );
		return 0;
	}

	//
	void PreCreate() override;
	void PostCreate( IInterface *pInterface ) override;

public:
	void Configure( const char *pszConfig ) override;
};
