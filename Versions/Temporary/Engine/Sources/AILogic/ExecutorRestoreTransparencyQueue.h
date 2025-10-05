#pragma once
#include "Executor.h"

// object registed here if it is needed to restore it's transparency
// to avoid lags in game during massive static object deletion
class CExecutorRestoreTransparencyQueue :	public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorRestoreTransparencyQueue)
	
	typedef std::unordered_map<int, bool> CObjectsToRestore;
	typedef std::unordered_map<CVec2, bool, SVec2Hash> CObjectsToAskAround;
	typedef std::list<int> CObjectsQueue;

	ZDATA_(CExecutor)
	CObjectsToAskAround objectsToAskAround;
	CObjectsToRestore objectsToRestore;
	CObjectsQueue objectQueue;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&objectsToAskAround); f.Add(3,&objectsToRestore); f.Add(4,&objectQueue); return 0; }
public:

	CExecutorRestoreTransparencyQueue()
		: CExecutor( TID_RESTORE_TRANSPARENCY, 1 )
	{
	}
	int Segment();
	bool NotifyEvent( const CExecutorEvent &event );
	bool IsExecutorValid() const { return true; }
	void RegisterOnEvents( IExecutorContainer *pContainer );
};

