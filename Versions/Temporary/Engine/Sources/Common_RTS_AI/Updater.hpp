#pragma once

#include "Updater.h"

class CUpdater : public IUpdater
{
	OBJECT_NOCOPY_METHODS( CUpdater );

	enum EUpdateType { EUT_SINGLE, EUT_CONTINUOS };
	ZDATA
		list< CPtr<CObjectBase> > singleUpdates;
		list< CPtr<IContinuosUpdate> > continuosUpdates;
		list< CPtr<IContinuosUpdate> > processedContinuosUpdates;
		list<EUpdateType> updateTypes;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&singleUpdates); f.Add(3,&continuosUpdates); f.Add(4,&processedContinuosUpdates); f.Add(5,&updateTypes); return 0; }

	CObjectBase* GetContinuesUpdate();
public:
	CUpdater() { }

	virtual void GetUpdates( list< CPtr<CObjectBase> > *pUpdates );

	virtual void AddUpdate( CObjectBase *pSingleUpdate, IUpdatableObject *pUpdateOwner );
	virtual void AddUpdate( IContinuosUpdate *pContinuosUpdate );
};


