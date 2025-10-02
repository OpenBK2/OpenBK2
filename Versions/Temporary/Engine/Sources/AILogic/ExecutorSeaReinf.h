#pragma once
#include "Executor.h"
#include "AIUnit.h"

class CExecutorTransportReinforcement : public CExecutor
{	
	OBJECT_BASIC_METHODS(CExecutorTransportReinforcement);

	enum ETransportStates { ETS_FULL, ETS_UNLOADING, ETS_EMPTY, ETS_LEFT };

	struct STransportInfo
	{
		ZDATA
		CPtr<CAIUnit> pUnit;
		ETransportStates eState;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&eState); return 0; }
	};
	typedef vector< STransportInfo > CTransportList;

	ZDATA_(CExecutor)
	CTransportList transports;
	CDBPtr<NDb::SReinforcement> pReinf;
	CVec2 vStart;
	CVec2 vUnload;
	CVec2 vTarget;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExecutor*)this); f.Add(2,&transports); f.Add(3,&pReinf); f.Add(4,&vStart); f.Add(5,&vUnload); f.Add(6,&vTarget); return 0; }

	CExecutorTransportReinforcement() {}
	CExecutorTransportReinforcement( list<CAIUnit*> *pTransports, const NDb::SReinforcement *_pReinf, const CVec2 &_vStart, const CVec2 &_vUnload, const CVec2 &_vTarget );
	virtual int Segment();
	virtual bool NotifyEvent( const CExecutorEvent &event ) { return false; }
	virtual bool IsExecutorValid() const { return true; }
};

