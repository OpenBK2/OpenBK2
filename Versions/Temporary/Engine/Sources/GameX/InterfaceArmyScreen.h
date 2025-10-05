#pragma once

#include "InterfaceScreenBase.h"
#include "ScenarioTracker.h"

class CInterfaceArmyScreen : public CInterfaceScreenBase, public IProgrammedReactionsAndChecks
{
	OBJECT_NOCOPY_METHODS( CInterfaceArmyScreen );
public:
	class CReinfViewer;
	class CReinfData;
	class CLeaderInfo;
private:
	struct SUnit
	{
		ZDATA
		std::string szBtnName;
		CPtr<IButton> pBtn;
		CPtr<IWindow> pIcon;
		CDBPtr<NDb::SHPObjectRPGStats> pDBStats;
		CPtr<ITextView> pCountView;
		CPtr<IWindow> pBgWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&szBtnName); f.Add(3,&pBtn); f.Add(4,&pIcon); f.Add(5,&pDBStats); f.Add(6,&pCountView); f.Add(7,&pBgWnd); return 0; }
	};
	
	struct SVisAbility
	{
		ZDATA
		ZSKIP //CPtr<IWindow> pIcon;
		ZSKIP //CPtr<IWindow> pEmpty;
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pIconWnd;
		CPtr<ITextView> pRankView;
		CPtr<ITextView> pNameView;
		CPtr<ITextView> pRankLabel;
		ZEND int operator&( IBinSaver &f ) { f.Add(4,&pWnd); f.Add(5,&pIconWnd); f.Add(6,&pRankView); f.Add(7,&pNameView); f.Add(8,&pRankLabel); return 0; }
	};
	
	ZDATA_(CInterfaceScreenBase)
	CPtr<IWindow> pMain;
	CPtr<IWindow> pRightPanel;
	ZSKIP //CPtr<IWindow> pPromotionsPanel;
	ZSKIP //CPtr<IWindow> pReinfHeaderPanel;
	ZSKIP //CPtr<IWindow> pLeaderPanel;
	ZSKIP //CPtr<IWindow> pReinfInfoPanel;
	CPtr<IListControl> pReinfList;
	CPtr<ITextView> pPromotionsView;

	CPtr<ITextView> pLeaderNameView;
	ZSKIP //CPtr<IWindow> pLeaderIconWnd;
	ZSKIP //CPtr<IWindow> pLeaderQuestionIconWnd;
	ZSKIP //CPtr<ITextView> pLeaderExpLabel;
	ZSKIP //CPtr<ITextView> pLeaderRankLabel;
	ZSKIP //CPtr<ITextView> pLeaderSpecLabel;
	ZSKIP //CPtr<ITextView> pLeaderKilledLabel;
	ZSKIP //CPtr<ITextView> pLeaderLostLabel;
	CPtr<IProgressBar> pLeaderExpBar;
	CPtr<ITextView> pLeaderRankView;
	CPtr<ITextView> pLeaderSpecView;
	CPtr<ITextView> pLeaderKilledView;
	CPtr<ITextView> pLeaderLostView;
	ZSKIP //vector< CPtr<IWindow> > leaderAbilities;
	
	CObj<IDataViewer> pReinfViewer;
	
	std::vector< CObj<CReinfData> > reinforcements;
	ZSKIP //int nPromotionsAvailable;
	ZSKIP //int nSelection;
	CPtr<CReinfData> pSelection;
	CPtr<IWindow> pLeaderExpWnd;
	CPtr<IWindow> pAssignLeaderDlg;
	CPtr<IEditLine> pAssignLeaderEdit;
	IScenarioTracker::SGenerateLeaderInfo generateLeaderInfo;

	CPtr<ITextView> pReinfInfoNameView;
	CPtr<IWindow> pReinfInfoIcon;
	ZSKIP //vector< CPtr<IWindow> > reinfInfoUnits;
	ZSKIP //CPtr<ITextView> pReinfInfoUnitNameView;
	ZSKIP //CPtr<ITextView> pReinfInfoUnitStatsLabel;
	ZSKIP //CPtr<IWindow> pReinfInfoHP;
	ZSKIP //CPtr<IWindow> pReinfInfoArmor;
	ZSKIP //CPtr<IWindow> pReinfInfoDamage;
	ZSKIP //CPtr<IWindow> pReinfInfoPenetration;
	ZSKIP //CPtr<ITextView> pReinfInfoHPView;
	ZSKIP //CPtr<ITextView> pReinfInfoArmorView;
	ZSKIP //CPtr<ITextView> pReinfInfoDamageView;
	ZSKIP //CPtr<ITextView> pReinfInfoPenetrationView;
	ZSKIP //CPtr<IWindow3DControl> pReinfInfo3DCtrl;
	ZSKIP //vector< SUnit > reinfInfoUnits;
	ZSKIP //CDBPtr<NDb::SHPObjectRPGStats> pSelectedUnit;
	ZSKIP //CPtr<IWindow> pReinfInfoLeaderIcon;
	std::vector<SVisAbility> visAbilities;
	
	ZSKIP //bool bIgnoreNextMenuBack;
	ZSKIP //int nEffectCounter;
	ZSKIP //bool bEnterState;
	ZSKIP //CPtr<IWindow> pLeaderDisabledQuestionIconWnd;
	ZSKIP //CPtr<IWindow> pLeaderDisabledQuestionIconWnd2;
	ZSKIP //CPtr<IWindow> pReinfInfoUnitHelp;
	ZSKIP //EState eState;
	
	CPtr<IWindow> pLeftPanel;
	CPtr<IWindow> pTopPanel;
	CPtr<IWindow> pBottomPanel;
	ZSKIP //CPtr<IWindow> pScreenBorderWnd;
	
	CPtr<IWindow> pCommanderInfoBlock;
	CPtr<IWindow> pReinfCurrentBlock;
	CPtr<IWindow> pPictureBlock;
	
	CPtr<ITextView> pPromoteCommanderView;
	CPtr<IWindow> pPermanentCommanderWnd;
	CPtr<IWindow> pPermanentCommanderIcon;
	CPtr<IButton> pSelectCommanderBtn;
	CPtr<IWindow> pSelectCommanderIcon;
	CPtr<IButton> pUnselectCommanderBtn;
	CPtr<IWindow> pUnselectCommanderIcon;

	CPtr<ITextView> pProfileUnitNameView;
	CPtr<ITextView> pProfileUnitExpView;
	
	CPtr<IButton> pUndoBtn;
	int nAssignmentCount;
	
	CPtr<IWindow> pAutoAssignDlg;
	CPtr<IWindow> pUndoPromotionsDlg;
	CPtr<IWindow> pUndoAllPromotionsDlg;
	std::wstring wszChangedValueTag;
	CPtr<ITextView> pLeaderRankLabel;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pMain); f.Add(3,&pRightPanel); f.Add(8,&pReinfList); f.Add(9,&pPromotionsView); f.Add(10,&pLeaderNameView); f.Add(18,&pLeaderExpBar); f.Add(19,&pLeaderRankView); f.Add(20,&pLeaderSpecView); f.Add(21,&pLeaderKilledView); f.Add(22,&pLeaderLostView); f.Add(24,&pReinfViewer); f.Add(25,&reinforcements); f.Add(28,&pSelection); f.Add(29,&pLeaderExpWnd); f.Add(30,&pAssignLeaderDlg); f.Add(31,&pAssignLeaderEdit); f.Add(32,&generateLeaderInfo); f.Add(33,&pReinfInfoNameView); f.Add(34,&pReinfInfoIcon); f.Add(50,&visAbilities); f.Add(58,&pLeftPanel); f.Add(59,&pTopPanel); f.Add(60,&pBottomPanel); f.Add(62,&pCommanderInfoBlock); f.Add(63,&pReinfCurrentBlock); f.Add(64,&pPictureBlock); f.Add(65,&pPromoteCommanderView); f.Add(66,&pPermanentCommanderWnd); f.Add(67,&pPermanentCommanderIcon); f.Add(68,&pSelectCommanderBtn); f.Add(69,&pSelectCommanderIcon); f.Add(70,&pUnselectCommanderBtn); f.Add(71,&pUnselectCommanderIcon); f.Add(72,&pProfileUnitNameView); f.Add(73,&pProfileUnitExpView); f.Add(74,&pUndoBtn); f.Add(75,&nAssignmentCount); f.Add(76,&pAutoAssignDlg); f.Add(77,&pUndoPromotionsDlg); f.Add(78,&pUndoAllPromotionsDlg); f.Add(79,&wszChangedValueTag); f.Add(80,&pLeaderRankLabel); return 0; }

	NTimer::STime timeAbs; // don't store
