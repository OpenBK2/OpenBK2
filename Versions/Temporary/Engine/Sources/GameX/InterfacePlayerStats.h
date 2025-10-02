#pragma once

#include "InterfaceScreenBase.h"
#include "UIElementsHelper.h"

class CInterfacePlayerStats : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfacePlayerStats );
private:
	struct SMedal
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pIconWnd;
		CPtr<ITextView> pNameView;
		CDBPtr<NDb::STexture> pIcon;
		CDBPtr<NDb::STexture> pPicture;
		wstring wszName;
		wstring wszDesc;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pIconWnd); f.Add(4,&pNameView); f.Add(5,&pIcon); f.Add(6,&pPicture); f.Add(7,&wszName); f.Add(8,&wszDesc); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	CPtr<IWindow> pHeaderPanel;
	CPtr<IWindow> pBottomPanel;
	CPtr<IWindow> pPlayerInfoPanel;
	CPtr<IWindow> pMedalInfoPanel;
	CPtr<IWindow> pMedalListPanel;
	
	CPtr<ITextView> pPlayerNameView;
	CPtr<IWindow> pPlayerRankPicture;
	CPtr<ITextView> pUnitsKilledView;
	CPtr<ITextView> pUnitsLostView;
	CPtr<ITextView> pMissionsPassedView;
	CPtr<ITextView> pInGameTimeView;
	CPtr<ITextView> pFavoriteReinfView;
	
	CPtr<ITextView> pMedalNameView;
	CPtr<IWindow> pMedalPicture;
	CPtr<ITextView> pMedalDescView;

	vector<SMedal> medals;
	CPtr<IScrollableContainer> pMedalCont;
	CPtr<IWindow> pMedalItem;
	CPtr<ITextView> pRankNameView;

	CPtr<IProgressBar> pBaseCareerBar;
	CPtr<IProgressBar> pNewCareerBar;
	CPtr<IProgressBar> pBaseNextRankBar;
	CPtr<IProgressBar> pNewNextRankBar;
	CPtr<ITextView> pNextRankProgressView;

	NUIElementsHelper::SExpProgressCareer expProgressCareer;
	NUIElementsHelper::SExpProgressRank expProgressRank;
	float fLastVisiblePlayerStatsExpCareer;
	float fLastVisiblePlayerStatsExpNextRank;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pHeaderPanel); f.Add(4,&pBottomPanel); f.Add(5,&pPlayerInfoPanel); f.Add(6,&pMedalInfoPanel); f.Add(7,&pMedalListPanel); f.Add(8,&pPlayerNameView); f.Add(9,&pPlayerRankPicture); f.Add(10,&pUnitsKilledView); f.Add(11,&pUnitsLostView); f.Add(12,&pMissionsPassedView); f.Add(13,&pInGameTimeView); f.Add(14,&pFavoriteReinfView); f.Add(15,&pMedalNameView); f.Add(16,&pMedalPicture); f.Add(17,&pMedalDescView); f.Add(18,&medals); f.Add(19,&pMedalCont); f.Add(20,&pMedalItem); f.Add(21,&pRankNameView); f.Add(22,&pBaseCareerBar); f.Add(23,&pNewCareerBar); f.Add(24,&pBaseNextRankBar); f.Add(25,&pNewNextRankBar); f.Add(26,&pNextRankProgressView); f.Add(27,&expProgressCareer); f.Add(28,&expProgressRank); f.Add(29,&fLastVisiblePlayerStatsExpCareer); f.Add(30,&fLastVisiblePlayerStatsExpNextRank); return 0; }
private:
	void MakeInterior();
	void InitInteriorView();
	void ExpProgressStep();

	bool OnMenuBack();
	bool OnSelectMedal();

	void SelectMedal( int nIndex, bool bForce );
public:
	CInterfacePlayerStats();

	bool Init();
	bool StepLocal( bool bAppActive );
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const string &szSender, const string &szReaction );
	int Check( const string &szCheckName ) const;
	//}
};

class CICPlayerStats : public CInterfaceCommandBase<CInterfacePlayerStats>
{
	OBJECT_BASIC_METHODS( CICPlayerStats );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


