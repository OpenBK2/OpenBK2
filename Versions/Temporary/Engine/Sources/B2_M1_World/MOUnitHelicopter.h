#pragma once

#include "MOUnitMechanical.h"

namespace NDb
{
	struct SHelicopterStats;
}

class CHelicopterDeviationProcess;

// Helicopters share normal mechanical-unit presentation and add only their aircraft-specific visuals.
class CMOUnitHelicopter : public CMOUnitMechanical
{
	OBJECT_NOCOPY_METHODS( CMOUnitHelicopter );
	friend class CHelicopterDeviationProcess;

	ZDATA_( CMOUnitMechanical )
		// Hover deviation is client-only and never changes the authoritative AI placement.
		CVec3 vStandingDeviationBasePlacement;
		CVec3 vStandingDeviationOffset;
		CVec3 vStandingDeviationTarget;
		NTimer::STime timeStandingDeviationLastUpdate;
		DWORD dwStandingDeviationRandomState;
		bool bStandingDeviationActive;
	public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CMOUnitMechanical *)this); f.Add(2,&vStandingDeviationBasePlacement); f.Add(3,&vStandingDeviationOffset); f.Add(4,&vStandingDeviationTarget); f.Add(5,&timeStandingDeviationLastUpdate); f.Add(6,&dwStandingDeviationRandomState); f.Add(7,&bStandingDeviationActive); return 0; }

private:
	const NDb::SHelicopterStats *GetHelicopterStats() const;
	float NextStandingDeviationRandom();
	CVec3 GetRandomStandingDeviationPoint( const float fRadius );
	void ChooseStandingDeviationTarget( const float fRadius );
	bool UpdateStandingDeviation( const NTimer::STime &time );

protected:
	virtual void SetupSpecialAnimationMutator( NAnimation::ISkeletonAnimator *pAnimator );
	virtual void AdjustVisualPlacement( struct SAINotifyPlacement *pPlacement );

public:
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason,
		const NDb::EDayNight eDayTime, bool bInEditor );
};
