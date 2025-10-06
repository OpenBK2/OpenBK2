#include "stdafx.h"
#include "InterfaceChapterMapMenuDialogs.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Misc/StrProc.h"
#include "ScenarioTracker.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "InterfaceState.h"
#include "System/Text.h"

const int BASE_ID_3D = 30000;
const int DELTA_ID_3D = 100;

const int SUPPLIES_COUNT = 1000; // CRAP - magic number

// SChapterReinfBase::SUnit

SChapterReinfBase::SUnit::~SUnit()
{
	NI_ASSERT( IsValid( pAppearanceWnd ), "Programmers: incorrect destroy order" );
	Clear3DView();
}

void SChapterReinfBase::SUnit::InitControls( IWindow *_pWnd, const std::string &_szBtnName, int nBaseIndex )
{
	pWnd = _pWnd;
	IButton *pBtn = GetChildChecked<IButton>( pWnd, "UnitBtn", true );
	IButton *pUnitCountBtn = GetChildChecked<IButton>( pWnd, "UnitCountBtn", true );
	szBtnName = _szBtnName;
	if ( pBtn )
		pBtn->SetName( szBtnName );
	if ( pUnitCountBtn )
		pUnitCountBtn->SetName( szBtnName );
	pAppearanceWnd = GetChildChecked<IWindow>( pWnd, "UnitAppearance", true );
	pUnknownWnd = GetChildChecked<IWindow>( pWnd, "UnitUnknown", true );
	pNoneWnd = GetChildChecked<IWindow>( pWnd, "UnitNone", true );
	pCountWnd = GetChildChecked<IWindow>( pWnd, "UnitCount", true );
	pCountView = GetChildChecked<ITextView>( pWnd, "UnitCountView", true );
	pSelectionWnd = GetChildChecked<IWindow>( pWnd, "UnitSelection", true );

	pArmorFrontView = GetChildChecked<ITextView>( pWnd, "ArmorFrontView", true );
	pArmorLeftView = GetChildChecked<ITextView>( pWnd, "ArmorLeftView", true );
	pArmorRightView = GetChildChecked<ITextView>( pWnd, "ArmorRightView", true );
	pArmorTopView = GetChildChecked<ITextView>( pWnd, "ArmorTopView", true );
	pArmorBackView = GetChildChecked<ITextView>( pWnd, "ArmorBackView", true );
	pHPView = GetChildChecked<ITextView>( pWnd, "HPView", true );
	p3DCtrl = GetChildChecked<IWindow3DControl>( pWnd, "3DControl", true );
	pHPBar = GetChildChecked<IProgressBar>( pWnd, "HPBar", true );

	if ( pAppearanceWnd )
		pAppearanceWnd->ShowWindow( false );
	if ( pUnknownWnd )
		pUnknownWnd->ShowWindow( false );
	if ( pNoneWnd )
		pNoneWnd->ShowWindow( false );
	if ( pCountWnd )
		pCountWnd->ShowWindow( false );
	if ( pSelectionWnd )
		pSelectionWnd->ShowWindow( false );

	if ( p3DCtrl )
		p3DCtrl->SetBaseID3D( nBaseIndex );
}

void SChapterReinfBase::SUnit::ShowSelection( bool bShow )
{
	if ( pSelectionWnd )
		pSelectionWnd->ShowWindow( bShow );
}

void SChapterReinfBase::SUnit::SetUnknown()
{
	data.nCount = 0;

	NI_ASSERT( IsValid( pAppearanceWnd ), "Programmers: incorrect destroy order" );
	if ( pAppearanceWnd )
		pAppearanceWnd->ShowWindow( false );
	if ( pUnknownWnd )
		pUnknownWnd->ShowWindow( true );
	if ( pNoneWnd )
		pNoneWnd->ShowWindow( false );
	if ( pCountWnd )
		pCountWnd->ShowWindow( false );
	if ( pSelectionWnd )
		pSelectionWnd->ShowWindow( false );
	if ( p3DCtrl )
		p3DCtrl->ShowWindow( false );
	
	Clear3DView();
}

