#include "stdafx.h"
#include "Misc/StrProc.h"
#include "MissionUnitFullInfo.h"
#include "InterfaceMissionInternal.h"
#include "ScenarioTracker.h"
#include "B2_M1_World/MOBuilding.h"
#include "B2_M1_World/MOUnitMechanical.h"
#include "System/Text.h"

const int SUPPLIES_COUNT = 1000; // CRAP - magic number
const int SUPPLIES_COUNT_INFINITE = 1000000;

const int BASE_ID_3D_OBJECT = 10000;
const int BASE_ID_3D_REINF = 20000;
const int BASE_ID_3D_SUPER_WEAPON = 30000;

// CMissionUnitFullInfo

CMissionUnitFullInfo::CMissionUnitFullInfo()
{
}

CMissionUnitFullInfo::~CMissionUnitFullInfo()
{
	Clear3DView();
}

void CMissionUnitFullInfo::InitByReinf( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason _eSeason )
{
	eSeason = _eSeason;
	
	InitPrivate( pInfo, pAppearanceWnd, ET_REINF );
}

void CMissionUnitFullInfo::InitByMission( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason _eSeason )
{
	eSeason = _eSeason;

	InitPrivate( pInfo, pAppearanceWnd, ET_OBJECT );
}

void CMissionUnitFullInfo::InitBySuperWeapon( IWindow *pInfo, IWindow *pAppearanceWnd, NDb::ESeason _eSeason )
{
	eSeason = _eSeason;

	InitPrivate( pInfo, pAppearanceWnd, ET_SUPER_WEAPON );
}

