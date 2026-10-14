#pragma once

#include "ExecutorUnitBase.h"
class CAviation;
class CAIUnit;

class CExecutorPlaneDropBombsObject : public CExecutorUnitBase
{
	OBJECT_BASIC_METHODS( CExecutorPlaneDropBombsObject )
	CPtr<CAIUnit> pUnit;
	bool IsExecutorValidInternal() const;

protected:
	float OnAbilityActive() override;
	float OnAbilityOff() override;
	void SwitchingOffEnd() override;
	void SwitchingOffStart() override;
	void SwitchOnEnd() override;
	void SwitchOnStart( const class CAICommand *pCommand ) override;

	bool ActivateDuringDisable() override
	{
		return false;
	}

	CCommonUnit *GetUnit() override;

public:
	CExecutorPlaneDropBombsObject( CAIUnit *_pUnit );

	CExecutorPlaneDropBombsObject() {}

	int Segment() override;
	void RegisterOnEvents( IExecutorContainer *pContainer ) override;

	bool IsExecutorValid() const override
	{
		return IsExecutorValidInternal();
	}

	bool NotifyEvent( const CExecutorEvent &event ) override;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, static_cast<CExecutorUnitBase *>( this ) );
		saver.Add( 2, &pUnit );
		return 0;
	}
};