void SChapterReinfBase::SUnit::SetNone()
{
	data.nCount = 0;

	NI_ASSERT( IsValid( pAppearanceWnd ), "Programmers: incorrect destroy order" );
	if ( pAppearanceWnd )
		pAppearanceWnd->ShowWindow( false );
	if ( pUnknownWnd )
		pUnknownWnd->ShowWindow( false );
	if ( pNoneWnd )
		pNoneWnd->ShowWindow( true );
	if ( pCountWnd )
		pCountWnd->ShowWindow( false );
	if ( pSelectionWnd )
		pSelectionWnd->ShowWindow( false );
	if ( p3DCtrl )
		p3DCtrl->ShowWindow( false );
		
	Clear3DView();
}

void SChapterReinfBase::SUnit::SetUnit( const SUnitData &_data )
{
	data = _data;

	MakeUnitInfo();

	if ( pAppearanceWnd )
		pAppearanceWnd->ShowWindow( true );
	if ( pUnknownWnd )
		pUnknownWnd->ShowWindow( false );
	if ( pNoneWnd )
		pNoneWnd->ShowWindow( false );
	if ( pCountWnd )
		pCountWnd->ShowWindow( data.nCount > 1 );
	if ( data.nCount > 1 )
	{
		std::wstring wszCount = NStr::ToUnicode( StrFmt( "%d", data.nCount ) );
		if ( pCountView )
			pCountView->SetText( pCountView->GetDBText() + wszCount );
	}
	
	std::wstring wszHP = NStr::ToUnicode( StrFmt( "%d", data.nHP ) );
	if ( pHPView )
		pHPView->SetText( pHPView->GetDBText() + wszHP );

	if ( pHPBar )
	{
		if ( IScenarioTracker *pST = Singleton<IScenarioTracker>() )
		{
			const IScenarioTracker::SPlayerColor &color = pST->GetPlayerColor( pST->GetLocalPlayer() );
			pHPBar->SetForward( color.pUnitFullInfo );
		}
		pHPBar->SetPosition( 1.0f );
	}
		
	bool bShowArmor = (data.armors.size() >= NUnitFullInfo::ES_ARMOR_COUNT);
	if ( bShowArmor )
	{
		std::wstring wszArmorFront = NStr::ToUnicode( StrFmt( "%d", data.armors[NUnitFullInfo::ES_ARMOR_FRONT] ) );
		std::wstring wszArmorSide = NStr::ToUnicode( StrFmt( "%d", data.armors[NUnitFullInfo::ES_ARMOR_SIDE] ) );
		std::wstring wszArmorTop = NStr::ToUnicode( StrFmt( "%d", data.armors[NUnitFullInfo::ES_ARMOR_TOP] ) );
		std::wstring wszArmorBack = NStr::ToUnicode( StrFmt( "%d", data.armors[NUnitFullInfo::ES_ARMOR_BACK] ) );
		if ( pArmorFrontView )
			pArmorFrontView->SetText( pArmorFrontView->GetDBText() + wszArmorFront );
		if ( pArmorLeftView )
			pArmorLeftView->SetText( pArmorLeftView->GetDBText() + wszArmorSide );
		if ( pArmorRightView )
			pArmorRightView->SetText( pArmorRightView->GetDBText() + wszArmorSide );
		if ( pArmorTopView )
			pArmorTopView->SetText( pArmorTopView->GetDBText() + wszArmorTop );
		if ( pArmorBackView )
			pArmorBackView->SetText( pArmorBackView->GetDBText() + wszArmorBack );
	}
	if ( pArmorFrontView )
		pArmorFrontView->ShowWindow( bShowArmor );
	if ( pArmorLeftView )
		pArmorLeftView->ShowWindow( bShowArmor );
	if ( pArmorRightView )
		pArmorRightView->ShowWindow( bShowArmor );
	if ( pArmorTopView )
		pArmorTopView->ShowWindow( bShowArmor );
	if ( pArmorBackView )
		pArmorBackView->ShowWindow( bShowArmor );

	if ( p3DCtrl )
		p3DCtrl->ShowWindow( true );
		
	Show3DView();
}