void CMissionUnitFullInfo::InitPrivate( IWindow *_pInfo, IWindow *pAppearanceWnd, EType _eType )
{
	eType = _eType;
	
	IWindow *pInfo = GetChildChecked<IWindow>( _pInfo, "UnitFullInfo3", true );
	
	pName = GetChildChecked<ITextView>( pInfo, "Name", true );
	pWeaponsWnd = GetChildChecked<IWindow>( pInfo, "Weapons", true );
	pSuppliesWnd = GetChildChecked<IWindow>( pInfo, "Supplies", true );
	pMembersWnd = GetChildChecked<IWindow>( pInfo, "Members", true );

	pFlagWnd = GetChildChecked<IWindow>( pAppearanceWnd, "Flag", true );
	p3DCtrl = GetChildChecked<IWindow3DControl>( pAppearanceWnd, "3DControl", true );
	pFuelPanel = GetChildChecked<IWindow>( pAppearanceWnd, "Fuel", true );
	pFuelBar = GetChildChecked<IProgressBar>( pFuelPanel, "ProgressBar", true );
	if ( pFuelPanel )
		pFuelPanel->ShowWindow( false );

	pArmorsWnd = GetChildChecked<IWindow>( pAppearanceWnd, "Armors", true );
	pArmorFrontView = GetChildChecked<ITextView>( pArmorsWnd, "ArmorFront", true );
	pArmorSide1View = GetChildChecked<ITextView>( pArmorsWnd, "ArmorSide1", true );
	pArmorSide2View = GetChildChecked<ITextView>( pArmorsWnd, "ArmorSide2", true );
	pArmorBackView = GetChildChecked<ITextView>( pArmorsWnd, "ArmorBack", true );
	pArmorTopView = GetChildChecked<ITextView>( pArmorsWnd, "ArmorTop", true );

	pHPView = GetChildChecked<ITextView>( pAppearanceWnd, "HPView", true );
	pHPBarUnit = GetChildChecked<IProgressBar>( pAppearanceWnd, "HPBarUnit", true );
	pHitbarPanel = GetChildChecked<IWindow>( pAppearanceWnd, "HitbarPanel", true );

	if ( pWeaponsWnd )
	{
		for ( int i = 0; ; ++i )
		{
			IWindow *pItem = pWeaponsWnd->GetChild( StrFmt( "WeaponBrief%02d", i + 1 ), true );
			if ( !pItem )
				break;

			weaponItems.push_back( SWeaponItem() );

			SWeaponItem &weaponItem = weaponItems.back();
			weaponItem.pItem = pItem;

			weaponItem.pDamage = GetChildChecked<ITextView>( pItem, "WeaponBriefDamage", true );
			weaponItem.pPenetration = GetChildChecked<ITextView>( pItem, "WeaponBriefPenetration", true );
			weaponItem.pAmmo = GetChildChecked<ITextView>( pItem, "WeaponBriefAmmo", true );
			weaponItem.pCount = GetChildChecked<ITextView>( pItem, "WeaponBriefCount", true );
			weaponItem.pIconBtn = dynamic_cast<IButton*>( pItem->GetChild( StrFmt( "WeaponBtn%d", i ), true ) );
			if ( !weaponItem.pIconBtn )
			{
				weaponItem.pIconBtn = GetChildChecked<IButton>( pItem, "WeaponBtn", true );
				weaponItem.pIconBtn->SetName( StrFmt( "%s%d", weaponItem.pIconBtn->GetName().c_str(), i ) );
			}
			weaponItem.pIcon = GetChildChecked<IWindow>( pItem, "WeaponIcon", true );
		}
		pWeaponFullName = GetChildChecked<ITextView>( pWeaponsWnd, "WeaponFullName", true );
		pWeaponFullDamage = GetChildChecked<ITextView>( pWeaponsWnd, "WeaponFullDamage", true );
		pWeaponFullPenetration = GetChildChecked<ITextView>( pWeaponsWnd, "WeaponFullPenetration", true );
		pWeaponFullAmmo = GetChildChecked<ITextView>( pWeaponsWnd, "WeaponFullAmmo", true );
	}

	if ( pMembersWnd )
	{
		for ( int i = 1; ; ++i )
		{
			IWindow *pWnd = pMembersWnd->GetChild( StrFmt( "Member%02d", i ), true );
			IButton *pBtn = dynamic_cast<IButton*>( pWnd );
			if ( !pBtn )
				break;
				
			SViewMember viewMember;
			viewMember.pBtn = pBtn;
			viewMember.pIconWnd = GetChildChecked<IWindow>( pBtn, "MemberIcon", true );

			viewMembers.push_back( viewMember );
		}
		IWindow *pTransportTooltipHolderWnd = GetChildChecked<IWindow>( pMembersWnd, "TransportTooltipHolder", true );
		IWindow *pBuildingTooltipHolderWnd = GetChildChecked<IWindow>( pMembersWnd, "BuildingTooltipHolder", true );
		if ( pTransportTooltipHolderWnd )
			wszTooltipMemberInTransport = pTransportTooltipHolderWnd->GetDBTooltipStr();
		if ( pBuildingTooltipHolderWnd )
			wszTooltipMemberInBuilding = pBuildingTooltipHolderWnd->GetDBTooltipStr();
	}
	
	if ( pSuppliesWnd )
	{
		pSuppliesCountView = GetChildChecked<ITextView>( pSuppliesWnd, "SuppliesCount", true );
		pSuppliesCountInfinite = GetChildChecked<ITextView>( pSuppliesWnd, "SuppliesInfinite", true );
	}

	if ( pInfo )
		pInfo->ShowWindow( true );
		
	ClearInfo();
	
	int nBaseID;
	if ( eType == ET_OBJECT )
		nBaseID = BASE_ID_3D_OBJECT;
	else if ( eType == BASE_ID_3D_REINF )
		nBaseID = BASE_ID_3D_REINF;
	else
		nBaseID = BASE_ID_3D_SUPER_WEAPON;
	SetBaseID3D( nBaseID );
}

void CMissionUnitFullInfo::ClearInfo()
{
	pBaseStats = 0;
	pBaseMO = 0;
	pSelStats = 0;
	pSelMO = 0;
	nSelWeapon = 0;
	nSelMember = -1;
	nCurSupplies = -1;
	nMaxSupplies = -1;
	wszLocalizedName = L"";
	fFuel = -1.0f;
	nPlayer = 0;
	
	weapons.clear();
	members.clear();
	armors.clear();
	objects.clear();
	
	HideSpecific();

	Clear3DView();
}

void CMissionUnitFullInfo::HideSpecific()
{
	for ( vector<SWeaponItem>::iterator it = weaponItems.begin(); it != weaponItems.end(); ++it )
	{
		SWeaponItem &item = *it;
		if ( item.pItem )
			item.pItem->ShowWindow( false );
	}

	for ( vector<SViewMember>::iterator it = viewMembers.begin(); it != viewMembers.end(); ++it )
	{
		SViewMember &viewMember = *it;
		if ( viewMember.pBtn )
			viewMember.pBtn->ShowWindow( false );
	}

	if ( pWeaponsWnd )
		pWeaponsWnd->ShowWindow( false );
	if ( pSuppliesWnd )
		pSuppliesWnd->ShowWindow( false );
	if ( pMembersWnd )
		pMembersWnd->ShowWindow( false );
	if ( pFlagWnd )
		pFlagWnd->ShowWindow( false );
	if ( pArmorsWnd )
		pArmorsWnd->ShowWindow( false );
	if ( pHitbarPanel )
		pHitbarPanel->ShowWindow( false );
	if ( pFuelPanel )
		pFuelPanel->ShowWindow( false );
}

