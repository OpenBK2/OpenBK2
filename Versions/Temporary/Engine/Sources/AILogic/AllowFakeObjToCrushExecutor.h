#pragma once

#include "Executor.h"

class CFakeCorpseStaticObject;
enum EStaticObjType;

class CAllowFakeObjToCrushExecutor : public CExecutor
{
	OBJECT_NOCOPY_METHODS( CAllowFakeObjToCrushExecutor )

	ZDATA_( CExecutor )
		CPtr<CFakeCorpseStaticObject> pObject;
		NTimer::STime changeTypeTime;
		EStaticObjType eNewType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CExecutor *)this); f.Add(2,&pObject); f.Add(3,&changeTypeTime); f.Add(4,&eNewType); return 0; }
public:
	CAllowFakeObjToCrushExecutor();
	CAllowFakeObjToCrushExecutor( CFakeCorpseStaticObject *pObject, const int nTimeDelta, const EStaticObjType eType );

	virtual bool IsExecutorValid() const;

	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
};