void SChapterReinfBase::SUnit::MakeUnitInfo()
{
	data.weapons.resize( 0 );

	const NDb::SHPObjectRPGStats *pStats = data.pMechUnit;
	if ( !pStats )
		pStats = data.pSquad;

	if ( pStats && CHECK_TEXT_NOT_EMPTY_PRE(pStats->,LocalizedName) )
		data.wszName = GET_TEXT_PRE(pStats->,LocalizedName);

	NUnitFullInfo::MakeWeapons( data.weapons, pStats );
	NUnitFullInfo::MakeArmors( data.armors, pStats );
	data.nHP = NUnitFullInfo::MakeHP( pStats );
	
	Make3DInfo( pStats );
}

void SChapterReinfBase::SUnit::Clear3DView()
{
	if ( p3DCtrl )
		p3DCtrl->SetObjects( std::vector<IWindow3DControl::SObject>() );
}

void SChapterReinfBase::SUnit::Show3DView()
{
	if ( p3DCtrl )
		p3DCtrl->SetObjects( data.objects3D );
}

void SChapterReinfBase::SUnit::Make3DInfo( const NDb::SHPObjectRPGStats *pStats )
{
	if ( !p3DCtrl )
		return;

	data.objects3D.clear();

	if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast<const NDb::SSquadRPGStats*>( pStats ) )
	{
		for ( int i = 0; i < (std::min<int>)( 3, pSquadStats->members.size() ); ++i )
		{
			const NDb::SInfantryRPGStats *pMember = pSquadStats->members[i];
			if ( !pMember )
				continue;
			IWindow3DControl::SParam param = p3DCtrl->GetDBObjectParam( i + 1 );
			IWindow3DControl::SObject object;
			object.nID = i;
			const NDb::SVisObj *pVisObj = pMember->pinfoVisualObject;
			if ( !pVisObj )
				pVisObj = pMember->pvisualObject;
			object.pModel = NUnitFullInfo::GetModel( pVisObj, NDb::SEASON_SUMMER );
			NI_ASSERT( object.pModel, StrFmt( "Designers: no model for object: %s", pStats->GetDBID().ToString() ) );
			object.vPos = param.vPos;
			object.vSize = param.vSize;
			object.pAnim = NUnitFullInfo::FindAnimation( pMember ) ;
			data.objects3D.push_back( object );
		}
	}
	else
	{
		IWindow3DControl::SParam param = p3DCtrl->GetDBObjectParam( 0 );
		IWindow3DControl::SObject object;
		object.nID = 0;
		const NDb::SVisObj *pVisObj = 0;
		if ( pStats )
		{
			pVisObj = pStats->pinfoVisualObject;
			if ( !pVisObj )
				pVisObj = pStats->pvisualObject;
		}
		object.pModel = pStats ? NUnitFullInfo::GetModel( pVisObj, NDb::SEASON_SUMMER ) : 0;
		NI_ASSERT( !pStats || object.pModel, StrFmt( "Designers: no model for object: %s", pStats->GetDBID().ToString() ) );
		object.vPos = param.vPos;
		object.vSize = param.vSize;
		data.objects3D.push_back( object );
	}
}

// SChapterReinfBase

void SChapterReinfBase::InitUnitInfoControls( IWindow *pBaseWnd )
{
	// selected unit info
	pUnitNameView = GetChildChecked<ITextView>( pBaseWnd, "UnitName", true );
	pUnitSupplyLabel = GetChildChecked<ITextView>( pBaseWnd, "UnitSupplyLabel", true );
	pUnitSupplyView = GetChildChecked<ITextView>( pBaseWnd, "UnitSupplyView", true );
	if ( pBaseWnd )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pBaseWnd->GetChild( StrFmt( "UnitWeapon%d", i ), true );
			if ( !pWnd )
				break;
			weapons.push_back( SUnitWeapon() );
			SUnitWeapon &weapon = weapons.back();
			weapon.pWnd = pWnd;
			weapon.pIconWnd = GetChildChecked<IWindow>( pWnd, "WeaponIcon", true );
			weapon.pCountView = GetChildChecked<ITextView>( pWnd, "WeaponBriefCount", true );
			weapon.pDamageView = GetChildChecked<ITextView>( pWnd, "DamageView", true );
			weapon.pPenetrationView = GetChildChecked<ITextView>( pWnd, "PenetrationView", true );
			weapon.pAmmoView = GetChildChecked<ITextView>( pWnd, "AmmoView", true );
		}
	}
	if ( pUnitSupplyLabel )
		pUnitSupplyLabel->ShowWindow( false );
	if ( pUnitSupplyView )
		pUnitSupplyView->ShowWindow( false );
}