void CMissionUnitFullInfo::Clear3DView()
{
	if ( p3DCtrl )
		p3DCtrl->SetObjects( vector<IWindow3DControl::SObject>() );
}

void CMissionUnitFullInfo::SetReinfUnit( const NDb::SHPObjectRPGStats *pStats )
{
	ClearInfo();

	if ( !pStats )
		return;

	pBaseStats = pStats;
	pSelStats = pStats;

//	MakeName();
	MakeMembers( &members );

	MakeCurrent();

	MakeCurrentHP();

//	ShowName();
	ShowMembers();

	ShowCurrent();
}

void CMissionUnitFullInfo::SetObject( CMapObj *pMO )
{
	if ( pMO && pMO == pBaseMO )
	{
		UpdateObject( pMO );
		return;
	}
	
	ClearInfo();
	
	if ( !pMO )
		return;

	pBaseMO = pMO;
	pSelMO = pMO;
		
	const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
	NI_VERIFY( pStats, "Incorrect unit's stats", return );
	
	pBaseStats = pStats;
	pSelStats = pStats;

//	MakeName();
	MakeMembers( &members );

	MakeCurrent();

//	ShowName();
	ShowMembers();

	ShowCurrent();
}

void CMissionUnitFullInfo::UpdateObject( CMapObj *pMO )
{
	if ( !pMO )
		return;
	if ( pMO != pBaseMO && pMO != pSelMO )
		return;

	const NDb::SHPObjectRPGStats *pStats = pMO->GetStats();
	NI_VERIFY( pStats, "Incorrect unit's stats", return );
	
	bool bSimilarMembers = false;
	bool bMembersDetailsUpdated = false;
	if ( pMO == pBaseMO )
	{
		vector<SMember> oldMembers;
		members.swap( oldMembers );
		MakeMembers( &members );
		bSimilarMembers = (members == oldMembers);
		
		if ( !bSimilarMembers )
		{
			pSelStats = pBaseStats;
			pSelMO = pBaseMO;
			nSelWeapon = 0;
			nSelMember = -1;
			
			MakeCurrent();
		}
		else
		{
			for ( int i = 0; i < members.size(); ++i )
			{
				if ( members[i].nPassengerCount != oldMembers[i].nPassengerCount )
				{
					bMembersDetailsUpdated = true;
					break;
				}
			}
		}
	}
	
	if ( bSimilarMembers || bMembersDetailsUpdated )
	{
		SObjectStatus status;
		pSelMO->GetStatus( &status );
		
		if ( !status.bIsTransport )
		{
			if ( !IsSameWeapons( status ) )
			{
				nSelWeapon = 0;
				MakeWeapons( status, dynamic_cast<IMOSquad*>( pSelMO.GetPtr() ) != 0 );
			}
			else
			{
				UpdateWeaponsAmmo( status );
			}
		}
		else
			MakeCurrentSupplies();

		MakeCurrentHP();
		MakeCurrentFuel();

		MakeCurrent3DObjects(); // TODO: можно оптимизировать и вызывать только на смерть пехотинца из отряда
	}
	
//	if ( bMembersUpdated )
		ShowMembers();
	ShowCurrent();
}

void CMissionUnitFullInfo::MakeName()
{
	if ( pSelStats )
	{
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pSelStats->,LocalizedName) )
			wszLocalizedName = GET_TEXT_PRE(pSelStats->,LocalizedName);
		else
			wszLocalizedName = L"";
	}
}

void CMissionUnitFullInfo::MakeMembers( vector<SMember> *pMembers )
{
	IMOContainer *pMOCont = dynamic_cast_ptr<IMOContainer*>( pBaseMO );
	if ( !pMOCont )
	{
		bool bResult = !pMembers->empty();
		pMembers->clear();
		return;
	}

	vector<CMOSelectable*> passengers;
	if ( pMOCont->GetTypeID() != NDb::SSquadRPGStats::typeID )
	{
		pMOCont->GetPassangers( &passengers );
		for ( vector<CMOSelectable*>::iterator it = passengers.begin(); it != passengers.end(); ++it )
		{
			CMOSelectable *pPassenger = *it; 
			if ( !pPassenger )
				continue;
				
			NI_ASSERT( pPassenger->IsAlive(), "Programmers: dead unit inside container" );
				
			SMember member;
			member.pStats = pPassenger->GetStats();
			member.pMO = pPassenger;
			member.nPassengerCount = 0;
			
			if ( CDynamicCast<IMOSquad> pSquad = pPassenger )
			{
				member.nPassengerCount = pSquad->GetPassangersCount();
			}

			pMembers->push_back( member );
		}
	}
}

