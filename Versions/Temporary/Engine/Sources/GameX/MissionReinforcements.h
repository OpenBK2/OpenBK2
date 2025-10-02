
#pragma once

#include "../misc/2darray.h"
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "WorldClient.h"
#include "../UI/UI.h"

#include <zconf.h>

class CMissionReinf : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CMissionReinf );
	
	struct SReinfInfo
	{
		ZDATA
		CPtr<IButton> pButton; // possible reinf
		CDBPtr< NDb::SReinforcement > pReinf; // enabled reinf
		CPtr<IWindow> pIconWnd;
		ZSKIP //CPtr<IButton> pXPLevelBtn;
		CPtr<IProgressBar> pXPBar;
		CPtr<IWindow> pBadWeatherWnd;
		CPtr<IWindow> pIconDisabledWnd;
		vector< CPtr<IWindow> > chevrons;
		CPtr<IWindow> pXPBarBg;
		CPtr<IWindow> pIconEnabledWnd;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pButton); f.Add(3,&pReinf); f.Add(4,&pIconWnd); f.Add(6,&pXPBar); f.Add(7,&pBadWeatherWnd); f.Add(8,&pIconDisabledWnd); f.Add(9,&chevrons); f.Add(10,&pXPBarBg); f.Add(11,&pIconEnabledWnd); return 0; }
		
		void ShowChevron( int nIndex );
	};
	struct SUnitInfo
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IButton> pButton;
		CPtr<IWindow> pIcon;
		CPtr<ITextView> pCountView;
		CDBPtr<NDb::SHPObjectRPGStats> pStats;
		int nCount;
		CPtr<IWindow> pFlagBg;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pButton); f.Add(4,&pIcon); f.Add(5,&pCountView); f.Add(6,&pStats); f.Add(7,&nCount); f.Add(8,&pFlagBg); return 0; }
	};
	typedef hash_map< string, NDb::EReinforcementType > CNameToTypes;
	typedef hash_map< NDb::EReinforcementType, SReinfInfo, SEnumHash > CReinfInfos;

	ZDATA
	CPtr<CWorldClient> pWorld;
	CPtr<IVisualNotifications> pNotifications;
	CPtr<IWindow> pReinfPanel;
	ZSKIP//CPtr<IWindow> pReinfMode;
	CPtr<IProgressBar> pProgress;
	CPtr<ITextView> pCount;
	CPtr<ITextView> pName;
	CNameToTypes nameToTypes;
	CReinfInfos reinfInfos;
	NDb::EReinforcementType eSelected;
	bool bTrackMousePos;
	CVec2 vMousePos;
	bool bAvailNotified;
	ZSKIP//bool bAvailNotifyRepeat;
	vector<SUnitInfo> units;
	bool bFullInfoMode;
	CPtr<IWindow> pCommonInfo;
	CPtr<IWindow> pFullInfoWnd;
	CObj<class CMissionUnitFullInfo> pUnitFullInfo;
	CPtr<ITextView> pNoInfoView;
	bool bBadWeather;
	CPtr<struct IWindowRoundProgressBar> pRoundProgress;
	NDb::ESeason eSeason;
	CPtr<IWindow> pRoundProgressMask;
	int nCalls;
	bool bDisabledByInterface;
	ZSKIP//CPtr<IButton> pCloseReinfBtn;
	CPtr<IWindow> pReinfCountPanel;
	CPtr<IPlayer> pRoller1;
	CPtr<IPlayer> pRoller2;
	int nPrevReinfCount;
	bool bIsOpen;
	bool bIsLight;
	vector< CDBPtr<NDb::STexture> > textures;
	CPtr<IButton> pToggleReinfBtn;

	bool bBlink;
	bool bBlinkFullTime;
	float fBlinkStep;
	float fBlinkDuration;
	float fBlinkPeriod;
	CPtr<IButton> pCallReinfModeBtn;
	CPtr<IButton> pAutoShowReinfBtn;
	bool bWasEnabled;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWorld); f.Add(3,&pNotifications); f.Add(4,&pReinfPanel); f.Add(6,&pProgress); f.Add(7,&pCount); f.Add(8,&pName); f.Add(9,&nameToTypes); f.Add(10,&reinfInfos); f.Add(11,&eSelected); f.Add(12,&bTrackMousePos); f.Add(13,&vMousePos); f.Add(14,&bAvailNotified); f.Add(16,&units); f.Add(17,&bFullInfoMode); f.Add(18,&pCommonInfo); f.Add(19,&pFullInfoWnd); f.Add(20,&pUnitFullInfo); f.Add(21,&pNoInfoView); f.Add(22,&bBadWeather); f.Add(23,&pRoundProgress); f.Add(24,&eSeason); f.Add(25,&pRoundProgressMask); f.Add(26,&nCalls); f.Add(27,&bDisabledByInterface); f.Add(29,&pReinfCountPanel); f.Add(30,&pRoller1); f.Add(31,&pRoller2); f.Add(32,&nPrevReinfCount); f.Add(33,&bIsOpen); f.Add(34,&bIsLight); f.Add(35,&textures); f.Add(36,&pToggleReinfBtn); f.Add(37,&bBlink); f.Add(38,&bBlinkFullTime); f.Add(39,&fBlinkStep); f.Add(40,&fBlinkDuration); f.Add(41,&fBlinkPeriod); f.Add(42,&pCallReinfModeBtn); f.Add(43,&pAutoShowReinfBtn); f.Add(44,&bWasEnabled); return 0; }
	
	NDb::EReinforcementType ePreSelected;
	
	NTimer::STime timeBlinkAbs;