void SChapterReinfBase::ShowUnitInfo( SUnit *pUnit )
{
	std::wstring wszName;
	if ( pUnit )
		wszName = pUnit->data.wszName;
	if ( pUnitNameView )
		pUnitNameView->SetText( pUnitNameView->GetDBText() + wszName );

	for ( int i = 0; i < weapons.size(); ++i )
	{
		SUnitWeapon &weapon = weapons[i];
		if ( !pUnit || i >= pUnit->data.weapons.size() )
		{
			if ( weapon.pWnd )
				weapon.pWnd->ShowWindow( false );
			continue;
		}

		NUnitFullInfo::SWeapon &weaponInfo = pUnit->data.weapons[i];
		if ( weapon.pWnd )
			weapon.pWnd->ShowWindow( true );
		if ( weapon.pIconWnd )
		{
			std::wstring wszTooltip;
			if ( weaponInfo.pWeapon && CHECK_TEXT_NOT_EMPTY_PRE(weaponInfo.pWeapon->,LocalizedName) )
				wszTooltip = GET_TEXT_PRE(weaponInfo.pWeapon->,LocalizedName);
			weapon.pIconWnd->SetTexture( weaponInfo.pWeapon ? weaponInfo.pWeapon->pWeaponTypeTexture : 0 );
			weapon.pIconWnd->SetTooltip( wszTooltip );
		}
		std::wstring wszCount = NStr::ToUnicode( StrFmt( "%d", weaponInfo.nCount ) );
		if ( weapon.pCountView )
			weapon.pCountView->SetText( weapon.pCountView->GetDBText() + wszCount );
		std::wstring wszDamage = NStr::ToUnicode( StrFmt( "%d", weaponInfo.nDamage ) );
		if ( weapon.pDamageView )
			weapon.pDamageView->SetText( weapon.pDamageView->GetDBText() + wszDamage );
		std::wstring wszPenetration = NStr::ToUnicode( StrFmt( "%d", weaponInfo.nPenetration ) );
		if ( weapon.pPenetrationView )
			weapon.pPenetrationView->SetText( weapon.pPenetrationView->GetDBText() + wszPenetration );
		std::wstring wszAmmo = NStr::ToUnicode( StrFmt( "%d", weaponInfo.nAmmo ) );
		if ( weapon.pAmmoView )
			weapon.pAmmoView->SetText( weapon.pAmmoView->GetDBText() + wszAmmo );
	}

	bool bIsSupply = pUnit ? pUnit->data.weapons.empty() : false;
	if ( bIsSupply && pUnit->data.pMechUnit && pUnit->data.pMechUnit->IsTransport() )
	{
		if ( pUnit->data.pMechUnit->pActions && NUnitFullInfo::IsResourcesCarrier( pUnit->data.pMechUnit->pActions->availUserActions ) )
			bIsSupply = true;
	}
	if ( pUnitSupplyLabel )
		pUnitSupplyLabel->ShowWindow( bIsSupply );
	if ( pUnitSupplyView )
	{
		pUnitSupplyView->ShowWindow( bIsSupply );
		pUnitSupplyView->SetText( pUnitSupplyView->GetDBText() + NStr::ToUnicode( StrFmt( "%d", SUPPLIES_COUNT ) ) );
	}
}

void SChapterReinfBase::MakeReinfData( std::vector<SUnitData> &units, const NDb::SReinforcement *pReinf )
{
	units.resize( 0 );
	if ( !pReinf )
		return;

	units.reserve( pReinf->entries.size() );
	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		const NDb::SReinforcementEntry &entry = pReinf->entries[i];
		SUnitData unitData;
		unitData.pMechUnit = entry.pMechUnit;
		unitData.pSquad = entry.pSquad;
		unitData.nCount = 1;

		std::vector<SUnitData>::iterator it = find_if( units.begin(), units.end(), unitData );
		if ( it == units.end() )
			units.push_back( unitData );
		else
			++(it->nCount);
	}
}

