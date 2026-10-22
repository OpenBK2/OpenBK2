#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include <zconf.h>

struct ICheckSumLog;

class CCheckSumSaver : public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CCheckSumSaver );
	typedef std::list<CPtr<CObjectBase>> CAddOrder;
	CAddOrder addOrder;

	typedef std::unordered_map<CPtr<CObjectBase>, bool, SPtrHash> CAddedObjects;
	CAddedObjects addedObjects;
	int nAddedObjects = 0;

	uint32_t *checkSumData = nullptr;
	uint32_t *checkSumString = nullptr;
	uint32_t *checkSumObjects = nullptr;

	bool bLog = false;
	bool bDissalowStoreObjects = false;
	CPtr<ICheckSumLog> pCommandsHistory;
	int nCount = 0;
	NTimer::STime nSegment = 0;

	bool StartChunk( const chunk_id idChunk, int nChunkNumber ) override
	{
		return true;
	}

	void FinishChunk() override {}

	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) override;
	void DataChunkString( std::string &data ) override;
	void DataChunkString( std::wstring &data ) override;
	// storing/loading pointers to objects
	void StoreObject( CObjectBase *pObject ) override;

	CObjectBase *LoadObject() override
	{
		NI_ASSERT( false, "wrong call" );
		return 0;
	}

	int CountChunks( const chunk_id idChunk ) override
	{
		return 0;
	}

public:
	CCheckSumSaver();
	CCheckSumSaver( uint32_t *pCheckSum, struct ICheckSumLog *pLog, const NTimer::STime segmentTime );
	~CCheckSumSaver() override;

	bool IsReading() override
	{
		return false;
	}

	int GetVersion() const override
	{
		return 0;
	}

	bool IsChecksum() override
	{
		return true;
	}
};
