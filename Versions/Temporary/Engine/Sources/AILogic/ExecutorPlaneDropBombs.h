#pragma once


#include "ExecutorUnitBase.h"
class CAviation;
class CAIUnit;

class CExecutorPlaneDropBombsObject : public CExecutorUnitBase
{
	OBJECT_BASIC_METHODS(CExecutorPlaneDropBombsObject)
	CPtr<CAIUnit> pUnit;
	bool IsExecutorValidInternal() const;
protected:
	float OnAbilityActive();
	float OnAbilityOff();
	void SwitchingOffEnd();
	void SwitchingOffStart();
	void SwitchOnEnd();
	void SwitchOnStart( const class CAICommand *pCommand );
	bool ActivateDuringDisable() { return false; }
	CCommonUnit *GetUnit();
public:
	CExecutorPlaneDropBombsObject( CAIUnit *_pUnit );
	CExecutorPlaneDropBombsObject() {  }
	int Segment();
	void RegisterOnEvents( IExecutorContainer *pContainer );
	bool IsExecutorValid() const { return IsExecutorValidInternal(); }
	bool NotifyEvent( const CExecutorEvent &event );
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CExecutorUnitBase*>( this ) );
		saver.Add( 2, &pUnit );
		return 0;
	}
};

