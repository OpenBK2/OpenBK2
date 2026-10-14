#pragma once

#include "Executor.h"
class CStaticObject;

class CExecutorStaticObjectSegment : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorStaticObjectSegment )
	ZDATA_( CExecutor )
	CPtr<CStaticObject> pObject;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &pObject );
		return 0;
	}

	bool IsExecutorValidInternal() const;

public:
	CExecutorStaticObjectSegment( CStaticObject *_pObj );

	CExecutorStaticObjectSegment() {}

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
