#pragma once

#include "Executor.h"
class CStaticObject;

class CExecutorStaticObjectSegment :	public CExecutor
{
	OBJECT_BASIC_METHODS(CExecutorStaticObjectSegment)
	ZDATA_(CExecutor)
	CPtr<CStaticObject> pObject;  
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&pObject); return 0; }

	bool IsExecutorValidInternal() const;
public:
	CExecutorStaticObjectSegment( CStaticObject *_pObj );
	CExecutorStaticObjectSegment() {  }

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
	virtual bool IsExecutorValid() const { return IsExecutorValidInternal(); }

};
