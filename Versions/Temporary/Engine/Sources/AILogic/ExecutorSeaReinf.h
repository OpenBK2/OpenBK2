#pragma once
#include "AIUnit.h"
#include "Executor.h"

class CExecutorTransportReinforcement : public CExecutor
{
	OBJECT_BASIC_METHODS( CExecutorTransportReinforcement );

	enum ETransportStates
	{
		ETS_FULL,
		ETS_UNLOADING,
		ETS_EMPTY,
		ETS_LEFT
	};

	struct STransportInfo
	{
		ZDATA
		CPtr<CAIUnit> pUnit;
		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		ETransportStates eState = {};

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pUnit );
			f.Add( 3, &eState );
			return 0;
		}
	};

	typedef std::vector<STransportInfo> CTransportList;

	ZDATA_( CExecutor )
	CTransportList transports;
	CDBPtr<NDb::SReinforcement> pReinf;
	CVec2 vStart;
	CVec2 vUnload;
	CVec2 vTarget;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExecutor *)this );
		f.Add( 2, &transports );
		f.Add( 3, &pReinf );
		f.Add( 4, &vStart );
		f.Add( 5, &vUnload );
		f.Add( 6, &vTarget );
		return 0;
	}

	CExecutorTransportReinforcement() {}

	CExecutorTransportReinforcement( std::list<CAIUnit *> *pTransports,
	                                 const NDb::SReinforcement *_pReinf,
	                                 const CVec2 &_vStart,
	                                 const CVec2 &_vUnload,
	                                 const CVec2 &_vTarget );
	int Segment() override;

	bool NotifyEvent( const CExecutorEvent &event ) override
	{
		return false;
	}

	bool IsExecutorValid() const override
	{
		return true;
	}
};
