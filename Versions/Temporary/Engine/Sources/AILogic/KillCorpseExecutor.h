#pragma once

#include "Executor.h"

class CFakeCorpseStaticObject;

class CKillCorpseExecutor : public CExecutor
{
	OBJECT_NOCOPY_METHODS( CKillCorpseExecutor )

	ZDATA_( CExecutor )
		CPtr<CFakeCorpseStaticObject> pObject;
		NTimer::STime killTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CExecutor *)this); f.Add(2,&pObject); return 0; }
public:
	CKillCorpseExecutor() : killTime( 0 ) {}
	CKillCorpseExecutor( CFakeCorpseStaticObject *pObject );

	virtual bool IsExecutorValid() const;

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
};