// SChapterReinfUpgrade

void SChapterReinfUpgrade::InitControls( IWindow *_pBaseWnd )
{
	pBaseWnd = _pBaseWnd;
	pUpgradePanel = GetChildChecked<IWindow>( pBaseWnd, "UpgradePanel", true );
	pReinfIconWnd = GetChildChecked<IWindow>( pUpgradePanel, "ReinfIcon", true );
	pReinfHeaderView = GetChildChecked<ITextView>( pUpgradePanel, "Header", true );
	if ( pUpgradePanel )
		pUpgradePanel->ShowWindow( false );

	pLineTemplate = GetChildChecked<IWindow>( pUpgradePanel, "UpgradeLine", true );
	if ( pLineTemplate )
		pLineTemplate->ShowWindow( false );
	pContainer = GetChildChecked<IScrollableContainer>( pUpgradePanel, "Container", true );

	InitUnitInfoControls( pUpgradePanel );
}

void SChapterReinfUpgrade::ShowReinf( const NDb::SReinforcement *pOldReinf, 
	const NDb::SReinforcement *pNewReinf )
{
	if ( !pNewReinf && !pOldReinf )
		return;

	if ( pBaseWnd )
		pBaseWnd->ShowWindow( true );
	if ( pUpgradePanel )
		pUpgradePanel->ShowWindow( true );

	if ( pReinfIconWnd )
		pReinfIconWnd->SetTexture( pNewReinf ? pNewReinf->pIconTexture : 0 );
	std::wstring wszName;
	if ( pNewReinf && CHECK_TEXT_NOT_EMPTY_PRE(pNewReinf->,LocalizedName) )
		wszName = GET_TEXT_PRE(pNewReinf->,LocalizedName);
	if ( pReinfHeaderView )
		pReinfHeaderView->SetText( pReinfHeaderView->GetDBText() + wszName );

	std::vector<SUnitData> unitsDataOld;
	std::vector<SUnitData> unitsDataNew;
	MakeReinfData( unitsDataOld, pOldReinf );
	MakeReinfData( unitsDataNew, pNewReinf );
	
	int nMaxCount = (std::max)( unitsDataOld.size(), unitsDataNew.size() );
	lines.reserve( nMaxCount );
	for ( int i = 0; i < nMaxCount; ++i )
	{
		lines.push_back( SUpgradeLine() );
		SUpgradeLine &line = lines.back();
		line.pWnd = AddWindowCopy( pContainer, pLineTemplate );
		if ( pContainer )
		{
			pContainer->PushBack( line.pWnd, false );
			pContainer->SetDiscreteScroll( 2 );
		}
		line.pUnit1 = new SUnit();
		line.pUnit2 = new SUnit();
		line.pUnit1->InitControls( GetChildChecked<IWindow>( line.pWnd, "UnitPanel1", true ), StrFmt( "UnitBtnOld%d", i ), BASE_ID_3D + DELTA_ID_3D * (2 * i) );
		line.pUnit2->InitControls( GetChildChecked<IWindow>( line.pWnd, "UnitPanel2", true ), StrFmt( "UnitBtnNew%d", i ), BASE_ID_3D + DELTA_ID_3D * (2 * i + 1) );
		line.pUnit1->data.nCount = 0;
		line.pUnit2->data.nCount = 0;
		
		if ( i < unitsDataOld.size()  )
		{
			line.pUnit1->SetUnit( unitsDataOld[i] );
		}
		else
		{
			line.pUnit1->SetUnknown();
		}

		if ( i < unitsDataNew.size()  )
		{
			line.pUnit2->SetUnit( unitsDataNew[i] );
		}
		else
		{
			line.pUnit1->SetNone();
		}
	}

	SUnit *pUnit = 0;
	for ( int i = 0; i < nMaxCount; ++i )
	{
		SUpgradeLine &line = lines[i];
		if ( line.pUnit1->data.nCount > 0 )
		{
			pUnit = line.pUnit1;
			break;
		}
		if ( line.pUnit2->data.nCount > 0 )
		{
			pUnit = line.pUnit2;
			break;
		}
	}
	SelectUnit( pUnit );
}

