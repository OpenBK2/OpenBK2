#pragma once

#include "StaticObject.h"

#include <cstdint>

//*******************************************************************
//*												 CMineStaticObject								  			*
//*******************************************************************

class CMineStaticObject : public CGivenPassabilityStObject
{
	OBJECT_BASIC_METHODS( CMineStaticObject );

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bIfRegisteredInCWorld = {};
	ZDATA_( CGivenPassabilityStObject )
	CDBPtr<SMineRPGStats> pStats;
	int player = {};

	uint32_t mVisibleStatus = {};
	NTimer::STime nextSegmTime = {};

	bool bIfWillBeDeleted = {}; // кто-то из солдат направляется к этой мине
	ZONSERIALIZE
	ZSKIP //bool bIfRegisteredInCWorld; // мина видима

			bool bAlive = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CGivenPassabilityStObject *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &player );
		f.Add( 4, &mVisibleStatus );
		f.Add( 5, &nextSegmTime );
		f.Add( 6, &bIfWillBeDeleted );
		OnSerialize( f );
		f.Add( 8, &bAlive );
		return 0;
	}

	//
	void Detonate();

	// зависит от клиента
	bool IsRegisteredInWorld() const;

	void OnSerialize( IBinSaver &f )
	{
		if ( !f.IsChecksum() )
		{
			f.Add( 7, &bIfRegisteredInCWorld );
		}
	}

public:
	CMineStaticObject();
	CMineStaticObject(
			const SMineRPGStats *_pStats, const CVec3 &center, const float fHP, const int nFrameIndex, int player );
	void Init() override;

	const SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	void Segment() override;

	const NTimer::STime GetNextSegmentTime() const override
	{
		return nextSegmTime;
	}

	// if explodes under the given unit
	bool WillExplodeUnder( CAIUnit *pUnit );

	// сдетонировать, если при наезде данного юнита мина взрывается; true - если сдетонировала
	bool CheckToDetonate( class CAIUnit *pUnit );
	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override;
	void Die( const float fDamage ) override;

	EStaticObjType GetObjectType() const override
	{
		return ESOT_MINE;
	}

	const bool IsVisible( const uint8_t nParty ) const override;
	void SetVisible( int nParty, bool bVis = true );

	// для удаления инженерами
	bool IsBeingDisarmed() const
	{
		return bIfWillBeDeleted;
	}

	void SetBeingDisarmed( bool bStartDisarm );

	// зависит от клиента
	void RegisterInWorld();

	void ClearVisibleStatus();

	bool IsContainer() const override
	{
		return false;
	}

	const int GetNDefenders() const override
	{
		return 0;
	}

	class CSoldier *GetUnit( const int n ) const override
	{
		return 0;
	}

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override
	{
		return true;
	}

	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;
};