private:
	bool OnBack();
	bool OnSelect();
	bool OnSetLeader();
	bool OnAssignLeaderOk();
	bool OnAssignLeaderCancel();
	bool OnMenuUndo();
	bool OnUndoAssignCommander();

	bool OnAutoAssignOk();
	bool OnAutoAssignCancel();
	bool OnUndoPromotionsOk();
	bool OnUndoPromotionsCancel();
	bool OnUndoAllPromotionsOk();
	bool OnUndoAllPromotionsCancel();

	void MakeInterior();
	void GetElements();
	void ShowLeaderInfo( bool bEnabled, const CLeaderInfo *pInfo, bool bExist );
	void ShowReinfInfo( bool bEnabled, const CReinfData *pReinf );
	void ShowReinfAbilities( bool bEnabled, const CReinfData *pReinf, const CLeaderInfo *pLeader );
	void UpdateSelectionInfo();
	void FillLeaderInfo( CReinfData *pReinf, const IScenarioTracker::SUndoLeaderInfo &undo );
	void Back();
	bool CheckAutoAssign();
	void ShowNoSelection();
	void UndoAssignCommander();
	void UndoAllAssignCommander();
	void AutoAssignCommanders();
	void UpdateSelectedLeaderVisualInfo();
	void UpdateLeaderVisualInfo( const CLeaderInfo *pLeader );
	bool IsMainScreenActive() const;
public:
	CInterfaceArmyScreen();
	~CInterfaceArmyScreen();

	bool Init();

	bool StepLocal( bool bAppActive );
	
	//{ IProgrammedReactionsAndChecks
	bool Execute( const std::string &szSender, const std::string &szReaction );
	int Check( const std::string &szCheckName ) const;
	//}
};

class CICArmyScreen : public CInterfaceCommandBase<CInterfaceArmyScreen>
{
	OBJECT_BASIC_METHODS( CICArmyScreen );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};