void CMissionUnitFullInfo::MakeCurrentWeapons()
{
	weapons.clear();
	
	if ( pSelMO )
	{
		SObjectStatus status;
		pSelMO->GetStatus( &status );
		MakeWeapons( status, dynamic_cast_ptr<IMOSquad*>( pSelMO ) != 0 );
	}
	else
	{
		NUnitFullInfo::MakeWeapons( weapons, pSelStats );
	}
}

void CMissionUnitFullInfo::MakeCurrentArmor()
{
	NUnitFullInfo::MakeArmors( armors, pSelStats );
}

void CMissionUnitFullInfo::MakeCurrentSupplies()
{
	nCurSupplies = -1;
	nMaxSupplies = -1;

	if ( pSelMO )
	{
		if ( CDynamicCast<CMOUnitMechanical> pMech = pSelMO )
		{
			if ( const NDb::SMechUnitRPGStats *pMechStats = dynamic_cast_ptr<const NDb::SMechUnitRPGStats*>( pSelStats ) )
			{
				if ( pMechStats->IsTransport() )
				{
					if ( pMechStats->pActions && NUnitFullInfo::IsResourcesCarrier( pMechStats->pActions->availUserActions ) )
					{
						SObjectStatus status;
						pSelMO->GetStatus( &status );
						if ( status.bIsTransport )
						{
							nCurSupplies = status.nSupply;
							nMaxSupplies = SUPPLIES_COUNT;
						}
					}
				}
			}
		}
		else if ( CDynamicCast<CMOBuilding> pBuilding = pSelMO )
		{
			if ( pBuilding->IsStorage() )
				nMaxSupplies = SUPPLIES_COUNT_INFINITE;
		}
	}
	else if ( const NDb::SMechUnitRPGStats *pMechStats = dynamic_cast_ptr<const NDb::SMechUnitRPGStats*>( pSelStats ) )
	{
		if ( pMechStats->IsTransport() )
		{
			if ( pMechStats->pActions && NUnitFullInfo::IsResourcesCarrier( pMechStats->pActions->availUserActions ) )
				nMaxSupplies = SUPPLIES_COUNT;
		}
	}
}

void CMissionUnitFullInfo::MakeCurrentFlag()
{
	pKeyObjectTexture = 0;
	
	if ( pSelMO && pSelMO->IsKeyObject() )
	{
		if ( const NDb::SMapInfo *pMapInfo = Singleton<IScenarioTracker>()->GetCurrentMission() )
		{
			if ( pSelMO->GetKeyObjectPlayer() < pMapInfo->players.size() )
			{
				const NDb::SMapPlayerInfo &info = pMapInfo->players[pSelMO->GetKeyObjectPlayer()];
				const NDb::SPartyDependentInfo *pPartyInfo = Singleton<IScenarioTracker>()->GetPlayerParty( pSelMO->GetKeyObjectPlayer() );
				if ( pPartyInfo )
				{
					pKeyObjectTexture = pPartyInfo->pMinimapKeyObjectIcon;
				}
			}
		}
	}
}

