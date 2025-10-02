#pragma once

#include "AnimUnit.h"

class CAIUnit;

class CAnimUnitMech : public IAnimUnit
{
	OBJECT_BASIC_METHODS( CAnimUnitMech );
	
	struct SMovingState
	{
	public:
		enum EMovingState { EMS_STOPPED, EMS_MOVING, EMS_STOPPED_TO_MOVING, EMS_MOVING_TO_STOPPED };
		ZDATA
		EMovingState state;
		NTimer::STime timeOfIntentionStart;
	public: 
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&state); f.Add(3,&timeOfIntentionStart); return 0; }

		SMovingState() : state( EMS_STOPPED ), timeOfIntentionStart( 0 ) { }
	};
	ZDATA
	CPtr<CAIUnit> pOwner;
	SMovingState movingState;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&movingState); return 0; }
public:
	CAnimUnitMech() : pOwner( 0 ) { }
	virtual void Init( class CAIUnit *pOwner );

	virtual void AnimationSet( int nAnimation );

	virtual void Moved();
	virtual void Stopped();

	virtual void Segment();
	
	virtual void StopCurAnimation() { }
};
