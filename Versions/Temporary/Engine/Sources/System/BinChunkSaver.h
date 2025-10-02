#pragma once

#include "..\Misc\HashFuncs.h"

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
	typedef string stdString;
	typedef wstring stdWString;
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
	list<SChunkLevel> chunks;
	typedef list<SChunkLevel>::iterator CChunkLevelIterator;
	bool bIsReading, bPackResult;
	// maps objects addresses during save(first) to addresses during load(second) - during loading
	// or serves as a sign that some object has been already stored - during storing
	typedef hash_map<void*,CPtr<CObjectBase>,SDefaultPtrHash> CObjectsHash;
	CObjectsHash objects;
	typedef hash_map<void*,bool,SDefaultPtrHash> CPObjectsHash;
	CPObjectsHash storedObjects;
	typedef hash_map<int,CObjectBase*> CExternalHash;
	CExternalHash externalObjects;
	list<CObjectBase*> toStore;
	int nVersion;
	vector< CPtr<IDebugSaveCheckObj> > checkers;

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
	void Start( const vector<SBinSaverExternalObject> &ext );
	void Finish();

	bool IsReading() { return bIsReading; }
	int GetVersion() const { return nVersion; }
public:
	CStructureSaver() : pRes( 0 ) {}
	CStructureSaver( CDataStream *_pRes, ESaverMode mode, const vector<SBinSaverExternalObject> &ext ) 
		: pRes( _pRes )
	{
		bIsReading = mode == SAVER_MODE_READ;
		bPackResult = mode == SAVER_MODE_WRITE_COMPRESSED;
		Start( ext ); 
	}

	CStructureSaver( CDataStream *_pRes, ESaverMode mode, const vector<SBinSaverExternalObject> &ext, vector< CPtr<IDebugSaveCheckObj> > &_checkers ) 
		: pRes( _pRes ), checkers( _checkers ), bDestroy( false )
	{
		bIsReading = mode == SAVER_MODE_READ;
		bPackResult = mode == SAVER_MODE_WRITE_COMPRESSED;
		Start( ext ); 
	}

	~CStructureSaver() { Finish(); }
};