void CMissionUnitFullInfo::MakeCurrent3DObjects()
{
	if ( !p3DCtrl )
		return;
	objects.clear();

	if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast_ptr<const NDb::SSquadRPGStats*>( pSelStats ) )
	{
		if ( const IMOSquad *pSquad = dynamic_cast_ptr<const IMOSquad*>( pSelMO ) )
		{
			vector<CMOSelectable*> passengers;
			pSquad->GetPassangers( &passengers );
			for ( int i = 0; i < Min( 3, passengers.size() ); ++i )
			{
				CMOSelectable *pSO = passengers[i];
				if ( !pSO )
					continue;
				IWindow3DControl::SParam param = p3DCtrl->GetDBObjectParam( i + 1 );
				IWindow3DControl::SObject object;
				object.nID = i;
				object.pModel = pSO->GetModelDesc();
				object.vPos = param.vPos;
				object.vSize = param.vSize;
				object.pAnim = !pSO->GetStats() ? 0 : NUnitFullInfo::FindAnimation( dynamic_cast<const NDb::SInfantryRPGStats*>( pSO->GetStats() ) ) ;
				objects.push_back( object );
			}
		}
		else
		{
			for ( int i = 0; i < Min( 3, pSquadStats->members.size() ); ++i )
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
				object.pModel = GetModel( pVisObj, eSeason );
				object.vPos = param.vPos;
				object.vSize = param.vSize;
				object.pAnim = NUnitFullInfo::FindAnimation( pMember ) ;
				objects.push_back( object );
			}
		}
	}
	else
	{
		IWindow3DControl::SParam param = p3DCtrl->GetDBObjectParam( 0 );
		IWindow3DControl::SObject object;
		object.nID = 0;
		const NDb::SVisObj *pVisObj = 0;
		if ( pSelStats )
		{
			pVisObj = pSelStats->pinfoVisualObject;
			if ( !pVisObj )
				pVisObj = pSelStats->pvisualObject;
		}
		object.pModel = pSelMO ? pSelMO->GetModelDesc() : (pSelStats ? GetModel( pVisObj, eSeason ) : 0);
		object.vPos = param.vPos;
		object.vSize = param.vSize;
		objects.push_back( object );
	}
}

void CMissionUnitFullInfo::MakeCurrentHP()
{
	hps.clear();
	nPlayer = 0;
	
	if ( pSelMO )
	{
		nPlayer = pSelMO->GetPlayer();
		NI_ASSERT( nPlayer >= 0, "Object's player < 0" );
		if ( CDynamicCast<IMOSquad> pSquad = pSelMO )
		{
			if ( const NDb::SSquadRPGStats *pSquadStats = dynamic_cast_ptr<const NDb::SSquadRPGStats*>( pSelStats ) )
			{
				if ( !pSquadStats->members.empty() )
				{
					SHP hp;
					hp.fFraction = (float)( pSquad->GetPassangersCount() ) / (float)( pSquadStats->members.size() );
					hp.nHP = pSquad->GetPassangersCount();
					hps.push_back( hp );
				}
			}
		}
		else
		{
			SObjectStatus status;
			pSelMO->GetStatus( &status );

			SHP hp;
			hp.fFraction = pSelMO->GetHP();
			float fHP = hp.fFraction * status.nMaxHP;
			hp.nHP = fHP > 1.0f ? fHP : ( fHP > 0.0f ? 1.0f : 0.0f );	
			hps.push_back( hp );
		}
	}
	else if ( pSelStats )
	{
		SHP hp;
		hp.fFraction = 1.0f;
		hp.nHP = pSelStats->fMaxHP;	
		hps.push_back( hp );
	}
}

void CMissionUnitFullInfo::MakeCurrentFuel()
{
	fFuel = -1.0f;
	if ( pSelMO )
	{
		if ( CDynamicCast<const NDb::SMechUnitRPGStats> pStats = pSelMO->GetStats() )
		{
			if ( pStats->IsAviation() )
			{
				SObjectStatus status;
				pSelMO->GetStatus( &status );
				fFuel = Clamp( status.fFuel / pStats->fFuel, 0.0f, 1.0f );
			}
		}
	}
}

void CMissionUnitFullInfo::MakeCurrent()
{
	MakeName();
	MakeCurrentWeapons();
	MakeCurrentArmor();
	MakeCurrentSupplies();
	MakeCurrentFlag();
	MakeCurrent3DObjects();
	MakeCurrentHP();
	MakeCurrentFuel();
}

void CMissionUnitFullInfo::UpdateMembers( bool bCanLeave )
{
	for ( vector<SViewMember>::iterator it = viewMembers.begin(); it != viewMembers.end(); ++it )
	{
		SViewMember &viewMember = *it;
		if ( viewMember.pBtn )
		{
			viewMember.pBtn->Enable( bCanLeave );
		}
	}
}

void CMissionUnitFullInfo::MakeWeapons( const SObjectStatus &status, bool bSquad )
{
	weapons.clear();

	for ( int i = 0; i < status.weapons.size(); ++i )
	{
		const SObjectStatus::SWeapon &statusWeapon = status.weapons[i];
		
		NUnitFullInfo::SWeapon weapon;
		weapon.pWeapon = statusWeapon.pWeaponID;
		weapon.nCount = statusWeapon.nCount;
		weapon.bPrimary = statusWeapon.bPrimary;
		weapon.wszLocalizedName = statusWeapon.szLocalizedName;
		weapon.nDamage = statusWeapon.nDamage;
		weapon.nPenetration = statusWeapon.nPenetration;
		weapon.nAmmo = statusWeapon.nAmmo;
		weapon.nMaxAmmo = statusWeapon.nMaxAmmo;
		
		weapons.push_back( weapon );
	}
//	if ( bSquad )
		sort( weapons.begin(), weapons.end(), NUnitFullInfo::SWeaponsSort() );
}

