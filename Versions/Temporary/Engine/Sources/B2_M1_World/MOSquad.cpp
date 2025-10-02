#include "StdAfx.h"

#include "MOSquad.h"
#include "MOUnitInfantry.h"
#include "MOUnitMechanical.h"
#include "..\Input\Bind.h"
#include "..\Stats_B2_M1\IClientGameConsts.h"
#include "..\Stats_B2_M1\ActionsRemap.h"

namespace
{

typedef hash_map< NDb::EDesignSquadType, SIconsSetInfo, SEnumHash > CIconsSet;
static bool bIsInitializedByDB = false;
CIconsSet iconsSets;
SIconsSetInfo iconsSetDefault;

const SIconsSetInfo& GetDBIconsSet( NDb::EDesignSquadType eType )
{
	if ( !bIsInitializedByDB )
	{		
		const NDb::SClientGameConsts *pClient = Singleton<IClientGameConsts>()->GetClientGameConsts();
		
		for ( int i = 0; i < pClient->squadIconsSets.size(); ++i )
		{
			const NDb::SClientGameConsts::SSquadIconsSet &iconsSet = pClient->squadIconsSets[i];
			iconsSets[iconsSet.eType] = SIconsSetInfo( iconsSet.fRaising, iconsSet.fHPBarLen );
			if ( iconsSet.eType == NDb::SQUAD_TYPE_UNKNOWN )
			{
				iconsSetDefault.fRaising = iconsSet.fRaising;
				iconsSetDefault.fHPBarLen = iconsSet.fHPBarLen;
			}
		}

		bIsInitializedByDB = true;
	}

	CIconsSet::const_iterator it = iconsSets.find( eType );
	if ( it == iconsSets.end() )
		return iconsSetDefault;
	return it->second;
}

} //namespace

CMOSquad::CMOSquad()
//	bCanCatchArtillery( false )
{
}

CMOSquad::~CMOSquad(void)
{
	while ( !units.empty() ) 
	{
		CObj<IMOUnit> pUnit = units.back();
		units.pop_back();
		pUnit->SetSquad( 0 );
	}
}

void CMOSquad::SendAcknowledgement( interface IClientAckManager *pAckManager, const NDb::EUnitAckType eAck )
{
	if ( !units.empty() )
		(*units.begin())->SendAcknowledgement( pAckManager, eAck );
}

bool CMOSquad::AIUpdateSpecialAbility( const struct SAISpecialAbilityUpdate &update )
{
	bool bCumulativeRet = false;
	for( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		const bool bRet = pUnit->AIUpdateSpecialAbility( update );
		bCumulativeRet |= bRet;
	}
	return bCumulativeRet;
}

bool CMOSquad::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor )
{
	const SAINewUnitUpdate *pUpdate = checked_cast<const SAINewUnitUpdate *>( _pUpdate );

	const NDb::SSquadRPGStats *pStats = checked_cast_ptr<const NDb::SSquadRPGStats *>( pUpdate->info.pStats );
	SetStats( pStats );

	if ( !bInEditor ) 
	{
		if ( pStats->iconsSetParams.bCustom )
			SetIconsSetInfo( SIconsSetInfo( pStats->iconsSetParams.fRaising, pStats->iconsSetParams.fHPBarLen ) );
		else
		{
			const SIconsSetInfo &info = GetDBIconsSet( pStats->eSquadType );
			SetIconsSetInfo( info );
		}
	}

	return CMapObj::Create( nUniqueID, pUpdate, eSeason, eDayTime, bInEditor );
}

void CMOSquad::GetStatus( SObjectStatus *pStatus ) const
{
	static int nLevel = 0; // DEBUG
	
	IMOSquad::GetStatus( pStatus );
	
	NI_VERIFY( nLevel == 0, "Can't use nested squads", return );

	++nLevel;
	int nCount = 0;
	int nMaxArmor = -1;
	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		++nCount;
		
		SObjectStatus status;
		pUnit->GetStatus( &status );
		
		// Armor
		if ( status.armors[0] > nMaxArmor )
		{
			nMaxArmor = status.armors[0];
			for ( int i = 0; i < EOS_ARMOR_COUNT; ++i )
			{
				pStatus->armors[i] = status.armors[i];
			}
			pStatus->pArmorPattern = status.pArmorPattern;
		}
		
		// Weapons
		for ( int i = 0; i < status.weapons.size(); ++i )
		{
			pStatus->AddWeapon( status.weapons[i] );
		}
	}
	
	--nLevel;
}

bool CMOSquad::IsInSquad( interface IMOUnit *pUnit )
{
	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		if ( *it == pUnit )
			return true;
	}
	return false;
}

