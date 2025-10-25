#pragma once

#include "BinSaver.h"
#include "Misc/HashFuncs.h"

// a) chunk structure
// b) ptr/ref storage
// system is able to store ref/ptr only for objectbase ancestors
// final save file structure
// -header section list of object types with pointers
// -object data separated in chunks one chunk per object
// c) can replace CMemoryStream with specialized objects to increase perfomance

// chunk with index 0 is used for system and should not be used in user code

class CStructureSaver : public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CStructureSaver );
	//
public:
	typedef std::string stdString;
	typedef std::wstring stdWString;
private:
	CDataStream *pRes;
	bool bDestroy;

	struct SChunkLevel
	{
		chunk_id idChunk, idLastChunk;
		int nStart, nLength;
		int nLastPos, nLastNumber;
		
		void ClearCache();
		void Clear();
		SChunkLevel() { Clear(); }
	};
	// objects descriptors
	CMemoryStream obj;
	// objects data
	CMemoryStream data;
	std::list<SChunkLevel> chunks;
	typedef std::list<SChunkLevel>::iterator CChunkLevelIterator;
	bool bIsReading, bPackResult, bMode64;
	// maps objects addresses during save(first) to addresses during load(second) - during loading
	// or serves as a sign that some object has been already stored - during storing
	typedef std::unordered_map<void*,CPtr<CObjectBase>> CObjectsHash;
	CObjectsHash objects;
	typedef std::unordered_map<void*,bool> CPObjectsHash;
	CPObjectsHash storedObjects;
	typedef std::unordered_map<int,CObjectBase*> CExternalHash;
	CExternalHash externalObjects;
	std::list<CObjectBase*> toStore;
	int nVersion;
	std::vector< CPtr<IDebugSaveCheckObj> > checkers;

	bool ReadShortChunk( SChunkLevel &src, int &nPos, SChunkLevel &res );
	bool WriteShortChunk( SChunkLevel &dst, chunk_id dwID, const unsigned char *pData, int nLength );
	bool GetShortChunk( SChunkLevel &src, chunk_id dwID, SChunkLevel &res, int nNumber );
	int CountShortChunks( SChunkLevel &src, chunk_id dwID );
	//
	bool StartChunk( const chunk_id idChunk, int nChunkNumber );
	void FinishChunk();
	void AlignDataFileSize();
	int CountChunks( const chunk_id idChunk );
	//
	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber );
	void RawData( void *pData, int nSize );
	void WriteRawData( const void *pData, int nSize );
	void DataChunkString( stdString &data );
	void DataChunkString( stdWString &data );
	// storing/loading pointers to objects
	void StoreObject( CObjectBase *pObject );
	CObjectBase* LoadObject();
	void RegisterExternalObject( CObjectBase *pObject, int nID );
	//
	void Start( const std::vector<SBinSaverExternalObject> &ext );
	void Finish();

	bool IsReading() { return bIsReading; }
	int GetVersion() const { return nVersion; }

	void SetMode(const ESaverMode mode) {
		bIsReading = mode == SAVER_MODE_READ || mode == SAVER_MODE_READ_64;
		bPackResult = mode == SAVER_MODE_WRITE_COMPRESSED || mode == SAVER_MODE_WRITE_COMPRESSED_64;
		bMode64 = mode == SAVER_MODE_READ_64 || mode == SAVER_MODE_WRITE_64 || mode == SAVER_MODE_WRITE_COMPRESSED_64;
	}

public:
	CStructureSaver() : pRes( 0 ) {}
	CStructureSaver( CDataStream *_pRes, ESaverMode mode, const std::vector<SBinSaverExternalObject> &ext )
		: pRes( _pRes )
	{
		SetMode(mode);
		Start( ext ); 
	}

	CStructureSaver( CDataStream *_pRes, ESaverMode mode, const std::vector<SBinSaverExternalObject> &ext, std::vector< CPtr<IDebugSaveCheckObj> > &_checkers )
		: pRes( _pRes ), checkers( _checkers ), bDestroy( false )
	{
		SetMode(mode);
		Start( ext ); 
	}

	~CStructureSaver() { Finish(); }
};

