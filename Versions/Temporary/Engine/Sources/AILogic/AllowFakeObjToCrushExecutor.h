#pragma once

#include "Executor.h"

class CFakeCorpseStaticObject;
enum EStaticObjType : int;

class CAllowFakeObjToCrushExecutor : public CExecutor
{
	OBJECT_NOCOPY_METHODS( CAllowFakeObjToCrushExecutor )

	ZDATA_( CExecutor )
	CPtr<CFakeCorpseStaticObject> pObject;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime changeTypeTime = {};
	EStaticObjType eNewType = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &pObject );
		f.Add( 3, &changeTypeTime );
		f.Add( 4, &eNewType );
		return 0;
	}

public:
	CAllowFakeObjToCrushExecutor();
	CAllowFakeObjToCrushExecutor( CFakeCorpseStaticObject *pObject, const int nTimeDelta, const EStaticObjType eType );

	bool IsExecutorValid() const override;

	int Segment() override;

	bool NotifyEvent( const CExecutorEvent &event ) override
	{
		return false;
	}
};
