#pragma once

#include "InterfaceMPBase.h"
#include "ScenarioTracker.h"

#include <cstdint>

namespace NDb
{
	struct SMedal;
	struct SMultiplayerConsts;
}

class CInterfaceMPStatistics : public CInterfaceMPScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceMPStatistics );

	struct SPlayerItemData
	{
		std::wstring wszName;
		int nCountry;
		uint32_t dwColour;
		int nTeam;
		int nUnitsLost;
		int nUnitsKilled;
		int nScoreTactics;
		int nScoreStrategy;
		int nScore;
	};

	ZDATA_(CInterfaceScreenBase)
	CDBPtr<NDb::SMultiplayerConsts> pMPConsts;
	CPtr<IWindow> pMain;
	CPtr<IButton> pNextBtn;
	CPtr<IButton> pSaveReplayBtn;

	CPtr<ITextView> pPlayerNameView;
	CPtr<ITextView> pMissionTimeView;

	CPtr<ITextView> pHeaderWinView;
	CPtr<ITextView> pHeaderLostView;
	CPtr<IScrollableContainer> pPlayerList;
	CPtr<IWindow> pPlayerListDelimiter;
	CPtr<IWindow> pPlayerListItemTemplate;

	CPtr<IWindow> pMedalPopup;
	CPtr<IWindow> pRankPopup;
	std::vector< CDBPtr<NDb::SMedal> > medals;
	int nNextMedal;

	std::list<SPlayerItemData> wonTeam;
	std::list<SPlayerItemData> lostTeam;
	CPtr<IWindow> pWaiting;

	// Switchable
	CPtr<ITextView> pPlayerRankLabel;
	CPtr<ITextView> pPlayerRankView;
	CPtr<ITextView> pPlayerLevelLabel;
	CPtr<ITextView> pPlayerLevelView;
	CPtr<ITextView> pExpEarnedLabel;
	CPtr<ITextView> pExpEarnedView;
	CPtr<ITextView> pExpTotalLabel;
	CPtr<ITextView> pExpTotalView;
	CPtr<IProgressBar> pExpTotalProgress;

	bool bWasLadderGame;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMPConsts); f.Add(3,&pMain); f.Add(4,&pNextBtn); f.Add(5,&pSaveReplayBtn); f.Add(6,&pPlayerNameView); f.Add(7,&pMissionTimeView); f.Add(8,&pHeaderWinView); f.Add(9,&pHeaderLostView); f.Add(10,&pPlayerList); f.Add(11,&pPlayerListDelimiter); f.Add(12,&pPlayerListItemTemplate); f.Add(13,&pMedalPopup); f.Add(14,&pRankPopup); f.Add(15,&medals); f.Add(16,&nNextMedal); f.Add(17,&wonTeam); f.Add(18,&lostTeam); f.Add(19,&pWaiting); f.Add(20,&pPlayerRankLabel); f.Add(21,&pPlayerRankView); f.Add(22,&pPlayerLevelLabel); f.Add(23,&pPlayerLevelView); f.Add(24,&pExpEarnedLabel); f.Add(25,&pExpEarnedView); f.Add(26,&pExpTotalLabel); f.Add(27,&pExpTotalView); f.Add(28,&pExpTotalProgress); f.Add(29,&bWasLadderGame); return 0; }
private:
	void MakeInterior();
	void FillTeams();
	void PopulateList();
	void AddPlayerToTeam( const IScenarioTracker::SMultiplayerInfo::SPlayer &player, std::list<SPlayerItemData> &team );
	void AddPlayerItemsToList( std::list<SPlayerItemData> &team );

	void MsgNext( const SGameMessage &msg );

	bool OnGameAftermathMessage( struct SMPUIGameAftemathMessage *pMsg );

	bool OnClosePopup();
	bool OnSaveReplay();
	void ShowLevelPopup( int nLevel, int nLevelOld, int nCountry, int nRank, int nRankOld );
	void ShowMedalPopup( const NDb::SMedal *pMedal );
public:
	CInterfaceMPStatistics();

	bool Init();
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
};

class CICMPStatistics : public CInterfaceCommandBase<CInterfaceMPStatistics>
{
	OBJECT_BASIC_METHODS( CICMPStatistics );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