void CMissionUnitFullInfo::UpdateWeaponsAmmo( const struct SObjectStatus &status )
{
	for ( int i = 0; i < status.weapons.size(); ++i )
	{
		const SObjectStatus::SWeapon &statusWeapon = status.weapons[i];
		NUnitFullInfo::SWeapon &weapon = weapons[i];
		
		weapon.nAmmo = statusWeapon.nAmmo;
	}
}

bool CMissionUnitFullInfo::IsSameWeapons( const struct SObjectStatus &status ) const
{
	if ( weapons.size() != status.weapons.size() )
		return false;
	for ( int i = 0; i < weapons.size(); ++i )
	{
		if ( weapons[i].pWeapon != status.weapons[i].pWeaponID )
			return false;
		if ( weapons[i].nCount != status.weapons[i].nCount )
			return false;
	}
	return true;
}

void CMissionUnitFullInfo::ShowArmor()
{
	if ( armors.size() < NUnitFullInfo::ES_ARMOR_COUNT )
	{
		if ( pArmorsWnd )
			pArmorsWnd->ShowWindow( false );
		return;
	}

	if ( pArmorsWnd )
		pArmorsWnd->ShowWindow( true );
		
	wstring wszArmorFront = NStr::ToUnicode( StrFmt( "%d", armors[NUnitFullInfo::ES_ARMOR_FRONT] ) );
	wstring wszArmorSide = NStr::ToUnicode( StrFmt( "%d", armors[NUnitFullInfo::ES_ARMOR_SIDE] ) );
	wstring wszArmorBack = NStr::ToUnicode( StrFmt( "%d", armors[NUnitFullInfo::ES_ARMOR_BACK] ) );
	wstring wszArmorTop = NStr::ToUnicode( StrFmt( "%d", armors[NUnitFullInfo::ES_ARMOR_TOP] ) );
	
	if ( pArmorFrontView )
	{
		pArmorFrontView->SetText( pArmorFrontView->GetDBText() + wszArmorFront );
	}
	
	if ( pArmorSide1View )
	{
		pArmorSide1View->SetText( pArmorSide1View->GetDBText() + wszArmorSide );
	}

	if ( pArmorSide2View )
	{
		pArmorSide2View->SetText( pArmorSide2View->GetDBText() + wszArmorSide );
	}

	if ( pArmorBackView )
	{
		pArmorBackView->SetText( pArmorBackView->GetDBText() + wszArmorBack );
	}

	if ( pArmorTopView )
	{
		pArmorTopView->SetText( pArmorTopView->GetDBText() + wszArmorTop );
	}
}

