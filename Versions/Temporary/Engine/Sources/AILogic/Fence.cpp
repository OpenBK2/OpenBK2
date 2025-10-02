#include "stdafx.h"

#include "..\system\fastmath.h"
#include "..\system\time.h"
#include "fence.h"
#include "NewUpdater.h"
#include "Diplomacy.h"
#include "TimeCounter.h"
#include "StaticObjectsIters.h"
#include "Cheats.h"

extern CDiplomacy theDipl;
extern CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern SCheats theCheats;
extern NAI::CTimeCounter timeCounter;
int ConvertToNAngle( const WORD _wAngle );

CFence::CFence( const SFenceRPGStats *_pStats, const CVec3 &center, const float fHP, const WORD wDir, const int nDiplomacy, const int nFrameIndex )
: pStats( _pStats ), CCommonStaticObject( center, fHP, wDir, nFrameIndex, ESOT_FENCE ), eLifeType( NDb::SFenceRPGStats::ETOL_SAFE )
{
	eLifeType = pStats->GetDamageTypeByFrameIndex( GetFrameIndex() );
	SetAlive( eLifeType != NDb::SFenceRPGStats::ETOL_DESTROYED && CCommonStaticObject::IsAlive() );
	nCreator = (nDiplomacy == -1 || nDiplomacy == 0xff)? theDipl.GetNeutralPlayer() : nDiplomacy;
	bSuspendAppear = !theDipl.IsNetGame() && theDipl.GetDiplStatus( nCreator, theDipl.GetMyNumber() ) == EDI_ENEMY;
	rightTile = leftTile = AICellsTiles::GetTile( center.x, center.y );
}

bool CFence::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return
		( CCommonStaticObject::ShouldSuspendAction( eAction ) || 
		eAction == ACTION_NOTIFY_DISSAPEAR_OBJ ||
		eAction == ACTION_NOTIFY_NEW_ST_OBJ && bSuspendAppear );
}

void CFence::InitDirectionInfo()
{
	const int nDir = ConvertToNAngle( GetDir() );

	list<SVector> tiles;
	GetCoveredTiles( &tiles );

	// "rigth tile" - tile under object center
	// "left tile" - tle under the other end
	const CVec3 vCenter( GetCenter() );
	rightTile = leftTile = AICellsTiles::GetTile( vCenter.x, vCenter.y );
	
	const CVec2 vOtherEnd = GetVectorByDirection( GetDir() ) * SConsts::TILE_SIZE * 2 + CVec2( vCenter.x, vCenter.y );
	const SVector vOtherEndTile( AICellsTiles::GetTile( vOtherEnd ) );

	bool bFound = false;
	for ( list<SVector>::const_iterator iter = tiles.begin(); iter != tiles.end(); ++iter )
	{
		const SVector &tile = *iter;
		if ( tile == vOtherEndTile ) 
		{
			bFound = true;
			break;
		}
	}
	leftTile = vOtherEndTile;
}

void CFence::AnalyzeConnection( CFence *pFence )
{
	// такого нет
	if ( find( neighFences.begin(), neighFences.end(), pFence ) == neighFences.end() )
	{
		if ( rightTile == pFence->leftTile ||
				rightTile == pFence->rightTile ||
				leftTile == pFence->leftTile ||
				leftTile == pFence->rightTile )
		{
			neighFences.push_back( pFence );
			pFence->AnalyzeConnection( this );
		}
	}
}

void CFence::Init()
{
	timeCounter.Count( 0, true );

	CCommonStaticObject::Init();
	InitDirectionInfo();

	SRect boundRect;
	GetBoundRect( &boundRect );
	const float fR = fabs( boundRect.v1 - boundRect.v3 ) * 2.0f;
	for ( CStObjCircleIter<false> iter( CVec2(GetCenter().x,GetCenter().y), fR ); !iter.IsFinished(); iter.Iterate() )
	{
		CExistingObject *pObj = *iter;
		if ( pObj->GetObjectType() == ESOT_FENCE && pObj != this )
		{
			NI_ASSERT( dynamic_cast<CFence*>( pObj ) != 0, "Wrong fence" );
			AnalyzeConnection( checked_cast<CFence*>( pObj ) );
		}
	}

	timeCounter.Count( 0, false );
}

