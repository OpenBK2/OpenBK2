#pragma once

#include "Executor.h"


class CExecutorAttackGroupAddUnitEvent : public CExecutorEvent
{
	int nUnitID;
public:
	CExecutorAttackGroupAddUnitEvent( const SExecutorEventParam &param, int _nUnitID ) : nUnitID( _nUnitID ), CExecutorEvent( param ) {  }
	CExecutorAttackGroupAddUnitEvent() { }
	int GetUnitID() const { return nUnitID; }
};

class CExecutorEventAttackGroupAttackPoint: public CExecutorEvent
{
	ZDATA_( CExecutorEvent )
		int nGroupID;
	CVec2 vPoint;
	float fRange;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CExecutorEvent *)this); f.Add(2,&nGroupID); f.Add(3,&vPoint); f.Add(4,&fRange); return 0; }
public:
	CExecutorEventAttackGroupAttackPoint( int _nGroupID, const CVec2 &_vPoint, float _fRange, const SExecutorEventParam &_param ) 
		: nGroupID( _nGroupID ), CExecutorEvent( _param ), vPoint( _vPoint ), fRange( _fRange ) { }
		int GetID() const { return nGroupID; }
		const CVec2 &GetPoint() const { return vPoint; } 
		float GetRange() const { return fRange; }
};

