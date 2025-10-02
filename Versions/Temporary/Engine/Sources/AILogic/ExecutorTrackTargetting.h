#pragma once
#include "Executor.h"
#include "AIUnit.h"
#include "../Stats_B2_M1/AbilityActions.h"

// CExecutorTrackTargetting
//
// Provides autocast functionality for Track Targetting ability
// Is registered on events: ACTIVATE_AUTOCAST, DEACTIVATE_AUTOCAST
//

struct SAISpecialAbilityUpdate;

class CExecutorTrackTargetting : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorTrackTargetting );

	ZDATA_(CExecutor)
	NTimer::STime									timeLastUpdate;
	SAbilitySwitchState						state;
	SSpecialAbilityInfo						lastSent;
	CPtr<SAISpecialAbilityUpdate> pUpdate;
	CPtr<CAIUnit> pUnit;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&timeLastUpdate); f.Add(3,&state); f.Add(4,&lastSent); f.Add(5,&pUpdate); f.Add(6,&pUnit); return 0; }
private:
	void UpdateProgress( const SAbilitySwitchState _state, const float fParam );

protected:
	void SetAutocast( const bool _bAutocast );
	bool GetAutocast() const { return state.bAutocast; }

public:
	CExecutorTrackTargetting( CAIUnit *_pUnit );
	CExecutorTrackTargetting() { }

	void RegisterOnEvents( IExecutorContainer *pContainer );
	bool NotifyEvent( const CExecutorEvent &event );
	bool IsExecutorValid() const { return IsValid( pUnit ); }
	int Segment();
};