void CFence::Delete()
{
	bSuspendAppear = true;
	if ( eLifeType != NDb::SFenceRPGStats::ETOL_DESTROYED )
	{
		UnlockTiles();
		RemoveTransparencies();
		eLifeType = NDb::SFenceRPGStats::ETOL_DESTROYED;
		SetAlive( eLifeType != NDb::SFenceRPGStats::ETOL_DESTROYED && CCommonStaticObject::IsAlive() );
		SetFrameIndex( pStats->GetRandomFrameIndexByDamageType( eLifeType ) );
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_FRAME_INDEX, this, GetFrameIndex() );

		SetPassProfile( 0 );
		SetVisProfile( 0 );

		SetTransparencies();
		LockTiles();
	}

	for ( list< CPtr<CFence> >::iterator it = neighFences.begin(); it != neighFences.end(); ++it )
		(*it)->DamagePartially( this );
}

void CFence::Die( const float fDamage )
{
	Delete();
}

void CFence::GetVisibility( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *visibility ) const
{
	const CVec3 vCenter( GetCenter() );
	visibility->Init( pStats->GetVisibility( GetFrameIndex() ), GetDir(), pStats->GetVisOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

void CFence::GetPassability( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *passability ) const
{
	const CVec3 vCenter( GetCenter() );
	passability->Init( pStats->GetPassability( GetFrameIndex() ), GetDir(), pStats->GetOrigin( GetFrameIndex() ), CVec2( vCenter.x, vCenter.y ) );
}

void CFence::GetNewUnitInfo( SNewUnitInfo *pNewUnitInfo )
{
	CCommonStaticObject::GetNewUnitInfo( pNewUnitInfo );
	pNewUnitInfo->fHitPoints = eLifeType == NDb::SFenceRPGStats::ETOL_DESTROYED ? 0.0f : GetHitPoints();
}

int CFence::GetHeight() const
{
	return pStats->fFenceHeight;
}

void CFence::DamagePartially( CFence *pFence )
{
	bSuspendAppear = true;

	//CRAP{ calculate need of change frame index - before leghthy operations
	UnlockTiles();
	RemoveTransparencies();
	//CRAP}

	const bool bRightTileDamage = pFence->rightTile == rightTile || pFence->leftTile == rightTile;
	const bool bLeftTileDamage = pFence->leftTile == leftTile || pFence->rightTile == leftTile;

	if ( bRightTileDamage || bLeftTileDamage )
	{
		if ( eLifeType != NDb::SFenceRPGStats::ETOL_SAFE )
		{
			eLifeType = NDb::SFenceRPGStats::ETOL_DESTROYED;
			SetAlive( false );
			SetPassProfile( 0 );
			SetVisProfile( 0 );
		}
		else if ( bRightTileDamage )
		{
			eLifeType = NDb::SFenceRPGStats::ETOL_LEFT;
			SetAlive( CCommonStaticObject::IsAlive() );
			if ( !pStats->HasLeftDamaged() )
			{
				const CVec3 vCenter( GetCenter() );
				const CVec2 vNewCenter = GetVectorByDirection( GetDir() ) * SConsts::TILE_SIZE * 2;
				RotateFence( CVec3( vNewCenter.x + vCenter.x, vNewCenter.y + vCenter.y, 0) );
			}
		}
		else
		{
			eLifeType = NDb::SFenceRPGStats::ETOL_RIGHT;
			SetAlive( CCommonStaticObject::IsAlive() );
		}
		SetFrameIndex( pStats->GetRandomFrameIndexByDamageType( eLifeType ) );
		updater.AddUpdate( 0, ACTION_NOTIFY_CHANGE_FRAME_INDEX, this, GetFrameIndex() );
	}

	//CRAP{ calculate need of change frame index - before leghthy operations
	SetTransparencies();
	LockTiles();
	//CRAP}
}

bool CFence::CanUnitGoThrough( const EAIClasses &eClass ) const
{
	return ( pStats->nAIPassabilityClass & eClass ) == 0;
}

REGISTER_SAVELOAD_CLASS( 0x1108D4E4, CFence );

