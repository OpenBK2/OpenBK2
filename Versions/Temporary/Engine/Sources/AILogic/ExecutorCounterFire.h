#pragma once

#include "Executor.h"
#include "Stats_B2_M1/AbilityActions.h"

class CArtillery;
struct SAISpecialAbilityUpdate;
class CAntiArtillery;

class CExecutorCounterFire : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorCounterFire )

	ZDATA_( CExecutor )
	CPtr<CArtillery> pUnit;

	SAbilitySwitchState state;
	SAbilitySwitchState stateBeforeDisable;
	SSpecialAbilityInfo lastSent;
	CPtr<SAISpecialAbilityUpdate> pUpdate;

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime timeLastHeard = {};
	CVec2 vSearchCenter;  // User-selected position
	CVec2 vLastCirclePos; // Latest information (last circle center)
	float fSearchRadius = {};
	CPtr<CAntiArtillery> pCurrentTarget;

	bool bBonusApplied = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &pUnit );
		f.Add( 3, &state );
		f.Add( 4, &stateBeforeDisable );
		f.Add( 5, &lastSent );
		f.Add( 6, &pUpdate );
		f.Add( 7, &timeLastHeard );
		f.Add( 8, &vSearchCenter );
		f.Add( 9, &vLastCirclePos );
		f.Add( 10, &fSearchRadius );
		f.Add( 11, &pCurrentTarget );
		f.Add( 12, &bBonusApplied );
		return 0;
	}

	void UpdateState();
	void ApplyBonus( const bool bForward );

public:
	CExecutorCounterFire( CArtillery *_pUnit );

	CExecutorCounterFire() {}

	int Segment() override;
	bool NotifyEvent( const CExecutorEvent &event ) override;

	bool IsExecutorValid() const override
	{
		return IsValid( pUnit );
	}

	void RegisterOnEvents( IExecutorContainer *pContainer ) override;
};
