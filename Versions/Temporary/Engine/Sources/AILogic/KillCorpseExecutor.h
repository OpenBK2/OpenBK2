#pragma once

#include "Executor.h"

class CFakeCorpseStaticObject;

class CKillCorpseExecutor : public CExecutor
{
	OBJECT_NOCOPY_METHODS( CKillCorpseExecutor )

	ZDATA_( CExecutor )
		CPtr<CFakeCorpseStaticObject> pObject;
		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		NTimer::STime killTime = {};
	// Keep the original expiry time when resuming a saved simulation.
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CExecutor *)this); f.Add(2,&pObject); f.Add(3,&killTime); return 0; }
public:
	CKillCorpseExecutor() : killTime( 0 ) {}
	CKillCorpseExecutor( CFakeCorpseStaticObject *pObject );

	virtual bool IsExecutorValid() const;

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
};


