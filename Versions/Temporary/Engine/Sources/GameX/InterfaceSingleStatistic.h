#pragma once

#include "InterfaceScreenBase.h"
#include "UIElementsHelper.h"

namespace NDb
{
	struct SMedal;
	struct SReinforcement;
	struct SPlayerRank;
	struct SMapPlayerInfo;
}
struct SChapterReinfUpgrade;
struct SChapterReinfComposition;

class CInterfaceSingleStatistic : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceSingleStatistic );

//	class CData;
//	class CDataViewer;
	
	struct SPlayer
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pIconWnd;
		CPtr<ITextView> pNameView;
		CPtr<ITextView> pLostView;
		CPtr<ITextView> pKilledView;
		CPtr<ITextView> pReinfView;
		bool bHasStatistics;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pIconWnd); f.Add(4,&pNameView); f.Add(5,&pLostView); f.Add(6,&pKilledView); f.Add(7,&pReinfView); f.Add(8,&bHasStatistics); return 0; }
	};
	struct SReinf
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pIconWnd;
		CPtr<ITextView> pNameView;
		ZSKIP //CPtr<ITextView> pBaseUnitNameView;
		CDBPtr<NDb::SReinforcement> pReinf;
		CPtr<IButton> pBtn;
		string szButtonName;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pIconWnd); f.Add(4,&pNameView); f.Add(6,&pReinf); f.Add(7,&pBtn); f.Add(8,&szButtonName); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	ZSKIP //CPtr<IButton> pNextBtn;
	ZSKIP //CPtr<IButton> pSaveReplayBtn;
	ZSKIP //CPtr<ITextView> pMissionExpView;
	ZSKIP //CPtr<ITextView> pCampaignExpView;
	ZSKIP //CPtr<IProgressBar> pMissionProgressBar;
	ZSKIP //CPtr<IProgressBar> pCampaignProgressBar;
	ZSKIP //CPtr<ITextView> pMissionTimeView;
	ZSKIP //CPtr<ITextView> pCampaignTimeView;
	ZSKIP //vector<SNewReinf> newReinfs;
	ZSKIP //CObj<CDataViewer> pDataViewer;
	ZSKIP //CPtr<IListControl> pPlayersList;
	ZSKIP //CPtr<ITextView> pCustomPlayerView;
	ZSKIP //CPtr<IWindow> pNewRankDlg;
	ZSKIP //CPtr<ITextView> pNewRankView;
	ZSKIP //CPtr<IWindow> pMedalDialogBgr;
	list< CDBPtr<NDb::SMedal> > pendingMedals;
	ZSKIP //NTimer::STime timeShowNextMedal;
	
	CPtr<IWindow> pInfoPanel;
	CPtr<IWindow> pRewardPanel;
	CPtr<ITextView> pMissionSuccessLabel;
	CPtr<ITextView> pMissionFailedLabel;
	CPtr<IProgressBar> pCareerProgress;
	CPtr<IProgressBar> pRankProgress;
	CPtr<ITextView> pExpView;
	CPtr<ITextView> pMissionTimeView;
	CPtr<ITextView> pCampaignTimeView;
	vector<SPlayer> players;
	vector<SReinf> reinfs;
	wstring wszTime1;
	wstring wszTime2;
	wstring wszTime3;
	
	CPtr<IWindow> pNewRankDlg;
	CPtr<IWindow> pNewRankPanel;
	CPtr<IWindow> pNewRankIconWnd;
	CPtr<ITextView> pNewRankView;

	CPtr<IWindow> pNewMedalDlg;
	CPtr<IWindow> pNewMedalPanel;
	CPtr<ITextView> pMedalNameView;
	CPtr<IWindow> pMedalIconWnd;
	CPtr<IScrollableContainer> pMedalDescCont;
	CPtr<ITextView> pMedalDescView;
	
	CPtr<IWindow> pChapterReinfDescWnd;
	CObj<SChapterReinfUpgrade> pChapterReinfUpgrade;
	ZSKIP //CObj<SChapterReinfComposition> pChapterReinfComposition;

	CDBPtr<NDb::SPlayerRank> pNewPlayerRank;
	list< CDBPtr<NDb::SMedal> > medals;
	
	bool bPopup;
	CPtr<IWindow> pPopupBgWnd;
	ZSKIP //float fCareerProgressWidth;
	ZSKIP //float fRankProgressWidth;
	NUIElementsHelper::SExpProgressCareer expProgressCareer;
	NUIElementsHelper::SExpProgressRank expProgressRank;

	CPtr<IProgressBar> pNewCareerProgress;
	CPtr<IProgressBar> pNewRankProgress;
	CPtr<IButton> pRestartBtn;
	CPtr<IButton> pExitToMainMenuBtn;
	CPtr<IButton> pExitToChapterBtn;
	CPtr<IButton> pLoadBtn;
	ZSKIP //CPtr<IButton> pExitToWindowsBtn;
	CPtr<IWindow> pBottomPanel;
	CPtr<IButton> pNextBtn;
	CPtr<IWindow> pBlackBgWnd;
	CPtr<ITextView> pCareerExpNAView;
	CPtr<ITextView> pMissionExpNAView;
	CPtr<ITextView> pNewReinfLabel;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(18,&pendingMedals); f.Add(20,&pInfoPanel); f.Add(21,&pRewardPanel); f.Add(22,&pMissionSuccessLabel); f.Add(23,&pMissionFailedLabel); f.Add(24,&pCareerProgress); f.Add(25,&pRankProgress); f.Add(26,&pExpView); f.Add(27,&pMissionTimeView); f.Add(28,&pCampaignTimeView); f.Add(29,&players); f.Add(30,&reinfs); f.Add(31,&wszTime1); f.Add(32,&wszTime2); f.Add(33,&wszTime3); f.Add(34,&pNewRankDlg); f.Add(35,&pNewRankPanel); f.Add(36,&pNewRankIconWnd); f.Add(37,&pNewRankView); f.Add(38,&pNewMedalDlg); f.Add(39,&pNewMedalPanel); f.Add(40,&pMedalNameView); f.Add(41,&pMedalIconWnd); f.Add(42,&pMedalDescCont); f.Add(43,&pMedalDescView); f.Add(44,&pChapterReinfDescWnd); f.Add(45,&pChapterReinfUpgrade); f.Add(47,&pNewPlayerRank); f.Add(48,&medals); f.Add(49,&bPopup); f.Add(50,&pPopupBgWnd); f.Add(53,&expProgressCareer); f.Add(54,&expProgressRank); f.Add(55,&pNewCareerProgress); f.Add(56,&pNewRankProgress); f.Add(57,&pRestartBtn); f.Add(58,&pExitToMainMenuBtn); f.Add(59,&pExitToChapterBtn); f.Add(60,&pLoadBtn); f.Add(62,&pBottomPanel); f.Add(63,&pNextBtn); f.Add(64,&pBlackBgWnd); f.Add(65,&pCareerExpNAView); f.Add(66,&pMissionExpNAView); f.Add(67,&pNewReinfLabel); return 0; }
	
	NTimer::STime timePrevPopup; // don't serialize
