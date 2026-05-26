#include "stdafx.h"

#include "Mine.h"
#include "StaticObjects.h"
#include "Shell.h"
#include "NewUpdater.h"
#include "UnitsIterators2.h"
#include "AIUnit.h"
#include "Cheats.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "FeedBackSystem.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4B0, CMineStaticObject );

extern CFeedBackSystem theFeedBackSystem;
extern CStaticObjects theStatObjs;
extern CEventUpdater updater;
extern CDiplomacy theDipl;
extern NTimer::STime curTime;
extern CShellsStore theShellsStore;
extern SCheats theCheats;

//*******************************************************************
//*												  CMineStaticObject												*
//*******************************************************************

//BASIC_REGISTER_CLASS( CMineStaticObject );

CMineStaticObject::CMineStaticObject()
{
}

CMineStaticObject::CMineStaticObject( const SMineRPGStats *_pStats, const CVec3 &center,  const float fHP, const int nFrameIndex, int _player ) 
: pStats( _pStats ), bIfWillBeDeleted( false ), bAlive( true ), CGivenPassabilityStObject( center, fHP, 0, nFrameIndex ), player( _player )
, bIfRegisteredInCWorld( false )
{ 
	SetAlive( bAlive );
}

void CMineStaticObject::Init()
{
	CGivenPassabilityStObject::Init(); 	
	nextSegmTime = curTime + 500;

	theStatObjs.RegisterSegment( this );
}

void CMineStaticObject::Detonate()
{
	const CVec3 vCenter( GetCenter() );
	theShellsStore.AddShell
	( new CInvisShell( curTime, new CBurstExpl( 0, pStats->pWeapon, 
	CVec3( vCenter.x, vCenter.y, GetHeights()->GetVisZ( vCenter.x, vCenter.y ) ), VNULL3, 0, false, 1, true ), 0 ) );

	Delete();
	bAlive = false;
	SetAlive( bAlive );
}

bool CMineStaticObject::IsRegisteredInWorld() const 
{
	return bIfRegisteredInCWorld;
}

void CMineStaticObject::SetBeingDisarmed( bool bStartDisarm )
{ 
	bIfWillBeDeleted = bStartDisarm; 
}

void CMineStaticObject::RegisterInWorld()
{
	if ( !IsRegisteredInWorld() )
	{
		bIfRegisteredInCWorld = true;
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_ST_OBJ, this, -1 );
	}
}

bool CMineStaticObject::WillExplodeUnder( CAIUnit *pUnit )
{
	return // pUnit->GetStats()->fWeight >= pStats->fWeight && // weight is enough
			( pUnit->GetStats()->etype != RPG_TYPE_ENGINEER );
}

bool CMineStaticObject::CheckToDetonate( CAIUnit *pUnit )
{
	if ( IsValidObj( pUnit ) && !pUnit->GetStats()->IsAviation() )
	{
		bool bMatchTiles = pUnit->GetCenterTile() == AICellsTiles::GetTile( CVec2(GetCenter().x,GetCenter().y) );
		SRect rect( pUnit->GetUnitRect() );
		const bool bGoodUnitToExplode = WillExplodeUnder( pUnit );
		CDBPtr<SMineRPGStats> pMineStats = checked_cast<const SMineRPGStats*>( GetStats() ); 
		if ( bGoodUnitToExplode && ( bMatchTiles || rect.IsPointInside( CVec2(GetCenter().x,GetCenter().y) ) || pMineStats->bRangeDetonator ) )
		{
			// наступили 
			Detonate();
			return true;
		}
	}

	return false;
}

void CMineStaticObject::Segment()
{
	nextSegmTime = curTime + NRandom::Random( 500, 1000 ); RecordRandomCall();

  CDBPtr<SMineRPGStats> pMineStats = checked_cast<const SMineRPGStats*>( GetStats() ); 
	if ( pMineStats->etype != MT_CHARGE )
	{
		CUnitsIter<0,0> it( theDipl.GetNParty( player ), EDI_ENEMY, CVec2(GetCenter().x,GetCenter().y), pMineStats->nTriggerRange * SConsts::TILE_SIZE );
		while ( !it.IsFinished() )
		{
			if ( CheckToDetonate( *it ) )
				return;

			it.Iterate();
		}
	}
}

void CMineStaticObject::Die( const float fDamage )
{
	Detonate();
}

void CMineStaticObject::TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	if ( bFromExplosion && fHP > 0 )
	{
		fHP -= fDamage;
		if ( fHP <= 0 || theCheats.GetFirstShoot( nPlayerOfShoot ) == 1 )
		{
			fHP = 0;
			Detonate();
		}
		else
			updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
	}
}

void CMineStaticObject::ClearVisibleStatus()
{
	mVisibleStatus = 0;
	bIfRegisteredInCWorld = false; // изначально у Юрика нет мины. 
}

void CMineStaticObject::SetVisible( int nParty, bool bVis ) 
{ 
	if ( nParty != theDipl.GetNeutralParty() )
	{
		mVisibleStatus = ( mVisibleStatus & ~( 1UL << nParty ) ) | ( DWORD(bVis) << nParty );

		if ( theDipl.GetDiplStatusForParties( nParty, theDipl.GetMyParty() ) == EDI_FRIEND ) 
		{
			RegisterInWorld();
			if ( theDipl.GetNParty( player ) != nParty )
				theFeedBackSystem.AddFeedbackAndForget( nUniqueID, CVec2( GetCenter().x, GetCenter().y ), EFB_MINE_SIGHTED, -1 );
		}
	}
}

const bool CMineStaticObject::IsVisible( const BYTE nParty ) const
{ 
	return mVisibleStatus & ( 1 << nParty ); 
}

bool CMineStaticObject::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return
		(	eAction == ACTION_NOTIFY_UPDATE_DIPLOMACY ||
		eAction == ACTION_NOTIFY_TREE_BROKEN ||
		eAction == ACTION_NOTIFY_CHANGE_FRAME_INDEX ||
		eAction == ACTION_NOTIFY_DISSAPEAR_OBJ ||
		//eAction == ACTION_NOTIFY_DELETED_ST_OBJ ||
		eAction == ACTION_NOTIFY_SILENT_DEATH );
}


