#pragma once

#include "InterfaceChapterMapMenu.h"
#include "UnitFullInfoHelper.h"
#include "../UISpecificB2/UISpecificB2.h"

namespace NDb
{
	struct SMissionEnableInfo;
}

struct SChapterReinfBase : public CObjectBase
{
	struct SUnitData
	{
		ZDATA
		CDBPtr<NDb::SMechUnitRPGStats> pMechUnit;
		CDBPtr<NDb::SSquadRPGStats> pSquad;
		int nCount;
		wstring wszName;
		vector<NUnitFullInfo::SWeapon> weapons;
		vector<int> armors;
		int nHP;
		vector<IWindow3DControl::SObject> objects3D;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMechUnit); f.Add(3,&pSquad); f.Add(4,&nCount); f.Add(5,&wszName); f.Add(6,&weapons); f.Add(7,&armors); f.Add(8,&nHP); f.Add(9,&objects3D); return 0; }

		bool operator()( const SUnitData &data ) const
		{
			return pMechUnit == data.pMechUnit && pSquad == data.pSquad;
		}
	};
	struct SUnit : public CObjectBase
	{
		OBJECT_NOCOPY_METHODS( SUnit );
	public:
		ZDATA
		CPtr<IWindow> pWnd;
		string szBtnName;
		CPtr<IWindow> pAppearanceWnd;
		CPtr<IWindow> pUnknownWnd;
		CPtr<IWindow> pNoneWnd;
		CPtr<IWindow> pCountWnd;
		CPtr<ITextView> pCountView;
		CPtr<IWindow> pSelectionWnd;
		SUnitData data;
		CPtr<ITextView> pHPView;
		CPtr<ITextView> pArmorFrontView;
		CPtr<ITextView> pArmorLeftView;
		CPtr<ITextView> pArmorRightView;
		CPtr<ITextView> pArmorTopView;
		CPtr<ITextView> pArmorBackView;
		CPtr<IWindow3DControl> p3DCtrl;
		int nBaseIndex;
		CPtr<IProgressBar> pHPBar;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&szBtnName); f.Add(4,&pAppearanceWnd); f.Add(5,&pUnknownWnd); f.Add(6,&pNoneWnd); f.Add(7,&pCountWnd); f.Add(8,&pCountView); f.Add(9,&pSelectionWnd); f.Add(10,&data); f.Add(11,&pHPView); f.Add(12,&pArmorFrontView); f.Add(13,&pArmorLeftView); f.Add(14,&pArmorRightView); f.Add(15,&pArmorTopView); f.Add(16,&pArmorBackView); f.Add(17,&p3DCtrl); f.Add(18,&nBaseIndex); f.Add(19,&pHPBar); return 0; }
	private:
		void MakeUnitInfo();
		void Show3DView();
		void Clear3DView();
		void Make3DInfo( const NDb::SHPObjectRPGStats *pStats );
	public:
		~SUnit();
		
		void InitControls( IWindow *pWnd, const string &szBtnName, int nBaseIndex );
		void ShowSelection( bool bShow );
		void SetUnknown();
		void SetNone();
		void SetUnit( const SUnitData &data );
	};
	struct SUnitWeapon
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CPtr<IWindow> pIconWnd;
		CPtr<ITextView> pCountView;
		CPtr<ITextView> pDamageView;
		CPtr<ITextView> pPenetrationView;
		CPtr<ITextView> pAmmoView;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pIconWnd); f.Add(4,&pCountView); f.Add(5,&pDamageView); f.Add(6,&pPenetrationView); f.Add(7,&pAmmoView); return 0; }
	};
	
	ZDATA
	CPtr<ITextView> pUnitNameView;
	vector<SUnitWeapon> weapons;
	CPtr<ITextView> pUnitSupplyLabel;
	CPtr<ITextView> pUnitSupplyView;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnitNameView); f.Add(3,&weapons); f.Add(4,&pUnitSupplyLabel); f.Add(5,&pUnitSupplyView); return 0; }
protected:
	void ShowUnitInfo( SUnit *pUnit );
	void MakeReinfData( vector<SUnitData> &units, const NDb::SReinforcement *pReinf );
	void InitUnitInfoControls( IWindow *pBaseWnd );
};