void CMOSquad::GetPassangers( vector<CMOSelectable*> *pBuffer ) const
{
	NI_ASSERT( pBuffer, "Wrong pointer" );
	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		pBuffer->push_back( *it );
	}
}

bool CMOSquad::NeedShowInterrior() const
{
	return false;
}

void CMOSquad::GetActions( CUserActions *pActions, EActionsType eActions ) const
{
	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		if ( !pUnit->IsAlive() )
			continue;
		pUnit->GetActions( pActions, eActions );
	}
	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{
		const NDb::SSquadRPGStats* pSt = checked_cast<const NDb::SSquadRPGStats*>(GetStats());
		if ( pSt->formations.empty() )
			pActions->RemoveAction( NDb::USER_ACTION_FORMATION );
		else
		{

		}
	}
}

void CMOSquad::GetPossibleActions( CUserActions *pActions ) const
{
	if ( !GetStats() )
		return;

	const NDb::SSquadRPGStats* pSt = checked_cast<const NDb::SSquadRPGStats*>(GetStats());

	for ( int i = 0; i < pSt->members.size(); ++i )
	{
		const NDb::SInfantryRPGStats *pMemberStats = pSt->members[i];
		const NDb::SUnitActions *pMemberActions = pMemberStats->pActions;
		if ( !pMemberActions )
			continue;

		*pActions |= pMemberActions->availUserActions;
		for ( int i = 0; i < pMemberActions->specialAbilities.size(); ++i )
		{
			const NDb::SUnitSpecialAblityDesc *pDesc = pMemberActions->specialAbilities[ i ];
			if ( !pDesc )
				continue;
			pActions->SetAction( GetActionByAbility( pDesc->eName ) );
		}
		if ( pActions->HasAction( NDb::USER_ACTION_MOVE ) )
			pActions->SetAction( NDb::USER_ACTION_MOVE_LIKE_TERRAIN );
	}
	
	if ( pSt->formations.empty() )
		pActions->RemoveAction( NDb::USER_ACTION_FORMATION );
	else
	{
		pActions->SetAction( NDb::USER_ACTION_FORMATION );
		for ( int i = 0; i < pSt->formations.size(); ++i )
			pActions->SetAction( NDb::USER_ACTION_FORMATION_0 + pSt->formations[i].etype );
	}
}

void CMOSquad::GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	if ( eActions == ACTIONS_BY || eActions == ACTIONS_ALL )
	{
		if ( pServedGun )
			pActions->SetAction( NDb::USER_ACTION_CAPTURE_ARTILLERY );

		pActions->SetAction( NDb::USER_ACTION_FORMATION_0 );
		pActions->SetAction( NDb::USER_ACTION_FORMATION_1 );
		pActions->SetAction( NDb::USER_ACTION_FORMATION_2 );
		pActions->SetAction( NDb::USER_ACTION_FORMATION_3 );
		const NDb::SSquadRPGStats* pSt = checked_cast<const NDb::SSquadRPGStats*>(GetStats());
		for (vector< NDb::SSquadRPGStats::SFormation >::const_iterator it = pSt->formations.begin(); it != pSt->formations.end(); ++it )
		{
			switch ( it->etype )
			{
			case NDb::SSquadRPGStats::SFormation::OFFENSIVE:
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_3 );
				break;
			case NDb::SSquadRPGStats::SFormation::DEFAULT:
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_0 );
				break;
			case NDb::SSquadRPGStats::SFormation::DEFENSIVE:
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_2 );
				break;
			case NDb::SSquadRPGStats::SFormation::MOVEMENT:
				pActions->RemoveAction( NDb::USER_ACTION_FORMATION_1 );
				break;
			}
		}
	}
}

void CMOSquad::GetEnabledActions( CUserActions *pActions, EActionsType eActions ) const
{
	return IMOSquad::GetEnabledActions( pActions, eActions );
}

void CMOSquad::Select( bool bSelect )
{
	CMOSelectable::Select( bSelect );

	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
  {
    (*it)->Select( bSelect );
  }
}

void CMOSquad::SetSelectionGroup( int nSelectionGroup )
{
	CMOSelectable::SetSelectionGroup( nSelectionGroup );

	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
  {
    (*it)->SetSelectionGroup( nSelectionGroup );
  }
}

void CMOSquad::AIUpdateDissapear( const SAIDissapearObjUpdate *pUpdate, interface ISoundScene *pSoundScene, 
	IClientAckManager *pAckManager )
{
	CPtr<IMOContainer> pTransport = units.empty() ? 0 : checked_cast_ptr<CMOUnitInfantry*>( units.front() )->GetTransport();
	
	vector< CPtr<IMOUnit> > toDissapear;
	toDissapear.resize( units.size() );
	copy( units.begin(), units.end(), toDissapear.begin() );
	for ( vector< CPtr<IMOUnit> >::const_iterator it = toDissapear.begin(); it != toDissapear.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		pUnit->AIUpdateDissapear( pUpdate, pSoundScene, pAckManager );
	}

	if ( IsValid( pTransport ) )
		pTransport->LoadSquad( this, false );
}

