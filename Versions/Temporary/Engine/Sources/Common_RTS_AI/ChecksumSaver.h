#pragma once
#include "..\zlib\zconf.h"


interface ICheckSumLog;
class CCheckSumSaver : 	public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CCheckSumSaver );
	typedef list<CPtr<CObjectBase> > CAddOrder;
	CAddOrder addOrder;

	typedef hash_map<CPtr<CObjectBase>, bool, SPtrHash> CAddedObjects;
	CAddedObjects addedObjects;
	int nAddedObjects; 

	uLong *checkSumData;
	uLong *checkSumString;
	uLong *checkSumObjects;

	bool bLog;
	bool bDissalowStoreObjects;
	CPtr<ICheckSumLog> pCommandsHistory;
	int nCount;
	NTimer::STime nSegment;

	virtual bool StartChunk( const chunk_id idChunk, int nChunkNumber ) { return true; }
	virtual void FinishChunk() {}

	virtual void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber );
	virtual void DataChunkString( string &data );
	virtual void DataChunkString( wstring &data );
	// storing/loading pointers to objects
	virtual void StoreObject( CObjectBase *pObject );
	
	virtual CObjectBase* LoadObject() { NI_ASSERT( false,  "wrong call" ); return 0; }

	virtual int CountChunks( const chunk_id idChunk ) { return 0; }
public:
	CCheckSumSaver();
	CCheckSumSaver( uLong *pCheckSum, interface ICheckSumLog * pLog, const NTimer::STime segmentTime );
	~CCheckSumSaver();

	virtual bool IsReading() { return false; }
	virtual int GetVersion() const { return 0; }
	virtual bool IsChecksum() { return true; }
};

