#ifndef __MINE_H__
#define __MINE_H__

#pragma ONCE

#include "StaticObject.h"

//*******************************************************************
//*												 CMineStaticObject								  			*
//*******************************************************************

class CMineStaticObject : public CGivenPassabilityStObject
{
	OBJECT_BASIC_METHODS( CMineStaticObject );

	bool bIfRegisteredInCWorld;
	ZDATA_(CGivenPassabilityStObject)
	CDBPtr<SMineRPGStats> pStats;
	int	player;

	DWORD mVisibleStatus;
	NTimer::STime nextSegmTime;

	bool bIfWillBeDeleted; // кто-то из солдат направляется к этой мине
	ZONSERIALIZE 
	ZSKIP		//bool bIfRegisteredInCWorld; // мина видима

	bool bAlive;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGivenPassabilityStObject*)this); f.Add(2,&pStats); f.Add(3,&player); f.Add(4,&mVisibleStatus); f.Add(5,&nextSegmTime); f.Add(6,&bIfWillBeDeleted); OnSerialize( f ); f.Add(8,&bAlive); return 0; }
	//
	void Detonate();

	// зависит от клиента
	bool IsRegisteredInWorld() const;
	void OnSerialize( IBinSaver &f )
	{
		if ( !f.IsChecksum() )
			f.Add( 7, &bIfRegisteredInCWorld );
	}
public: 
	CMineStaticObject();
	CMineStaticObject( const SMineRPGStats *_pStats, const CVec3 &center,  const float fHP, const int nFrameIndex, int player );
	virtual void Init();

	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual void Segment();
	virtual const NTimer::STime GetNextSegmentTime() const { return nextSegmTime; }

	// if explodes under the given unit
	bool WillExplodeUnder( CAIUnit *pUnit );

	// сдетонировать, если при наезде данного юнита мина взрывается; true - если сдетонировала
	bool CheckToDetonate( class CAIUnit *pUnit );
	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual void Die( const float fDamage );
	virtual EStaticObjType GetObjectType() const { return ESOT_MINE; }

	virtual const bool IsVisible( const BYTE nParty ) const;
	void SetVisible( int nParty, bool bVis = true );

	// для удаления инженерами
	bool IsBeingDisarmed() const {return bIfWillBeDeleted; }
	void SetBeingDisarmed( bool bStartDisarm );

	// зависит от клиента
	void RegisterInWorld();

	void ClearVisibleStatus();
	
	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }
	
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
};

#endif // __MINE_H__