void CMOSquad::AIUpdateServedArtillery( IMOUnit *pMOUnit )
{
	UpdateServedGunCrew( true );
	pServedGun = pMOUnit;
	UpdateServedGunCrew( false );

	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		pUnit->UpdateIcons();
	}
}

bool CMOSquad::AIUpdateAction( bool bEnable, EActionCommand eAction )
{
	NI_ASSERT( eAction == ACTION_COMMAND_CATCH_ARTILLERY, "Unknown update" );
	
//	bCanCatchArtillery = bEnable;

	return true;
}

void CMOSquad::GetAbilityInfo( CAbilityInfo &squadAbilities ) const
{
	squadAbilities.clear();
	
	//Collect ability info
	for( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		if ( !pUnit->IsAlive() )
			continue;

		CAbilityInfo unitAbilities;
		pUnit->GetAbilityInfo( unitAbilities );
		CombineAbilities( &squadAbilities, unitAbilities );
	}
}

int CMOSquad::GetAbilityTier( NDb::EUserAction eAction ) const
{
	for( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		if ( !pUnit->IsAlive() )
			continue;
			
		int nTier = pUnit->GetAbilityTier( eAction );
		if ( nTier != -1 )
			return nTier;
	}
	return -1;
}

bool CMOSquad::Load( interface IMOUnit *pUnit, const bool bEnter )
{
	if ( bEnter )
	{
		if ( !IsInSquad( pUnit ) ) 
		{
			units.push_back( pUnit );
			pUnit->SetSquad( this );
		}
	}
	else
	{
		for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
		{
			if ( *it == pUnit ) 
			{
				units.remove( pUnit );
				pUnit->SetSquad( 0 );
				break;
			}
		}
	}
	UpdateServedGunCrew( false );
	UpdateSquadIcons();
	return true;
}

float CMOSquad::GetVisualHPFraction() const
{
	float fFraction = 0.0f;
	if ( const NDb::SSquadRPGStats *pSquadStats = checked_cast<const NDb::SSquadRPGStats*>( GetStats() ) )
	{
		if ( !pSquadStats->members.empty() )
		{
			fFraction = (float)( GetPassangersCount() ) / (float)( pSquadStats->members.size() );
		}
	}
	return fFraction;
}

float CMOSquad::GetVisualHPFractionSmooth() const
{
	float fFraction = 0.0f;
	if ( const NDb::SSquadRPGStats *pSquadStats = checked_cast<const NDb::SSquadRPGStats*>( GetStats() ) )
	{
		if ( !pSquadStats->members.empty() )
		{
			for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
			{
				const IMOUnit *pUnit = *it;
				fFraction += pUnit->GetHP();
			}
			fFraction = fFraction / (float)( pSquadStats->members.size() );
		}
	}
	return fFraction;
}

bool CMOSquad::IsFirstUnit( const IMOUnit *_pUnit ) const
{
	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		bool bFit = pUnit->IsAlive() && pUnit->IsVisible();
		if ( pUnit == _pUnit )
			return bFit;
		if ( bFit )
			return false;
	}
	return false;
}

void CMOSquad::FillIconsInfoForFirstUnit( SSceneObjIconInfo &iconInfo )
{
	iconInfo.nHPBarBaseLength = GetIconsHPBarLen();

	if ( iconInfo.bIsMainHitbar )
	{
		iconInfo.fHPBarValue = GetVisualHPFractionSmooth();
	}
}

void CMOSquad::UpdateSquadIcons()
{
	for ( CUnitsList::const_iterator it = units.begin(); it != units.end(); ++it )
	{
		IMOUnit *pUnit = *it;
		pUnit->UpdateIcons();
	}
	if ( pContainer )
		pContainer->UpdateIcons();
}

void CMOSquad::InitByDB()
{
}

void CMOSquad::UpdateServedGunCrew( const bool bClearCrew )
{
	CDynamicCast<CMOUnitMechanical> pArtillery = pServedGun;
	if ( !IsValid( pArtillery ) )
		return;
	if ( bClearCrew || units.empty() || !IsValid( *units.begin() ) )
		pArtillery->SetCrewSoldier( 0 );
	else
		pArtillery->SetCrewSoldier( *units.begin() );
}

REGISTER_SAVELOAD_CLASS( 0x100A7485, CMOSquad );
