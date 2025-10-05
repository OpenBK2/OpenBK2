
#pragma once

#include "InterfaceScreenBase.h"
#include "SaveLoadHelper.h"

namespace NDb
{
	struct SCampaign;
	struct SChapter;
	struct SMapInfo;
}

class CInterfaceSaveLoadMenu : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceSaveLoadMenu );
public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS( CReactions );
		
		ZDATA
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceSaveLoadMenu> pInterface;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pScreen); f.Add(3,&pInterface); return 0; }
	public:
		CReactions() { }
		CReactions( IWindow *_pScreen, CInterfaceSaveLoadMenu *_pInterface ) : 
			pScreen( _pScreen ), pInterface( _pInterface ) { }
		~CReactions()	{ }
		
		bool Execute( const std::string &szSender, const std::string &szReaction );
		int Check( const std::string &szCheckName ) const;
	};
	
	enum ESortModes
	{
		SORT_BY_NAME = 1,
		SORT_BY_DATE = 2,
		SORT_BY_TIME = 3
	};

	enum EConfirmAction
	{
		CONFIRM_SAVE,
		CONFIRM_DELETE,
		CONFIRM_LOAD,
		NO_SAVE_NAME,
	};
	
	enum EPrevInterface
	{
		EPI_ANY,
		EPI_MAIN_MENU,
		EPI_CHAPTER_MAP,
		EPI_LOOSE_MENU,
	};
	
private:
	struct SSortByName
	{
		bool operator()( const NSaveLoad::SSavegameEntry &save1, const NSaveLoad::SSavegameEntry &save2 );
	};
	struct SSortByDate
	{
		bool operator()( const NSaveLoad::SSavegameEntry &save1, const NSaveLoad::SSavegameEntry &save2 );
	};
	
	ZDATA_(CInterfaceScreenBase)
	CObj<CReactions> pReactions;

	EConfirmAction eConfirmAction;
	std::string				szPath;
	NSaveLoad::CSaveList			saves;

	CObj<CBackgroundMutableTexture>	pScreenShotTexture;

	int						nSelected;
	CPtr<IWindow> pMain;
	CPtr<IWindow> pLoad;
	CPtr<IWindow> pSave;
	CPtr<IWindow> pPicture;
	CPtr<IEditLine> pEditLine;
	CPtr<IWindow> pListTemplate;
	CPtr<IWindow> pListItemTemplate;
	CPtr<IScrollableContainer> pSaveList;
	CPtr<ITextView> pHeaderSaveView;
	CPtr<ITextView> pHeaderLoadView;

	CPtr<IWindow> pDescPanel;
	CPtr<ITextView> pDescCampaignView;
	CPtr<ITextView> pDescChapterView;
	CPtr<ITextView> pDescMissionView;
	CPtr<IScrollableContainer> pDescMissionBriefingCont;
	CPtr<ITextView> pDescMissionBriefingView;
	
	CPtr<IWindow> pDescCustomPanel;
	CPtr<ITextView> pDescCustomMissionView;
	CPtr<IScrollableContainer> pDescCustomMissionBriefingCont;
	CPtr<ITextView> pDescCustomMissionBriefingView;
	
	CPtr<IWindow> pDescChapterPanel;
	CPtr<ITextView> pDescChapterCampaignView;
	CPtr<ITextView> pDescChapterChapterView;
	CPtr<IScrollableContainer> pDescChapterBriefingCont;
	CPtr<ITextView> pDescChapterBriefingView;
	
	bool bConfirmLoadQuestion;
	int nLastID;
	
	CPtr<IWindow> pChapterTexture;
	CPtr<IWindow> pChapterTextureBorder;
	CPtr<IButton> pDeleteBtn;
	CPtr<IWindow> pListPanel;
	EPrevInterface ePrevInterface;
	
	NSaveLoad::SSaveInfo startSaveInfo;
	bool bLoadMode;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pReactions); f.Add(3,&eConfirmAction); f.Add(4,&szPath); f.Add(5,&saves); f.Add(6,&pScreenShotTexture); f.Add(7,&nSelected); f.Add(8,&pMain); f.Add(9,&pLoad); f.Add(10,&pSave); f.Add(11,&pPicture); f.Add(12,&pEditLine); f.Add(13,&pListTemplate); f.Add(14,&pListItemTemplate); f.Add(15,&pSaveList); f.Add(16,&pHeaderSaveView); f.Add(17,&pHeaderLoadView); f.Add(18,&pDescPanel); f.Add(19,&pDescCampaignView); f.Add(20,&pDescChapterView); f.Add(21,&pDescMissionView); f.Add(22,&pDescMissionBriefingCont); f.Add(23,&pDescMissionBriefingView); f.Add(24,&pDescCustomPanel); f.Add(25,&pDescCustomMissionView); f.Add(26,&pDescCustomMissionBriefingCont); f.Add(27,&pDescCustomMissionBriefingView); f.Add(28,&pDescChapterPanel); f.Add(29,&pDescChapterCampaignView); f.Add(30,&pDescChapterChapterView); f.Add(31,&pDescChapterBriefingCont); f.Add(32,&pDescChapterBriefingView); f.Add(33,&bConfirmLoadQuestion); f.Add(34,&nLastID); f.Add(35,&pChapterTexture); f.Add(36,&pChapterTextureBorder); f.Add(37,&pDeleteBtn); f.Add(38,&pListPanel); f.Add(39,&ePrevInterface); f.Add(40,&startSaveInfo); f.Add(41,&bLoadMode); return 0; }
private:
	void Load();
protected:
	void MsgBack( const SGameMessage &msg );
	void MsgDelete( const SGameMessage &msg );
	void MsgLoad( const SGameMessage &msg );
	void MsgSave( const SGameMessage &msg );
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgDblClick( const SGameMessage &msg );
	void MsgSelectPrev( const SGameMessage &msg );
	void MsgSelectNext( const SGameMessage &msg );
	
	void OnSelectionChange( const std::string &szSender );
	
	void FillSaveList( const ESortModes nSortMode );
	void DoSaveGame();
	void ComposeDescription( const NSaveLoad::SSaveInfo &info );
	void ShowSaveInfo( NSaveLoad::SSaveInfo &info );
	
	void SelectItem( bool bUpdateSelection, bool bClearEditLine );
	void UpdateButtons();
	void MakeInterior();
	
	~CInterfaceSaveLoadMenu();
public:
	CInterfaceSaveLoadMenu();

	bool Init();

	void OnGetFocus( bool bFocus );

	bool ProcessEvent( const SGameMessage &msg );

	void SetMode( const std::string &szMode );

public:
	bool StepLocal( bool bAppActive );
};

class CICInterfaceSaveLoad : public CInterfaceCommandBase<CInterfaceSaveLoadMenu>
{
	OBJECT_BASIC_METHODS( CICInterfaceSaveLoad );
	
	ZDATA_(CInterfaceCommandBase<CInterfaceSaveLoadMenu>)
	std::string szMode;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceCommandBase<CInterfaceSaveLoadMenu>*)this); f.Add(2,&szMode); return 0; }
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


