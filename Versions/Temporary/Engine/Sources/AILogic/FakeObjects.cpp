#include "stdafx.h"

#include "objectprofile.h"
#include "System/Time.h"
#include "AIUnit.h"
#include "AllowFakeObjToCrushExecutor.h"
#include "Diplomacy.h"
#include "ExecutorContainer.h"
#include "FakeObjects.h"
#include "KillCorpseExecutor.h"
#include "NewUpdater.h"
#include "StaticObjects.h"

extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern CExecutorContainer theExecutorContainer;
extern CStaticObjects theStatObjs;

//*******************************************************************
//*                     CFakeCorpseStaticObject											*
//*******************************************************************

void CFakeCorpseStaticObject::CreateFakeCorpseStaticObject( CExistingObject *pObj )
{
	std::list<SObjTileInfo> lockTiles;
	pObj->CreateLockedTilesInfo( &lockTiles );

	const SStaticObjectRPGStats *pStats = dynamic_cast<const SStaticObjectRPGStats*>(pObj->GetStats());
	const bool bDestructableCorpse = pStats && pStats->bDestructableCorpse;

	CPtr<CFakeCorpseStaticObject> pFakeStObj =
		new CFakeCorpseStaticObject( pObj->GetCenter(), pObj->GetDir(), 1.0f, -1, lockTiles, bDestructableCorpse, pObj, pObj->GetPassProfile() );

	pFakeStObj->Init();
	pFakeStObj->ChangeType( ESOT_CANT_CRUSH );
	theExecutorContainer.Add( new CAllowFakeObjToCrushExecutor( pFakeStObj, SConsts::TIME_TO_ALLOW_KILL_CRUSHED_OBJ, ESOT_FAKE_CORPSE ) );

	pObj->UnlockTiles();
	theStatObjs.AddStaticObject( pFakeStObj, false );

	pObj->SetTrampled();
	pObj->Delete();
}

void CFakeCorpseStaticObject::CreateFakeCorpseStaticObject( class CAIUnit *pUnit, const std::list<SObjTileInfo> &tiles, const bool bCantCrushForSomeTime )
{
	if ( pUnit->IsLockingTiles() )
	{
		pUnit->UnlockTiles();
		pUnit->FixUnlocking();
	}
	
	CVec3 vPos( pUnit->GetCenter().x, pUnit->GetCenter().y, 0 );

	const SUnitBaseRPGStats *pStats = pUnit->GetStats();
	const SMechUnitRPGStats *pMechUnitStats = dynamic_cast<const SMechUnitRPGStats*>( pStats );
	const bool bDestructableCorpse = pMechUnitStats ? pMechUnitStats->bDestructableCorpse : false;
	
	CPtr<CFakeCorpseStaticObject> pFakeStObj =
		new CFakeCorpseStaticObject( vPos, pUnit->GetDirection(), 1.0f, -1, tiles, bDestructableCorpse, pUnit, new CObjectProfile( pUnit->GetUnitRect(), false ) );
	pFakeStObj->Init();

	if ( bCantCrushForSomeTime )
	{
		pFakeStObj->ChangeType( ESOT_CANT_CRUSH );
		theExecutorContainer.Add( new CAllowFakeObjToCrushExecutor( pFakeStObj, SConsts::TIME_TO_ALLOW_KILL_CRUSHED_OBJ, ESOT_FAKE_CORPSE ) );
	}

	theStatObjs.AddStaticObject( pFakeStObj, false );

	if ( SConsts::TIME_TO_DISAPPEAR != 0 )
		theExecutorContainer.Add( new CKillCorpseExecutor( pFakeStObj ) );
}

CFakeCorpseStaticObject::CFakeCorpseStaticObject( const CVec3 &center, const WORD wDir, const float fHP, const int nFrameIndex, 
																								  const std::list<SObjTileInfo> &tiles, const bool _bDestructByTracks,
																									CUpdatableObj* _pDeadObj, CObjectProfile *_pPassProfile )
: CCommonStaticObject( center, fHP, wDir, nFrameIndex, ESOT_FAKE_CORPSE ), pDeadObj( _pDeadObj ), 
	tilesToLock( tiles ), eType( ESOT_FAKE_CORPSE ), pPassProfile( _pPassProfile ), bDestructByTracks( _bDestructByTracks )
{
	for ( std::list<SObjTileInfo>::iterator it = tilesToLock.begin(); it != tilesToLock.end(); ++it )
		it->lockInfo = bDestructByTracks ? EAIClasses( it->lockInfo & ( ~EAC_TRACK ) ) : EAIClasses( it->lockInfo );
}

const BYTE CFakeCorpseStaticObject::GetPlayer() const
{ 
	return theDipl.GetNeutralPlayer(); 
}

bool CFakeCorpseStaticObject::ShouldSuspendAction( const EActionNotify &eAction ) const
{
	return CCommonStaticObject::ShouldSuspendAction( eAction );
}

void CFakeCorpseStaticObject::LockTiles()
{
	GetTerrain()->AddStaticObjectTiles( tilesToLock );
}

void CFakeCorpseStaticObject::CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles )
{
	*pTiles = tilesToLock;
}

void CFakeCorpseStaticObject::UnlockTiles()
{
	GetTerrain()->RemoveStaticObjectTiles( tilesToLock );
}

void CFakeCorpseStaticObject::GetCoveredTiles( std::list<SVector> *pTiles ) const
{
	for ( std::list<SObjTileInfo>::const_iterator it = tilesToLock.begin(); it != tilesToLock.end(); ++it )
	{
		if ( it->lockInfo )
			pTiles->push_back( it->tile );
	}
}

void CFakeCorpseStaticObject::Delete()
{
	updater.AddUpdate( 0, ACTION_NOTIFY_DISSAPEAR_OBJ, pDeadObj, 1 );
	CCommonStaticObject::Delete();
}

bool CFakeCorpseStaticObject::ProcessAreaDamage( const class CExplosion *pExpl, const int nArmorDir, const float fRadius, const float fSmallRadius )
{
	return false;
}

REGISTER_SAVELOAD_CLASS( 0x110BA300, CFakeCorpseStaticObject );