void CMissionUnitFullInfo::ShowWeapons()
{
	if ( weapons.empty() )
	{
		if ( pWeaponsWnd )
			pWeaponsWnd->ShowWindow( false );
		return;
	}

	if ( pWeaponsWnd )
		pWeaponsWnd->ShowWindow( true );

	for ( int i = 0; i < weaponItems.size(); ++i )
	{
		if ( i >= weapons.size() )
		{
			if ( weaponItems[i].pItem )
				weaponItems[i].pItem->ShowWindow( false );
			continue;
		}
		
		if ( weaponItems[i].pItem )
			weaponItems[i].pItem->ShowWindow( true );
			
		wstring wszDamage = NStr::ToUnicode( StrFmt( "%d", weapons[i].nDamage ) );
		wstring wszPenetration = NStr::ToUnicode( StrFmt( "%d", weapons[i].nPenetration ) );
		wstring wszAmmo = NStr::ToUnicode( StrFmt( "%d/%d", weapons[i].nAmmo, weapons[i].nMaxAmmo ) );
		wstring wszCount = NStr::ToUnicode( StrFmt( "%d", weapons[i].nCount ) );
		
		if ( weaponItems[i].pDamage )
			weaponItems[i].pDamage->SetText( weaponItems[i].pDamage->GetDBText() + wszDamage );
		if ( weaponItems[i].pPenetration )
			weaponItems[i].pPenetration->SetText( weaponItems[i].pPenetration->GetDBText() + wszPenetration );
		if ( weaponItems[i].pAmmo )
			weaponItems[i].pAmmo->SetText( weaponItems[i].pAmmo->GetDBText() + wszAmmo );
		if ( weaponItems[i].pCount )
		{
			weaponItems[i].pCount->SetText( weaponItems[i].pCount->GetDBText() + wszCount );
		}

		if ( weaponItems[i].pIconBtn )
		{
			weaponItems[i].pIconBtn->SetState( i == nSelWeapon ? 1 : 0 );
			weaponItems[i].pIconBtn->SetTooltip( weaponItems[i].pIconBtn->GetDBTooltipStr() + weapons[i].wszLocalizedName );
		}
		if ( weaponItems[i].pIcon )
		{
			if ( weapons[i].pWeapon )
				weaponItems[i].pIcon->SetTexture( weapons[i].pWeapon->pWeaponTypeTexture );
		}
		
		if ( i == nSelWeapon )
		{
			if ( pWeaponFullName )
			{
				if ( weapons[i].nCount <= 1 )
					pWeaponFullName->SetText( pWeaponFullName->GetDBText() + weapons[i].wszLocalizedName );
				else
					pWeaponFullName->SetText( pWeaponFullName->GetDBText() + wszCount + L"x " + weapons[i].wszLocalizedName );
			}
			if ( pWeaponFullDamage )
				pWeaponFullDamage->SetText( pWeaponFullDamage->GetDBText() + wszDamage );
			if ( pWeaponFullPenetration )
				pWeaponFullPenetration->SetText( pWeaponFullPenetration->GetDBText() + wszPenetration );
			if ( pWeaponFullAmmo )
				pWeaponFullAmmo->SetText( pWeaponFullAmmo->GetDBText() + wszAmmo );
		}
	}
}

void CMissionUnitFullInfo::ShowResources()
{
	if ( !weapons.empty() || nMaxSupplies < 0 )
	{
		if ( pSuppliesWnd )
			pSuppliesWnd->ShowWindow( false );
		return;
	}

	if ( pSuppliesWnd )
		pSuppliesWnd->ShowWindow( true );

	if ( nMaxSupplies == SUPPLIES_COUNT_INFINITE )
	{
		if ( pSuppliesCountView )
			pSuppliesCountView->SetText( pSuppliesCountView->GetDBText() + pSuppliesCountInfinite->GetDBText() );
	}
	else
	{
		wstring wszValue = NStr::ToUnicode( StrFmt( "%d", nMaxSupplies ) );
		if ( nCurSupplies >= 0 )
			wszValue = NStr::ToUnicode( StrFmt( "%d", nCurSupplies ) ) + L"/" + wszValue;
		if ( pSuppliesCountView )
			pSuppliesCountView->SetText( pSuppliesCountView->GetDBText() + wszValue );
	}
}

void CMissionUnitFullInfo::ShowMembers()
{
	if ( members.empty() )
	{
		if ( pMembersWnd )
			pMembersWnd->ShowWindow( false );
		return;
	}

	if ( pMembersWnd )
		pMembersWnd->ShowWindow( true );
		
	for ( int i = 0; i < viewMembers.size(); ++i )
	{
		SViewMember &viewMember = viewMembers[i];
		if ( i >= members.size() )
		{
			if ( viewMember.pBtn )
				viewMember.pBtn->ShowWindow( false );
			continue;
		}
		
		SMember &member = members[i];
		
		if ( viewMember.pBtn )
		{
			viewMember.pBtn->ShowWindow( true );
			SetMemberTooltip( viewMember.pBtn );
		}
		if ( viewMember.pIconWnd && member.pStats )
			viewMember.pIconWnd->SetTexture( member.pStats->pIconTexture );
	}
}

void CMissionUnitFullInfo::ShowFlag()
{
	if ( !pKeyObjectTexture )
	{
		if ( pFlagWnd )
			pFlagWnd->ShowWindow( false );
		return;
	}

	if ( pFlagWnd )
		pFlagWnd->ShowWindow( true );
		
	if ( pFlagWnd )
		pFlagWnd->SetTexture( pKeyObjectTexture );
}

void CMissionUnitFullInfo::ShowName()
{
	if ( pName )
		pName->SetText( pName->GetDBText() + wszLocalizedName );
}

void CMissionUnitFullInfo::Show3DObjects()
{
	if ( p3DCtrl )
	{
		p3DCtrl->SetObjects( objects );
	}
}

