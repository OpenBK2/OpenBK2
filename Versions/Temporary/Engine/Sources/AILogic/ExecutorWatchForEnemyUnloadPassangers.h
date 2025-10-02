#pragma once
#include "executor.h"

class CMilitaryCar;

class CExecutorWatchForEnemyUnloadPassangers : public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorWatchForEnemyUnloadPassangers)
	ZDATA_(CExecutor)
	CPtr<CMilitaryCar> pUnit;  

public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pUnit); return 0; }
	bool IsExecutorValidInternal() const;
public:
	CExecutorWatchForEnemyUnloadPassangers( CMilitaryCar *_pUnit )
		: CExecutor(TID_TRANSPORT_LOOK_FOR_ENEMY_UNLOAD_INFANTRY, 20), pUnit( _pUnit ) { }
	CExecutorWatchForEnemyUnloadPassangers() {  }
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
	virtual bool IsExecutorValid() const { return IsExecutorValidInternal(); }
};