struct SChapterReinfUpgrade : public SChapterReinfBase
{
	OBJECT_NOCOPY_METHODS( SChapterReinfUpgrade );
public:
	struct SUpgradeLine
	{
		ZDATA
		CPtr<IWindow> pWnd;
		CObj<SUnit> pUnit1;
		CObj<SUnit> pUnit2;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWnd); f.Add(3,&pUnit1); f.Add(4,&pUnit2); return 0; }
	};
	
	ZDATA_(SChapterReinfBase)
	CPtr<IWindow> pBaseWnd;
	CPtr<IWindow> pUpgradePanel;
	CPtr<IWindow> pReinfIconWnd;
	CPtr<ITextView> pReinfHeaderView;
	CPtr<IWindow> pLineTemplate;
	CPtr<IScrollableContainer> pContainer;

	vector<SUpgradeLine> lines;
	CPtr<SUnit> pSelectedUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SChapterReinfBase*)this); f.Add(2,&pBaseWnd); f.Add(3,&pUpgradePanel); f.Add(4,&pReinfIconWnd); f.Add(5,&pReinfHeaderView); f.Add(6,&pLineTemplate); f.Add(7,&pContainer); f.Add(8,&lines); f.Add(9,&pSelectedUnit); return 0; }
private:
	void SelectUnit( SUnit *pUnit );
public:
	void InitControls( IWindow *pBaseWnd );
	void ShowReinf( const NDb::SReinforcement *pOldReinf, const NDb::SReinforcement *pNewReinf );
	void UnitBtnPressed( const string &szSender );
	void Hide();
};

struct SChapterReinfComposition : public SChapterReinfBase
{
	OBJECT_NOCOPY_METHODS( SChapterReinfComposition );
public:
	ZDATA_(SChapterReinfBase)
	CPtr<IWindow> pBaseWnd;
	CPtr<IWindow> pCompositionPanel;
	CPtr<IWindow> pReinfIconWnd;
	CPtr<ITextView> pReinfHeaderView;
	
	vector< CObj<SUnit> > units;
	CPtr<SUnit> pSelectedUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SChapterReinfBase*)this); f.Add(2,&pBaseWnd); f.Add(3,&pCompositionPanel); f.Add(4,&pReinfIconWnd); f.Add(5,&pReinfHeaderView); f.Add(6,&units); f.Add(7,&pSelectedUnit); return 0; }
private:
	void SelectUnit( SUnit *pUnit );
public:
	void InitControls( IWindow *pBaseWnd );
	void ShowReinf( const NDb::SReinforcement *pReinf );
	void UnitBtnPressed( const string &szSender );
	void Hide();
};

struct CInterfaceChapterMapMenu::SChapterDesc : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SChapterDesc );
public:
	ZDATA
	CPtr<IWindow> pBaseWnd;
	CPtr<IWindow> pPanel;
	CPtr<IScrollableContainer> pContainer;
	CPtr<ITextView> pDescView;
	CPtr<ITextView> pDescHeader;
	CPtr<IWindow> pItemGeneral;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseWnd); f.Add(3,&pPanel); f.Add(4,&pContainer); f.Add(5,&pDescView); f.Add(6,&pDescHeader); f.Add(7,&pItemGeneral); return 0; }
	
	void InitControls( IWindow *pBaseWnd );
	void Show( const NDb::SChapter *pChapter );
	void Hide();
};

struct CInterfaceChapterMapMenu::SMissionDesc : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SMissionDesc );
public:
	ZDATA
	CPtr<IWindow> pBaseWnd;
	CPtr<IWindow> pPanel;
	CPtr<IScrollableContainer> pContainer;
	CPtr<ITextView> pDescView;
	CPtr<ITextView> pNameView;
	CPtr<ITextView> pDifficultyView;
	CPtr<ITextView> pWeatherView;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pBaseWnd); f.Add(3,&pPanel); f.Add(4,&pContainer); f.Add(5,&pDescView); f.Add(6,&pNameView); f.Add(7,&pDifficultyView); f.Add(8,&pWeatherView); return 0; }

	void InitControls( IWindow *pBaseWnd );
	void Show( const NDb::SMissionEnableInfo &mission );
	void Hide();
};


