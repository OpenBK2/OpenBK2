#pragma once
#include "Executor.h"

class CCommonUnit;

class CExecutorAttackGroup :	public CExecutor
{
	enum EAttackGroupState
	{
		EAGS_BEGIN,
		EAGS_APPROACH_RADIUS,
		EAGS_SWARM,
		EAGS_END,
	};
	typedef list<CPtr<CCommonUnit> > CAttackGroupUnits;
	OBJECT_BASIC_METHODS( CExecutorAttackGroup );
	ZDATA_(CExecutor)
	int nGroupID;
	CAttackGroupUnits units;
	CAttackGroupUnits unitsGoingToRadius;
	CVec2 vPoint;
	float fRadius;
	EAttackGroupState eState;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&nGroupID); f.Add(3,&units); f.Add(4,&vPoint); f.Add(5,&fRadius); f.Add(6,&eState); return 0; }

public:
	CExecutorAttackGroup( int _nGroupID ) : CExecutor( TID_ATTACKGROUP, 20 ), nGroupID( _nGroupID ), eState( EAGS_BEGIN ) {  }
	CExecutorAttackGroup() {  }

	void RegisterOnEvents( IExecutorContainer *pContainer );
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event );
	virtual bool IsExecutorValid() const;
};


