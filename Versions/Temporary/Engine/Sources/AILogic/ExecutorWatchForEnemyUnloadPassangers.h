#pragma once
#include "Executor.h"

class CMilitaryCar;

class CExecutorWatchForEnemyUnloadPassangers : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorWatchForEnemyUnloadPassangers )
	ZDATA_( CExecutor )
	CPtr<CMilitaryCar> pUnit;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &pUnit );
		return 0;
	}

	bool IsExecutorValidInternal() const;

public:
	CExecutorWatchForEnemyUnloadPassangers( CMilitaryCar *_pUnit )
		: CExecutor( TID_TRANSPORT_LOOK_FOR_ENEMY_UNLOAD_INFANTRY, 20 )
		, pUnit( _pUnit )
	{
	}

	CExecutorWatchForEnemyUnloadPassangers() {}

	int Segment() override;

	bool NotifyEvent( const CExecutorEvent &event ) override
	{
		return false;
	}

	bool IsExecutorValid() const override
	{
		return IsExecutorValidInternal();
	}
};
