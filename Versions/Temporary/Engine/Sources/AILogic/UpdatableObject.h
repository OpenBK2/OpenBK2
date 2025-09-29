#pragma once
#include "..\Stats_B2_M1\ActionNotify.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef vector<SVector> CTilesSet;
namespace NDb
{
	struct SHPObjectRPGStats;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EActionNotify;
class CUpdatableObj : public CAIObjectBase
{
	ZDATA
		bool bIsAlive;
		bool bIsAliveSet;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bIsAlive); f.Add(3,&bIsAliveSet); return 0; }
protected:
	void SetAlive( const bool _bIsAlive ) { bIsAlive = _bIsAlive; bIsAliveSet = true; }
public:
	CUpdatableObj() : bIsAlive( true ), bIsAliveSet( false ) {}
	bool IsAlive() const { NI_ASSERT( bIsAliveSet, StrFmt( "IsAlive( %s ) is not set", typeid(this).name() ) ); return bIsAlive; }

	virtual bool IsSelectable()const{ NI_ASSERT( false, "Wrong call of IsSelectable" );return false;};
	virtual int GetMovingType() const { NI_ASSERT( false, "Wrong call of GetMovingType" ); return 0; }

	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )	{ NI_ASSERT( false, "Wrong call of GetPlacement" ); }
	virtual void GetSpeed3( CVec3 *pSpeed ) const { NI_ASSERT( false, "Wrong call of GetSpeed3" ); }
	virtual const NTimer::STime GetTimeOfDeath() const { NI_ASSERT( false, "Wrong call of GetTimeOfDeath" ); return 0; }

	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats ) { NI_ASSERT( false, "Wrong call of GetRPGStats" ); }
	virtual void GetHorTurretTurnInfo( struct SAINotifyTurretTurn *pTurretTurn ) { NI_ASSERT( false, "Wrong call of GetHorTurretTurnInfo" ); }
	virtual void GetVerTurretTurnInfo( struct SAINotifyTurretTurn *pTurretTurn ) { NI_ASSERT( false, "Wrong call of GetVerTurretTurnInfo" ); }
	virtual void GetHitInfo( struct SAINotifyHitInfo *pHitInfo ) const { NI_ASSERT( false, "Wrong call of GetHitInfo" ); }
	virtual void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo ) { NI_ASSERT( false, "Wrong call of GetNewUnitInfo" ); }
	virtual void GetProjectileInfo( struct SAINotifyNewProjectile *pProjectileInfo ) { NI_ASSERT( false, "Wrong call of GetProjectileInfo" ); }
	virtual void GetDyingInfo( struct SAINotifyAction *pDyingInfo, bool *pbVisibleWhenDie ) { NI_ASSERT( false, "Wrong call of GetDyingInfo" ); }
	
	virtual void GetMechShotInfo( struct SAINotifyMechShot *pMechShotInfo, const NTimer::STime &time ) const { NI_ASSERT( false, "Wrong call of GetMechShotInfo" ); }
	virtual void GetInfantryShotInfo( struct SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const { NI_ASSERT( false, "Wrong call of GetInfantryShotInfo" ); }
	virtual void GetEntranceStateInfo( struct SAINotifyEntranceState *pInfo ) const { NI_ASSERT( false, "Wrong call of GetEntranceStateInfo" ); }
	
	virtual void GetRevealCircle( CCircle *pCircle ) const { NI_ASSERT( false, "Wrong call of GetCircle" ); }
	virtual void GetShootAreas( struct SShootAreas *pShootAreas, int *pnAreas ) const { *pnAreas = 0; }
	virtual void GetRangeArea( struct SShootAreas *pRangeArea ) const {  }

	virtual const EActionNotify GetDieAction() const { NI_ASSERT( false, "Wrong call of GetDieAction" ); return ACTION_NOTIFY_NONE; }

	virtual const BYTE GetPlayer() const { NI_ASSERT( false, "Wrong call of GetPlayer" ); return 0xff; }
	virtual const int GetNAIGroup() const { return -2; }
	
	virtual float GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const;

	virtual const int GetUnitState() const { return 0; }

	// обязательно должна быть константной, чтобы не было расхождений в multiplayerb
	virtual const bool IsVisible( const BYTE cParty ) const { NI_ASSERT( false, "wrong call" ); return false; }
	// виден ли игроком
	virtual const bool IsVisibleByPlayer();
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { NI_ASSERT( false, "wrong call" );  }
	virtual const bool IsVisibleForDiplomacyUpdate() { return true; }

	virtual CUpdatableObj* GetDieObject() const { NI_ASSERT( false, "Wrong call of GetDieObject" ); return 0; }

	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { NI_ASSERT( false, "wrong call" ); return false; }
	// 
	virtual const int GetUniqueId() const = 0;
	
	virtual void SetScriptID( const int nScriptID ) { }
	
	virtual const NDb::SHPObjectRPGStats *GetStats() const { return 0; }
	
	virtual void AnimationSet( int nAnimation ) { }
	virtual bool IsFree() const { return true; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