private:
	void MakeInterior();
	void MakeCustomMissionStatistics();
	void MakeCampaignStatistics();
	void MakePlayerStatistics( bool bChapter );
	void MakePlayerName( const SPlayer &player, const NDb::SMapPlayerInfo &dbPlayer, bool bPlayer, bool bChapter );
	void MakeReinf();
	
	void NextMenu();
	void ShowReinf( const NDb::SReinforcement *pReinf );
	void CheckNextPopup();
	void ShowNewRank( const NDb::SPlayerRank *pRank );
	void ShowMedal( const NDb::SMedal *pMedal );
	void ExpProgressStep();
	
	wstring GetFormattedTime( int nTime ) const;

	bool OnMenuNext();
	bool OnMedalDialogClose();
	bool OnNewRankDialogClose();
	bool OnReinfClick( const string &szSender );
	bool OnChapterReinfClose();
	bool OnChapterReinfUnitBtn( const string &szSender );
	bool OnMenuNextOnEnter();
	bool OnRestartMission();
	bool OnExitToMainMenu();
	bool OnExitToChapter();
	bool OnLoad();
	bool OnExitToWindows();
	
	void MsgOk( const SGameMessage &msg );
	void MsgCancel( const SGameMessage &msg );
	void MsgTryExitWindows( const SGameMessage &msg );

public:
	CInterfaceSingleStatistic();

	bool Init();
	bool StepLocal( bool bAppActive );
	void OnGetFocus( bool bFocus );
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICSingleStatistic : public CInterfaceCommandBase<CInterfaceSingleStatistic>
{
	OBJECT_BASIC_METHODS( CICSingleStatistic );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};