private:
	void UpdateEnable();
	void UpdateReinfs();
	void UpdateReinfsVisual();
	void UpdateReinfsXPAndLevel();
	void UpdateMPReinfsXPAndLevel();
	void UpdatePoints();
	NDb::EReinforcementType GetDefaultReinfType() const;
	void Select( NDb::EReinforcementType eType );
	void SetFullInfoMode( bool bFullInfo );

	void ShowReinfContents( const NDb::SReinforcement *pReinf );
	
	void NotifyAvailReinf();
	void DisableNotifyAvailReinf();
	void UpdateForcedAction();
	
	const NDb::SReinforcement* GetReinfContext( const NDb::EReinforcementType eType ) const;
	const NDb::SReinforcementTypes* GetReinfTypes() const;
	
	NDb::EReinforcementType FindReinfInfo( const string &szName );
	void UpdateReinfInfo();
	bool CanCall( const NDb::EReinforcementType eType ) const;
	bool IsAvia( const NDb::EReinforcementType eType ) const;
	
	int GetReinfCallsLeft() const;

	void Update();
	
	void UpdateToggleButtonState();

	void SetBlink( bool bBlink, bool bFullTime );
	void UpdateBlink();
	
	bool HaveUnits() const;
	bool IsAutoShowReinf() const;

	bool IsAviaPresents() const;
	bool IsNonAllWeatherAviaPresents() const;
	bool IsAviaAndCanNotFly( const NDb::EReinforcementType eType ) const;
	bool IsAllWeatherAvia( const NDb::EReinforcementType eType ) const;
	bool HasMasterPilot( const NDb::SReinforcement *pReinf, int nLevel ) const;
public:
	CMissionReinf();
	~CMissionReinf();
	
	void Init( IScreen *pScr, CWorldClient *pWorld, IVisualNotifications *pNotifications, NDb::ESeason eSeason );
	
	void UpdateNewAvail();
	void UpdateNewPoint( bool bIsPoints );

	void Step();
	
	bool IsOpen() const { return bIsOpen; }
	void Show();
	void Select( const string &szSender );
	void SelectDblClick( const string &szSender );
	void Call( const CVec2 &vPos );
	void CallNoReinf();
	void Close( bool bForced );
	void ShowUnitInfo( const string &szSender );
	void UnitFullInfoBack();
	void MouseOverForward( const string &szSender );
	void MouseOverBackward( const string &szSender );
	void BadWeather( bool bStart );
	void UpdateWinLooseState();
	bool ResetReinfMode();

	// AI pos
	void SetMousePos( const CVec2 &vPos );
	void SetTrackMousePos( bool bTrack );

	void OnClickFullInfoMember( const string &szSender );
	void OnFullInfoMemberOverOn( const string &szSender );
	void OnFullInfoMemberOverOff( const string &szSender );
	void OnFullInfoWeaponOverOn( const string &szSender );
	void OnFullInfoWeaponOverOff( const string &szSender );

	void OnReinfCallMode();
	void OnReinfAutoShowReinf( bool bOn );

	bool IsEnabled() const;
	
	void AfterLoad();
};