void SChapterReinfUpgrade::UnitBtnPressed( const std::string &szSender )
{
	if ( !pBaseWnd || !pBaseWnd->IsVisible() )
		return;
	
	for ( int i = 0; i < lines.size(); ++i )
	{
		SUpgradeLine &line = lines[i];
		if ( line.pUnit1->szBtnName == szSender )
		{
			SelectUnit( line.pUnit1 );
			break;
		}
		if ( line.pUnit2->szBtnName == szSender )
		{
			SelectUnit( line.pUnit2 );
			break;
		}
	}
}

void SChapterReinfUpgrade::Hide()
{
	if ( pSelectedUnit )
		pSelectedUnit->ShowSelection( false );
	pSelectedUnit = 0;
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( false );
	if ( pUpgradePanel )
		pUpgradePanel->ShowWindow( false );

	for ( int i = 0; i < lines.size(); ++i )
	{
		if ( lines[i].pUnit1 )
			lines[i].pUnit1->SetUnknown();
		if ( lines[i].pUnit2 )
			lines[i].pUnit2->SetUnknown();
	}

	lines.resize( 0 );

	if ( pContainer )
		pContainer->RemoveItems();
}

void SChapterReinfUpgrade::SelectUnit( SUnit *pUnit )
{
	if ( !pUnit || pUnit->data.nCount == 0 )
		return;
	if ( pSelectedUnit )
		pSelectedUnit->ShowSelection( false );
	pSelectedUnit = pUnit;
	if ( pSelectedUnit )
	{
		pSelectedUnit->ShowSelection( true );
/*		for ( int i = 0; i < lines.size(); ++i )
		{
			SUpgradeLine &line = lines[i];
			if ( line.pUnit1 == pUnit || line.pUnit2 == pUnit )
			{
				if ( pContainer )
					pContainer->EnsureElementVisible( line.pWnd );
			}
		}*/
	}
	ShowUnitInfo( pUnit );
}

// SChapterReinfComposition

void SChapterReinfComposition::InitControls( IWindow *_pBaseWnd )
{
	pBaseWnd = _pBaseWnd;
	pCompositionPanel = GetChildChecked<IWindow>( pBaseWnd, "CompositionPanel", true );
	pReinfIconWnd = GetChildChecked<IWindow>( pCompositionPanel, "ReinfIcon", true );
	pReinfHeaderView = GetChildChecked<ITextView>( pCompositionPanel, "ReinfNameView", true );
	if ( pCompositionPanel )
		pCompositionPanel->ShowWindow( false );

	if ( pCompositionPanel )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pCompositionPanel->GetChild( StrFmt( "Unit%d", i ), true );
			if ( !pWnd )
				break;
			units.push_back( new SUnit() );
			SUnit *pUnit = units.back();
			pUnit->InitControls( pWnd, StrFmt( "UnitBtn%d", i ), BASE_ID_3D + DELTA_ID_3D * i );
		}
	}

	InitUnitInfoControls( pCompositionPanel );
}

