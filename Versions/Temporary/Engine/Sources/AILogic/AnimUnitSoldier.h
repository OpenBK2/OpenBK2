#ifndef __ANIM_UNIT_SOLDIER_H__
#define __ANIM_UNIT_SOLDIER_H__

#pragma ONCE

#include "AnimUnit.h"

class CAIUnit;
class CSoldier;
namespace NDb
{
	struct SInfantryRPGStats;
}

class CAnimUnitSoldier : public IAnimUnit
{
	OBJECT_BASIC_METHODS( CAnimUnitSoldier );
	
	struct SMovingState
	{
	public:
		enum EMovingState { EMS_STOPPED, EMS_MOVING, EMS_STOPPED_TO_MOVING, EMS_MOVING_TO_STOPPED };
		ZDATA
		EMovingState state;
		NTimer::STime timeOfIntentionStart;
		public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&state); f.Add(3,&timeOfIntentionStart); return 0; }
		SMovingState() : state( EMS_STOPPED ), timeOfIntentionStart( 0 ) { }
	};

	ZDATA
	CPtr<CSoldier> pOwner;
	CDBPtr<SInfantryRPGStats> pOwnerStats;
	bool bComplexAttack;

	int nCurAnimation;
	NTimer::STime timeOfFinishAnimation;
	bool bMustFinishCurAnimation;


	SMovingState movingState;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pOwnerStats); f.Add(4,&bComplexAttack); f.Add(5,&nCurAnimation); f.Add(6,&timeOfFinishAnimation); f.Add(7,&bMustFinishCurAnimation); f.Add(8,&movingState); return 0; }
	void OnSerialize( IBinSaver &saver );
public:
	CAnimUnitSoldier() : pOwner( 0 ) { }
	virtual void Init( CAIUnit *pOwner );

	virtual void AnimationSet( int nAnimation );

	virtual void Moved();
	virtual void Stopped();

	virtual void Segment();

	virtual void StopCurAnimation();
};

#endif // __ANIM_UNIT_INTERNAL_H__

