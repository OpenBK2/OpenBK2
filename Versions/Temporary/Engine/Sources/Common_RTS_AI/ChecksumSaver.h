#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include <zconf.h>


struct ICheckSumLog;
class CCheckSumSaver : 	public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CCheckSumSaver );
	typedef std::list<CPtr<CObjectBase> > CAddOrder;
	CAddOrder addOrder;

	typedef std::unordered_map<CPtr<CObjectBase>, bool, SPtrHash> CAddedObjects;
	CAddedObjects addedObjects;
	int nAddedObjects = 0;

	uLong *checkSumData = nullptr;
	uLong *checkSumString = nullptr;
	uLong *checkSumObjects = nullptr;

	bool bLog = false;
	bool bDissalowStoreObjects = false;
	CPtr<ICheckSumLog> pCommandsHistory;
	int nCount = 0;
	NTimer::STime nSegment = 0;

	virtual bool StartChunk( const chunk_id idChunk, int nChunkNumber ) { return true; }
	virtual void FinishChunk() {}

	virtual void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber );
	virtual void DataChunkString( std::string &data );
	virtual void DataChunkString( std::wstring &data );
	// storing/loading pointers to objects
	virtual void StoreObject( CObjectBase *pObject );
	
	virtual CObjectBase* LoadObject() { NI_ASSERT( false,  "wrong call" ); return 0; }

	virtual int CountChunks( const chunk_id idChunk ) { return 0; }
public:
	CCheckSumSaver();
	CCheckSumSaver( uLong *pCheckSum, struct ICheckSumLog * pLog, const NTimer::STime segmentTime );
	~CCheckSumSaver();

	virtual bool IsReading() { return false; }
	virtual int GetVersion() const { return 0; }
	virtual bool IsChecksum() { return true; }
};