void CMissionUnitFullInfo::ShowHP()
{
	if ( hps.size() != 1 )
	{
		if ( pHitbarPanel )
			pHitbarPanel->ShowWindow( false );
		if ( pHPView )
			pHPView->ShowWindow( false );
		if ( pHPBarUnit )
			pHPBarUnit->ShowWindow( false );
		return;
	}

	if ( hps[0].nHP >= 0 )
	{
		if ( pHitbarPanel )
			pHitbarPanel->ShowWindow( true );
		if ( pHPView )
			pHPView->ShowWindow( true );
		if ( pHPBarUnit )
			pHPBarUnit->ShowWindow( true );

//		float fHP = hps[0].fFraction * hps[0].fMax;
//		int nHP = fHP > 1.0f ? fHP : ( fHP != 0.0f ? 1.0f : 0.0f );
		wstring wszValue = NStr::ToUnicode( StrFmt( "%d", hps[0].nHP ) );
		if ( pHPView )
		{
			pHPView->SetText( pHPView->GetDBText() + wszValue );
		}
		
		if ( pHPBarUnit )
		{
			const IScenarioTracker::SPlayerColor &color = Singleton<IScenarioTracker>()->GetPlayerColor( nPlayer );
			pHPBarUnit->SetForward( color.pUnitFullInfo );
			pHPBarUnit->SetPosition( hps[0].fFraction );
		}
	}
}

void CMissionUnitFullInfo::ShowFuel()
{
	if ( fFuel == -1.0f )
	{
		if ( pFuelPanel )
			pFuelPanel->ShowWindow( false );
	}
	else
	{
		if ( pFuelPanel )
			pFuelPanel->ShowWindow( true );
		if ( pFuelBar )
			pFuelBar->SetPosition( fFuel );
	}
}

void CMissionUnitFullInfo::ShowCurrent()
{
	ShowName();
	ShowWeapons();
	ShowResources();
	ShowFlag();
	ShowArmor();
	Show3DObjects();
	ShowHP();
	ShowFuel();
}

void CMissionUnitFullInfo::OnClickMember( const string &szSender )
{
	for ( int i = 0; i < viewMembers.size(); ++i )
	{
		if ( viewMembers[i].pBtn && szSender == viewMembers[i].pBtn->GetName() )
		{
			NInput::PostEvent( "unload_unit", i, 0 );
			break;
		}
	}
}

void CMissionUnitFullInfo::OnMemberOverOn( const string &szSender )
{
	for ( int i = 0; i < Min( viewMembers.size(), members.size() ); ++i )
	{
		if ( viewMembers[i].pBtn && szSender == viewMembers[i].pBtn->GetName() )
		{
			pSelStats = members[i].pStats;
			pSelMO = members[i].pMO;
			nSelMember = i;
			MakeCurrent();
			ShowCurrent();
			break;
		}
	}
}

void CMissionUnitFullInfo::OnMemberOverOff( const string &szSender )
{
	pSelStats = pBaseStats;
	pSelMO = pBaseMO;
	nSelMember = -1;
	MakeCurrent();
	ShowCurrent();
}

void CMissionUnitFullInfo::OnWeaponOverOn( const string &szSender )
{
	for ( int i = 0; i < Min( weaponItems.size(), weapons.size() ); ++i )
	{
		if ( weaponItems[i].pIconBtn && szSender == weaponItems[i].pIconBtn->GetName() )
		{
			nSelWeapon = i;
			MakeCurrent();
			ShowCurrent();
			break;
		}
	}
}

void CMissionUnitFullInfo::OnWeaponOverOff( const string &szSender )
{
	nSelWeapon = 0;
	MakeCurrent();
	ShowCurrent();
}

void CMissionUnitFullInfo::SetBaseID3D( int nID )
{
	if ( p3DCtrl )
		p3DCtrl->SetBaseID3D( nID );
}

void CMissionUnitFullInfo::SetMemberTooltip( IWindow *pWnd )
{
	if ( !pWnd )
		return;
	if ( !pWnd->IsVisible() )
		return;

	bool bInBuilding = false;
	if ( dynamic_cast_ptr<CMOBuilding*>( pBaseMO ) != 0 )
		bInBuilding = true;
		
	if ( !bInBuilding )
		pWnd->SetTooltip( wszTooltipMemberInTransport );
	else
		pWnd->SetTooltip( wszTooltipMemberInBuilding );
}

REGISTER_SAVELOAD_CLASS( 0x17169340, CMissionUnitFullInfo )