void SChapterReinfComposition::ShowReinf( const NDb::SReinforcement *pReinf )
{
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( true );
	if ( pCompositionPanel )
		pCompositionPanel->ShowWindow( true );

	if ( pReinfIconWnd )
		pReinfIconWnd->SetTexture( pReinf ? pReinf->pIconTexture : 0 );
	std::wstring wszName;
	if ( pReinf && CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
		wszName = GET_TEXT_PRE(pReinf->,LocalizedName);
	if ( pReinfHeaderView )
		pReinfHeaderView->SetText( pReinfHeaderView->GetDBText() + wszName );

	if ( !pReinf )
	{
		ShowUnitInfo( 0 );
		return;
	}

	std::vector<SUnitData> unitsData;
	MakeReinfData( unitsData, pReinf );
	for ( int i = 0; i < units.size(); ++i )
	{
		SUnit *pUnit = units[i];
		if ( i >= unitsData.size() )
		{
			pUnit->SetNone();
			continue;
		}
		pUnit->SetUnit( unitsData[i] );
	}

	SelectUnit( !units.empty() ? units.front() : 0 );
}

void SChapterReinfComposition::UnitBtnPressed( const std::string &szSender )
{
	if ( !pBaseWnd || !pBaseWnd->IsVisible() )
		return;
	
	for ( int i = 0; i < units.size(); ++i )
	{
		SUnit *pUnit = units[i];
		if ( pUnit->szBtnName == szSender )
		{
			SelectUnit( pUnit );
			break;
		}
	}
}

void SChapterReinfComposition::Hide()
{
	if ( pSelectedUnit )
		pSelectedUnit->ShowSelection( false );
	pSelectedUnit = 0;
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( false );
	if ( pCompositionPanel )
		pCompositionPanel->ShowWindow( false );

	for ( int i = 0; i < units.size(); ++i )
	{
		units[i]->SetNone();
	}
}

void SChapterReinfComposition::SelectUnit( SUnit *pUnit )
{
	if ( !pUnit || pUnit->data.nCount == 0 )
		return;
	if ( pSelectedUnit )
		pSelectedUnit->ShowSelection( false );
	pSelectedUnit = pUnit;
	if ( pSelectedUnit )
		pSelectedUnit->ShowSelection( true );
	ShowUnitInfo( pUnit );
}

// CInterfaceChapterMapMenu::SChapterDesc

void CInterfaceChapterMapMenu::SChapterDesc::InitControls( IWindow *_pBaseWnd )
{
	pBaseWnd = _pBaseWnd;
	
	pPanel = GetChildChecked<IWindow>( pBaseWnd, "ChapterDesc", true );
	pContainer = GetChildChecked<IScrollableContainer>( pPanel, "Container", true );
	pDescView = GetChildChecked<ITextView>( pContainer, "ItemChapterDesc", true );
	pItemGeneral = GetChildChecked<IWindow>( pContainer, "ItemGeneral", true );
	pDescHeader = GetChildChecked<ITextView>( pContainer, "ItemHeader", true );
	
	if ( pPanel )
		pPanel->ShowWindow( false );
	if ( pContainer )
	{
		if ( pItemGeneral )
			pContainer->PushBack( pItemGeneral, false );
		if ( pDescView && pDescHeader )
		{
			pContainer->PushBack( pDescHeader, false );
			pContainer->PushBack( pDescView, false );
		}
	}
}

void CInterfaceChapterMapMenu::SChapterDesc::Show( const NDb::SChapter *pChapter )
{
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( true );
	if ( pPanel )
		pPanel->ShowWindow( true );
	if ( pContainer && pItemGeneral && pChapter )
	{
		CDBPtr<NDb::STexture> pPicture = pChapter->general.pPortrait;
		std::wstring wszDesc;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pChapter->general.,Desc) )
			wszDesc = GET_TEXT_PRE(pChapter->general.,Desc);

		ITextView *pGeneralDesc = GetChildChecked<ITextView>( pItemGeneral, "ItemGeneralDesc", true );
		IWindow *pGeneralPicture = GetChildChecked<IWindow>( pItemGeneral, "ItemGeneralPicture", true );
		if ( pGeneralDesc && pGeneralPicture )
		{
			pGeneralPicture->SetTexture( pPicture );
			pGeneralDesc->SetText( pGeneralDesc->GetDBText() + wszDesc );

			int nTextHeight = 0;
			int nItemHeight = 0;
			pGeneralPicture->GetPlacement( 0, 0, 0, &nItemHeight );
			pGeneralDesc->GetPlacement( 0, 0, 0, &nTextHeight );
			nItemHeight = (std::max)( nItemHeight, nTextHeight ) + 40;
			pItemGeneral->SetPlacement( 0, 0, 0, nItemHeight, EWPF_SIZE_Y );
		}
	}
	if ( pContainer && pDescView )
	{
		std::wstring wszDesc;
		if ( pChapter && CHECK_TEXT_NOT_EMPTY_PRE(pChapter->,LocalizedDescription) )
			wszDesc = GET_TEXT_PRE(pChapter->,LocalizedDescription);
		pDescView->SetText( pDescView->GetDBText() + wszDesc );

		pContainer->Update();
	}
}

void CInterfaceChapterMapMenu::SChapterDesc::Hide()
{
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( false );
	if ( pPanel )
		pPanel->ShowWindow( false );
}

