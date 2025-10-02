
#pragma once
#include "InterfaceScreenBase.h"
#include "InterfaceChapterMapMenuHelper.h"

namespace NDb
{
	struct SChapter;
	struct SMapInfo;
	struct SReinforcement;
	struct SMechUnitRPGStats;
	struct SSquadRPGStats;
	struct SEnemyEntry;
	struct SChapterBonus;
}
struct SChapterReinfUpgrade;
struct SChapterReinfComposition;

class CInterfaceChapterMapMenu : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceChapterMapMenu );
public:
	struct SChapterDesc;
	struct SMissionDesc;
public:
	class CReactions : public IProgrammedReactionsAndChecks
	{
		OBJECT_NOCOPY_METHODS(CReactions);
		CPtr<IWindow> pScreen;
		CPtr<CInterfaceChapterMapMenu> pInterface;
	public:
		CReactions() {  }
		~CReactions() 
		{  
		}
		CReactions( IWindow *_pScreen, CInterfaceChapterMapMenu *_pInterface ) : 
			pScreen( _pScreen ), pInterface( _pInterface ) {   }
		virtual bool Execute( const string &szSender, const string &szReaction );
		virtual int Check( const string &szCheckName ) const;

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pScreen );
			saver.Add( 2, &pInterface );
			return 0;
		}
	};
private:
	enum EMissionState { EMS_DISABLED, EMS_ENABLED, EMS_COMPLETED, EMS_RECOMMENDED };
	struct SReward
	{
		ZDATA
		CPtr<IButton> pBtn;
		CPtr<IWindow> pBonusWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBtn); f.Add(3,&pBonusWnd); return 0; }
	};
	struct STarget
	{
		ZDATA
		CDBPtr<NDb::SMapInfo> pDBInfo;
		EMissionState eState;
		int nCalls;
		int nX;									// Place on chapter map
		int nY;
		float fValue0;					// Values for automatic frontline
		float fValue1;
		CPtr<IWindowFrameSequence> pFlame;
		CPtr<IWindow> pCompleted;
		CPtr<IWindow> pRecommended;
		int nRecommendedX;
		int nRecommendedY;
		CPtr<IButton> pWindow;
		vector< CDBPtr<NDb::SChapterBonus> > rewardDescs;
		vector<SReward> rewards;
		CPtr<IWindow> pBonusIcon;
		int nPriority;					// Recommended order of missions
		CVec2 vEndOffset;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pDBInfo); f.Add(3,&eState); f.Add(4,&nCalls); f.Add(5,&nX); f.Add(6,&nY); f.Add(7,&fValue0); f.Add(8,&fValue1); f.Add(9,&pFlame); f.Add(10,&pCompleted); f.Add(11,&pRecommended); f.Add(12,&nRecommendedX); f.Add(13,&nRecommendedY); f.Add(14,&pWindow); f.Add(15,&rewardDescs); f.Add(16,&rewards); f.Add(17,&pBonusIcon); f.Add(18,&nPriority); f.Add(19,&vEndOffset); return 0; }
	};
	typedef vector< STarget > CTargets;

	// Simpler reinf button (for mission bonuses and enemy forces)
	struct SReinfButtonDesc
	{
		ZDATA
		CPtr<IButton> pButton;
		CPtr<IWindow> pIcon;
		CDBPtr<NDb::SReinforcement> pReinforcement;
		CDBPtr<NDb::SMechUnitRPGStats> pMechUnit;
		CDBPtr<NDb::SSquadRPGStats> pSquad;
		int nQuantity;
		CPtr<IWindow> pBonusWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pButton); f.Add(3,&pIcon); f.Add(4,&pReinforcement); f.Add(5,&pMechUnit); f.Add(6,&pSquad); f.Add(7,&nQuantity); f.Add(8,&pBonusWnd); return 0; }
	};
	typedef vector< SReinfButtonDesc > CReinfButtonList;

	// More complex reinf button (for active army)
	struct SReinfRemapItem : public SReinfButtonDesc
	{
		ZDATA_( SReinfButtonDesc )
		CDBPtr<NDb::STexture> pDefaultTexture;
		CDBPtr<NDb::STexture> pDisabledTexture;
		wstring wszDefaultTooltip;
		CPtr<IWindow> pUnknownWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(1,( SReinfButtonDesc *)this); f.Add(2,&pDefaultTexture); f.Add(3,&pDisabledTexture); f.Add(4,&wszDefaultTooltip); f.Add(5,&pUnknownWnd); return 0; }
	};
	typedef vector< SReinfRemapItem > CReinfRemap;

	enum EUIState
	{
		EUIS_NORMAL,
		EUIS_PLAY_MISSION_START_EFFECT,
		EUIS_PLAY_MISSION_START_TRANSIT_EFFECT,
		EUIS_PLAY_MISSION_DONE,
		EUIS_PLAY_MISSION_PRESSED,
	};
	
	enum EUIEffectState
	{
		EUIES_FADE,
		EUIES_EXPAND,
		EUIES_WAIT,
		EUIES_DONE,
	};

	enum EReinfDescWindowState
	{
		ERDWS_NONE,
		ERDWS_UNIT,
		ERDWS_REINF,
		ERDWS_MULTIREINF,
		ERDWS_UPGRADE,
	};

	enum EExitDestination
	{
		EED_ENTER,				// this one plays on screen start
		EED_RE_ENTER,				// this one plays when we return from Army Manager, SaveLoad, etc
		EED_BACK,
		EED_ARMY_MANAGER,
		EED_SAVE,
		EED_PLAYER_INFO,
	};

	int nDelay;											// Pass several StepLocal()-s to let it do the autosave
	// ReinfDesc {	- Don't need to be saved
	CReinfButtonList reinfDescReinfs;
	CReinfButtonList reinfDescUnits;
	CReinfButtonList reinfDescUnits2;
	EReinfDescWindowState eReinfDescState;
	SReinfButtonDesc reinfDescSingleItem;
	CPtr<IWindow> pReinfDescReinfArea;
	vector< CDBPtr<NDb::SReinforcement> > oldReinfs;
	int nSelectedReinf;
	int nButtonStep;
	int nFirstButtonPosX;
	CPtr<IButton> pButtonScrollRight;
	CPtr<IButton> pButtonScrollLeft;
	// } ReinfDesc

	EExitDestination eExitDir;
	bool bNeedToRunAnimation;

	ZDATA_(CInterfaceScreenBase)
	CObj<CReactions> pReactions;
	CTargets targets;

	CPtr<IWindow> pMain;
	
	CPtr<IWindow> pChapterMap;
	CPtr<IButton> pChapterMapTarget;
	CPtr<IButton> pChapterMapTargetBig;
	CPtr<IButton> pArmyManager;
	bool bArmyManagerInitialEnable;
	CPtr<IButton> pPlay;

	// Mission Desc
	CPtr<ITextView> pMissionName;
	CPtr<IWindow> pReinfGrid;
	CPtr<IWindow> pMissionEnabledLight;

	CReinfRemap reinfButtons;

	wstring wszReinfNotEnabledPrefix;
	wstring wszReinfDisabledPrefix;
	wstring wszReinfAvailablePrefix;

	EUIState eUIState;

	ZSKIP

	NTimer::STime timeStartEffect;
	EUIEffectState eUIEffectState;
	CTRect<float> rcInitialMapBounds;
	CPtr<IWindow> pMapPanel;

	CPtr<IPotentialLines> pFrontlines;
	CPtr<IWindow> pChapterMapBgr;
	int nFrontLineAnim;
	CDBPtr<NDb::SMapInfo> pDetailsMap;
	CVec2 vDetailsCoeff;			// Coefficient for DetailsMap->Screen translation

	CReinfButtonList bonusButtons;
	CPtr<IWindow> pFinalBonus;

	CPtr<IWindow> pReinfDesc;
	CPtr<IWindow> pReinfDesc1Unit;
	CPtr<IWindow> pReinfDesc1Reinf;
	CPtr<IWindow> pReinfDescMultiReinf;
	CPtr<IWindow> pReinfDescUpgrade;

	CPtr<IPlayer> pReinfRoller1;
	CPtr<IPlayer> pReinfRoller2;
	CPtr<IPlayer> pReinfRoller3;

	CPtr<IPlayer> pMissionReinfRoller1;
	CPtr<IPlayer> pMissionReinfRoller2;
	int nCurrentMissionReinfs;

	CDBPtr<NDb::SChapter> pChapter;
	int nCallsLeft;

	ZSKIP //bool bNeedReinfReport;
	int nEffectCounter;

	CPtr<IWindow> pChapterMapLeft;
	CPtr<IWindow> pChapterMapRight;
	int nChapterMapLeftX;
	int nChapterMapLeftY;
	int nChapterMapRightX;
	int nChapterMapRightY;
	
	CPtr<IButton> pReinfGridUpgradableTemplate;
	CVec2 vReinfGridUpgradableTemplateDelta;

	CPtr<IWindow> pBonusGrid;
	CPtr<IWindow> pBonusGridBonusTemplate;
	CVec2 vBonusGridBonusTemplateDelta;

	CObj<SChapterMapMenuHelper> pHelper;
	int nUpgradeMissionSelected;
	int nUpgradeRewardSelected;

	CDBPtr<NDb::SMapInfo> pMapToStart;
	CObj<SChapterReinfUpgrade> pReinfUpgrade;
	CObj<SChapterReinfComposition> pReinfComposition;
	CObj<SChapterDesc> pChapterDescDlg;
	CObj<SMissionDesc> pMissionDescDlg;
	int nSelectedMission;
	CPtr<IButton> pMissionDescBtn;
	int nRecommendedTarget;
	bool bInitialDialogVisible;
	bool bRequestAutoSave;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceScreenBase*)this); f.Add(2,&pReactions); f.Add(3,&targets); f.Add(4,&pMain); f.Add(5,&pChapterMap); f.Add(6,&pChapterMapTarget); f.Add(7,&pChapterMapTargetBig); f.Add(8,&pArmyManager); f.Add(9,&bArmyManagerInitialEnable); f.Add(10,&pPlay); f.Add(11,&pMissionName); f.Add(12,&pReinfGrid); f.Add(13,&pMissionEnabledLight); f.Add(14,&reinfButtons); f.Add(15,&wszReinfNotEnabledPrefix); f.Add(16,&wszReinfDisabledPrefix); f.Add(17,&wszReinfAvailablePrefix); f.Add(18,&eUIState); f.Add(20,&timeStartEffect); f.Add(21,&eUIEffectState); f.Add(22,&rcInitialMapBounds); f.Add(23,&pMapPanel); f.Add(24,&pFrontlines); f.Add(25,&pChapterMapBgr); f.Add(26,&nFrontLineAnim); f.Add(27,&pDetailsMap); f.Add(28,&vDetailsCoeff); f.Add(29,&bonusButtons); f.Add(30,&pFinalBonus); f.Add(31,&pReinfDesc); f.Add(32,&pReinfDesc1Unit); f.Add(33,&pReinfDesc1Reinf); f.Add(34,&pReinfDescMultiReinf); f.Add(35,&pReinfDescUpgrade); f.Add(36,&pReinfRoller1); f.Add(37,&pReinfRoller2); f.Add(38,&pReinfRoller3); f.Add(39,&pMissionReinfRoller1); f.Add(40,&pMissionReinfRoller2); f.Add(41,&nCurrentMissionReinfs); f.Add(42,&pChapter); f.Add(43,&nCallsLeft); f.Add(45,&nEffectCounter); f.Add(46,&pChapterMapLeft); f.Add(47,&pChapterMapRight); f.Add(48,&nChapterMapLeftX); f.Add(49,&nChapterMapLeftY); f.Add(50,&nChapterMapRightX); f.Add(51,&nChapterMapRightY); f.Add(52,&pReinfGridUpgradableTemplate); f.Add(53,&vReinfGridUpgradableTemplateDelta); f.Add(54,&pBonusGrid); f.Add(55,&pBonusGridBonusTemplate); f.Add(56,&vBonusGridBonusTemplateDelta); f.Add(57,&pHelper); f.Add(58,&nUpgradeMissionSelected); f.Add(59,&nUpgradeRewardSelected); f.Add(60,&pMapToStart); f.Add(61,&pReinfUpgrade); f.Add(62,&pReinfComposition); f.Add(63,&pChapterDescDlg); f.Add(64,&pMissionDescDlg); f.Add(65,&nSelectedMission); f.Add(66,&pMissionDescBtn); f.Add(67,&nRecommendedTarget); f.Add(68,&bInitialDialogVisible); f.Add(69,&bRequestAutoSave); return 0; }