// CInterfaceChapterMapMenu::SMissionDesc

void CInterfaceChapterMapMenu::SMissionDesc::InitControls( IWindow *_pBaseWnd )
{
	pBaseWnd = _pBaseWnd;
	
	pPanel = GetChildChecked<IWindow>( pBaseWnd, "MissionDesc", true );
	pContainer = GetChildChecked<IScrollableContainer>( pPanel, "Container", true );
	pDescView = GetChildChecked<ITextView>( pContainer, "Desc", true );
	pNameView = GetChildChecked<ITextView>( pPanel, "Name", true );
	pDifficultyView = GetChildChecked<ITextView>( pPanel, "Difficulty", true );
	pWeatherView = GetChildChecked<ITextView>( pPanel, "Weather", true );
	
	if ( pPanel )
		pPanel->ShowWindow( false );
	if ( pContainer && pDescView )
		pContainer->PushBack( pDescView, false );
}

void CInterfaceChapterMapMenu::SMissionDesc::Show( const NDb::SMissionEnableInfo &mission )
{
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( true );
	if ( pPanel )
		pPanel->ShowWindow( true );

	if ( pContainer && pDescView )
	{
		std::wstring wszMissionDesc;
		if ( mission.pMap && CHECK_TEXT_NOT_EMPTY_PRE(mission.pMap->,LoadingDescription) )
			wszMissionDesc = GET_TEXT_PRE(mission.pMap->,LoadingDescription);
		pDescView->SetText( pDescView->GetDBText() + wszMissionDesc );
		pContainer->Update();
	}

	std::wstring wszName;
	if ( mission.pMap && CHECK_TEXT_NOT_EMPTY_PRE(mission.pMap->,LocalizedName) )
		wszName = GET_TEXT_PRE(mission.pMap->,LocalizedName);
	if ( pNameView )
		pNameView->SetText( pNameView->GetDBText() + wszName );
	
	std::wstring wszDifficulty;
	if ( IScenarioTracker *pST = Singleton<IScenarioTracker>() )
	{
		if ( const NDb::SDifficultyLevel *pDifficulty = pST->GetDifficultyLevelDB() )
			if ( CHECK_TEXT_NOT_EMPTY_PRE(pDifficulty->,LocalizedName) )
				wszDifficulty = GET_TEXT_PRE(pDifficulty->,LocalizedName);
	}
	if ( pDifficultyView )
		pDifficultyView->SetText( pDifficultyView->GetDBText() + wszDifficulty );

	std::wstring wszWeather;
	switch ( mission.eWeather )
	{
		case NDb::EMW_SUN:
			wszWeather = InterfaceState()->GetTextEntry( "T_MISSION_WEATHER_SUN" );
			break;

		case NDb::EMW_RAIN:
			wszWeather = InterfaceState()->GetTextEntry( "T_MISSION_WEATHER_RAIN" );
			break;

		case NDb::EMW_SNOW:
			wszWeather = InterfaceState()->GetTextEntry( "T_MISSION_WEATHER_SNOW" );
			break;

		case NDb::EMW_SANDSTORM:
			wszWeather = InterfaceState()->GetTextEntry( "T_MISSION_WEATHER_SANDSTORM" );
			break;
	}
	if ( pWeatherView )
		pWeatherView->SetText( pWeatherView->GetDBText() + wszWeather );
}

void CInterfaceChapterMapMenu::SMissionDesc::Hide()
{
	if ( pBaseWnd )
		pBaseWnd->ShowWindow( false );
	if ( pPanel )
		pPanel->ShowWindow( false );
}

REGISTER_SAVELOAD_CLASS( 0x17235340, SChapterReinfUpgrade )
REGISTER_SAVELOAD_CLASS( 0x172354C0, SChapterReinfComposition )
REGISTER_SAVELOAD_CLASS_NM( 0x17235BC0, SUnit, SChapterReinfBase )
REGISTER_SAVELOAD_CLASS_NM( 0x17237BC0, SChapterDesc, CInterfaceChapterMapMenu )
REGISTER_SAVELOAD_CLASS_NM( 0x17237BC1, SMissionDesc, CInterfaceChapterMapMenu )