private:
	void PlayMissionStartEffect();
	void PlayMissionStartMission();
	void FadeMapElements( float fFade );
	void ExpandMap( float fProgress );
	void UpdateUIState();

	bool OnReinfUpgradeDialogClose( const string &szSender );
	bool OnReinfUpgradeUnitBtn( const string &szSender );
	
	bool OnChapterDescDlgClose( const string &szSender );
	bool OnMissionDescDlgClose( const string &szSender );
	bool OnShowMissionDesc( const string &szSender );
	
	void HideDialogs();
protected:
	void MsgBack( const SGameMessage &msg );
	void MsgMessageBoxOk( const SGameMessage &msg );
	void MsgMessageBoxCancel( const SGameMessage &msg );
	void MsgPlay( const SGameMessage &msg );
	void MsgContinuePlay( const SGameMessage &msg );
	void MsgReenter( const SGameMessage &msg );
	void MsgCompleteSelectedMission( const SGameMessage &msg ); // For Testers
	void MsgCompleteChapter( const SGameMessage &msg ); // For Testers

	bool OnArmyManager();
	bool OnPlayerStats();
	bool OnSaveGame();
	
	void OnTargetSelect( const string &szSender );
	bool OnTargetDblClick( const string &szSender );
	void OnPopupClicked( const string &szSender );
	bool OnMouseOverReinf( const string &szSender, bool bEnter );
	bool OnFixBonus( const string &szSender );

	// ReinfDesc{
	void ShowReinfDesc( const NDb::SReinforcement *pReinf );			// Display one reinf
	void ShowReinfDesc( const NDb::SMechUnitRPGStats *pMech, const NDb::SSquadRPGStats *pSquad );				// Display one enemy
	void ShowReinfDesc( const NDb::SReinforcement *pReinf, const NDb::SReinforcement *pReinfUpg );			// Display one reinf upgrade
	void OnReinfDescItem( const string &szSender );		// Reinforcement or unit selected
	void OnReinfDescOK();
	void OnReinfDescEncyclopedia();
	IWindow *GetCurrentReinfDescPopup();
	void ReinfDescFillUnits( const NDb::SReinforcement *pReinf, CReinfButtonList &units );
	void ReinfDescSelectUnit( CReinfButtonList &units, int nIndex );
	void ReinfDescMakeUnitInfo();
	void ReinfDescScrollRight();
	void ReinfDescScrollLeft();
	void ReinfDescProcessScroll( const int nStep );
	void ReinfDescSelectReinf( int nIndex );
	bool OnTargetOver( const string &szSender );
	bool OnTargetPushed( const string &szSender );
	bool OnTargetPushedBack( const string &szSender );
	// }ReinfDesc

	// Init{
	void InitLoadControls();
	void InitReinforcements();
	void InitMissions();
	void EffectStart( EExitDestination eWhereTo );
	void OnEffectFinish();
	// }Init

	void SelectTarget( int nIndex );
	void MakeMissionInfo( int nIndex );
	int MakeArmyInfo( bool bShow = true );
	void MakeChapterInfo();
	void SwitchTargetState( int nTarget, bool bSelected );
	const wstring MakeTooltip( const NDb::SReinforcement *pReinf );
	void ShowAllRewards( bool bShow );
	void PlayReinfRollerAnim( int _nStart, int _nEnd );
	void PlayMissionRollerAnim( int _nStart, int _nEnd );
	
	void HideChildren( IWindow *pParent );
	
	void RegisterObservers();

	void DoAutoSave();
	
	void UpdateRecommendedButton( STarget &target, bool bPushed );
	
	void ProceedInitialDialogs();

	~CInterfaceChapterMapMenu();
public:
	CInterfaceChapterMapMenu();

	bool Init();

	//{ IInterfaceBase
	void OnGetFocus( bool bFocus );
	bool ProcessEvent( const SGameMessage &msg );
	void AfterLoad();

	bool StepLocal( bool bAppActive );

	void Draw( NGScene::CRTPtr *pTexture );
	//}
};

#if !defined(_SINGLE_DEMO) || defined(_MP_DEMO)
class CICChapterMapMenu : public CInterfaceCommandBase<CInterfaceChapterMapMenu>
{
	OBJECT_BASIC_METHODS( CICChapterMapMenu );
	//
	void PreCreate();
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
};
#endif // !defined(_SINGLE_DEMO) || defined(_MP_DEMO)


