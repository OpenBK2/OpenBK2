#include "stdafx.h"
#include "BinChunkSaver.h"
#include "Cruncher.h"

#include <cstdint>

SYSTEM_EXPORT int N_SAVELOAD_VERSION = 4;

// remove this for final version
#define TEST_PACK
// to calculate objects size
//#define CALC_SIZE

#ifdef CALC_SIZE
typedef std::unordered_map<std::string, int> TObjSizes;
static TObjSizes objSizes;
static std::string szWhoSaved;
#endif

typedef IBinSaver::chunk_id chunk_id;

// CStructureSaver::CChunkLevel

void CStructureSaver::SChunkLevel::ClearCache() 
{ 
	idLastChunk = (chunk_id)0xff;
	nLastPos = 0;
}

void CStructureSaver::SChunkLevel::Clear() 
{
	idChunk = (chunk_id)0xff; 
	nStart = 0; 
	nLength = 0; 
	ClearCache(); 
}

// chunks operations with whole saves

static bool ReadShortChunkSave( CDataStream &file, chunk_id &dwID, CMemoryStream &chunk )
{
	uint32_t dwLeng = 0;
	if ( file.GetPosition() == file.GetSize() )
		return false;
	file.Read( &dwID, sizeof( dwID ) );
	file.Read( &dwLeng, 1 );
	if ( dwLeng & 1 )
		file.Read( ((char*)&dwLeng)+1, 3 );
	dwLeng >>= 1;
	if ( dwLeng > 500000000 )
	{
		NI_ASSERT( false, "attempt to load too long file" );
		return false;
	}
	chunk.SetSizeDiscard( dwLeng );
	file.Read( chunk.GetBufferForWrite(), dwLeng );
	return file.IsOk();
}

static bool WriteShortChunkSave( CDataStream &file, chunk_id dwID, CMemoryStream &chunk, bool bPack )
{
	if ( bPack )
	{
		CMemoryStream compressed;
		CNetCompressor cmp;
		cmp.Pack( chunk, compressed );
		compressed.Seek(0);
#ifdef TEST_PACK
		{
			CNetCompressor cmptest;
			CMemoryStream test;
			cmptest.Unpack( compressed, test );
			if ( test.GetSize() != chunk.GetSize() || memcmp( test.GetBuffer(), chunk.GetBuffer(), test.GetSize() ) != 0 )
			{
				CFileStream f( "SendThisNival.pls", CFileStream::WIN_CREATE );
				if ( f.IsOk() )
				{
					chunk.Seek(0);
					f.WriteFrom( chunk );
					compressed.Clear();
					CNetCompressor cstore;
					cstore.StorePack( chunk, compressed );
				}
			}
		}
#endif
		return WriteShortChunkSave( file, dwID, compressed, false );
	}
	uint32_t dwLeng;
	file.Write( &dwID, sizeof( dwID ) );
	dwLeng = chunk.GetSize();
	dwLeng <<= 1;
	if ( dwLeng >= 256 )
	{
		dwLeng |= 1;
		file.Write( &dwLeng, sizeof( dwLeng ) );
	}
	else
		file.Write( &dwLeng, 1 );
	file.Write( chunk.GetBuffer(), chunk.GetSize() );
	return true;
}

static bool GetShortChunkSave( CDataStream &file, chunk_id dwID, CMemoryStream &chunk, int nBaseSeek, bool bPacked )
{
	chunk_id dwRid;
	file.Seek( nBaseSeek );
	if ( bPacked )
	{
		CMemoryStream packed;
		while( ReadShortChunkSave( file, dwRid, packed ) )
		{
			if ( dwRid == dwID )
			{
				chunk.SetSizeDiscard( 0 );
				CNetCompressor cmp;
				cmp.Unpack( packed, chunk );
				chunk.Seek(0);
				return true;
			}
		}
	}
	else
	{
		while( ReadShortChunkSave( file, dwRid, chunk ) )
		{
			if ( dwRid == dwID )
				return true;
		}
	}
	chunk.Clear();
	return false;
}

// chunks operations with ChunkLevels

static void ReadPtrData( const unsigned char *pData, void *pDst, int &nPos, int nSize )
{
	memcpy( pDst, pData + nPos, nSize );
	nPos += nSize;
}
// should copy data from start
static void WritePtrData( unsigned char *pDst, const void *pSrc, int *nPos, int nSize )
{
	memcpy( pDst + *nPos, pSrc, nSize );
	*nPos += nSize;
}

bool CStructureSaver::ReadShortChunk( SChunkLevel &src, int &nPos, SChunkLevel &res )
{
	const unsigned char *pSrc = data.GetBuffer() + src.nStart;
	uint32_t dwLeng = 0;
	if ( nPos + 2 > src.nLength )
		return false;
	ReadPtrData( pSrc, &res.idChunk, nPos, sizeof( res.idChunk ) );
	ReadPtrData( pSrc, &dwLeng, nPos, 1 );
	if ( dwLeng & 1 )
		ReadPtrData( pSrc, ((char*)&dwLeng)+1, nPos, 3 );
	dwLeng >>= 1;
	if ( nPos + dwLeng > src.nLength )
		return false;
	res.nStart = nPos + src.nStart;
	res.nLength = dwLeng;
	nPos += dwLeng;
	return true;
}

bool CStructureSaver::WriteShortChunk( SChunkLevel &dst, chunk_id dwID, 
																			const unsigned char *pData, int nLength )
{
#if defined(_DEBUG) && !defined(_FASTDEBUG)
	bool bFound = false; 
	for ( int i = 0; i < nLength; ++i )
	{
		if ( pData[i] != 0xcd && pData[i] != 0xad && pData[i] != 0xfd  )
		{
			bFound = true;
			break;
		}
	}
	NI_ASSERT( nLength == 0 || bFound, "Unitialized data" );
#endif
	uint32_t dwLeng;
	data.SetSize( dst.nStart + dst.nLength + 1 + 4 + nLength );
	unsigned char *pDst = data.GetBufferForWrite() + dst.nStart;
	WritePtrData( pDst, &dwID, &dst.nLength, sizeof( dwID ) );
	dwLeng = nLength;
	dwLeng <<= 1;
	if ( dwLeng >= 256 )
	{
		dwLeng |= 1;
		WritePtrData( pDst, &dwLeng, &dst.nLength, sizeof( dwLeng ) );
	}
	else
		WritePtrData( pDst, &dwLeng, &dst.nLength, 1 );
	// prevent copying to itself
	if ( pDst + dst.nLength != pData )
		WritePtrData( pDst, pData, &dst.nLength, nLength );
	else
		dst.nLength += nLength;
	return true;
}

bool CStructureSaver::GetShortChunk( SChunkLevel &src, chunk_id dwID, SChunkLevel &res, int nNumber )
{
	ASSERT( dwID != 0xff );
	int nPos = src.nLastPos; // search from last found position
	int nCounter = nNumber;
	if ( src.idLastChunk == dwID )
	{
		if ( nNumber == src.nLastNumber + 1 )
			nCounter = 1;
		else
		{
			// not sequential access, fall back to linear search
			src.ClearCache();
			return GetShortChunk( src, dwID, res, nNumber );
		}
	}
	else 
	{
		if ( nNumber != 0 )
		{
			if ( src.nLastPos != 0 )
			{
				src.ClearCache();
				return GetShortChunk( src, dwID, res, nNumber );
			}
		}
		else
			nCounter = 1;
	}
	while ( ReadShortChunk( src, nPos, res ) )
	{
		if ( res.idChunk == dwID )
		{
			if ( nCounter == 1 )
			{
				src.idLastChunk = dwID;
				src.nLastPos = nPos;
				src.nLastNumber = nNumber;
				return true;
			}
			nCounter--;
		}
	}
	if ( src.nLastPos == 0 )
		return false;
	// search from start
	src.ClearCache();
	return GetShortChunk( src, dwID, res, nNumber );
}

int CStructureSaver::CountShortChunks( SChunkLevel &src, chunk_id dwID )
{
	int nPos = 0, nRes = 0;
	SChunkLevel temp;
	while ( ReadShortChunk( src, nPos, temp ) )
	{
		if ( temp.idChunk == dwID )
			nRes++;
	}
	return nRes;
}

// CStructureSaver main methods

void CStructureSaver::DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber )
{
	SChunkLevel &last = chunks.back();
	if ( IsReading() )
	{
		SChunkLevel res;
		if ( GetShortChunk( last, idChunk, res, nChunkNumber ) )
		{
			ASSERT( res.nLength == nSize );
			memcpy( pData, data.GetBuffer() + res.nStart, nSize );
		}
		else
			memset( pData, 0, nSize );
	}
	else
	{
#if defined(_DEBUG) && !defined(FAST_DEBUG)
		ASSERT( CountShortChunks( last, idChunk ) == nChunkNumber - 1 );
#endif
		WriteShortChunk( last, idChunk, (const unsigned char*) pData, nSize );
	}
}

void CStructureSaver::WriteRawData( const void *pData, int nSize )
{
// this can happen to be valid data, so making this assert is not entirely correct
// this can produce false positive
#if defined(_DEBUG) && !defined(FAST_DEBUG)
	NI_ASSERT( ((int*)pData)[0] != 0xcdcdcdcd, "Unitialized data" );
	NI_ASSERT( ((int*)pData)[0] != 0xadadadad, "Unitialized data" );
	NI_ASSERT( ((int*)pData)[0] != 0xfdfdfdfd, "Unitialized data" );
#endif

	SChunkLevel &res = chunks.back();
	data.SetSize( res.nStart + nSize );
	unsigned char *pDst = data.GetBufferForWrite() + res.nStart;
	WritePtrData( pDst, pData, &res.nLength, nSize );
}

void CStructureSaver::RawData( void *pData, int nSize )
{
	if ( IsReading() )
	{
		SChunkLevel &res = chunks.back();
		ASSERT( res.nLength == nSize );
		memcpy( pData, data.GetBuffer() + res.nStart, nSize );
	}
	else
	{
		WriteRawData( pData, nSize );
	}
}

void CStructureSaver::DataChunkString( stdString &str )
{
	if ( IsReading() )
	{
		SChunkLevel &res = chunks.back();
		const char *pStr = (const char*)( data.GetBuffer() + res.nStart );
		str.assign( pStr, res.nLength );
	}
	else
	{
		WriteRawData( str.data(), str.size() );
	}
}

void CStructureSaver::DataChunkString( stdWString &str )
{
	if ( IsReading() )
	{
		SChunkLevel &res = chunks.back();
		const wchar_t *pStr = (wchar_t*) ( data.GetBuffer() + res.nStart );
		str.assign( pStr, res.nLength / 2 );
	}
	else
	{
		WriteRawData( str.data(), str.size() * 2 );
	}
}

void CStructureSaver::StoreObject( CObjectBase *pObject )
{
	if ( pObject )
	{
		NI_ASSERT( NObjectFactory::GetObjectTypeID( pObject ) != -1, StrFmt( "trying to save unregistered object \"%s\"", typeid(*pObject).name() ) );
	}	
	if ( pObject != 0 && storedObjects.find( pObject ) == storedObjects.end() )
	{
		toStore.push_back( pObject );
		storedObjects[pObject] = true; // важно присвоить хоть что-нибудь
	}
	RawData( &pObject, bMode64 ? sizeof(void*) : 4 );

#ifndef _FINALRELEASE
	for ( int i = 0; i < checkers.size(); ++i )
	{
		NI_ASSERT( checkers[i]->CheckObj( pObject ), StrFmt( "object %s checking failed", typeid( *pObject ).name() ) );
	}
#endif

#ifdef CALC_SIZE
	if ( pObject )
	{
		std::string szObjectName = typeid(*pObject).name();
	}
#endif
}

CObjectBase* CStructureSaver::LoadObject()
{
	void *pServerPtr = 0;
	RawData( &pServerPtr, bMode64 ? sizeof(void*) : 4 );
	if ( pServerPtr != 0 )
	{
		CObjectsHash::iterator pFound = objects.find( pServerPtr );
		if ( pFound != objects.end() )
			return pFound->second;
		NI_ASSERT( false, "Here we are in trouble - stored object does not exist" );
		// here we are in problem - stored object does not exist
		// actually i think we got to throw the exception
	}
	return 0;
}

void CStructureSaver::RegisterExternalObject( CObjectBase *pObject, int nID )
{
	if ( IsReading() )
	{
		CExternalHash::iterator i = externalObjects.find( nID );
		if ( i != externalObjects.end() )
			objects[ i->second ] = pObject;
	}
	else
	{
		if ( pObject != 0 )
		{
			storedObjects[pObject] = true; // важно присвоить хоть что-нибудь
			externalObjects[ nID ] = pObject;
		}
	}
}

bool CStructureSaver::StartChunk( const chunk_id idChunk, int nChunkNumber )
{
	SChunkLevel &last = chunks.back();
	chunks.emplace_back();
	if ( IsReading() ) 
	{
		bool bRes = GetShortChunk( last, idChunk, chunks.back(), nChunkNumber );
		if ( !bRes )
			chunks.pop_back();
		return bRes;
	}
	else 
	{
#if defined(_DEBUG) && !defined(FAST_DEBUG)
		ASSERT( CountShortChunks( last, idChunk ) == nChunkNumber - 1 );
#endif
		SChunkLevel &newChunk = chunks.back();
		newChunk.idChunk = idChunk;
		newChunk.nStart = last.nStart + last.nLength + sizeof( chunk_id ) + 4;
		return true;
	}
}

void CStructureSaver::FinishChunk()
{
	if ( IsReading() ) 
	{
		chunks.pop_back();
	}
	else 
	{
		CChunkLevelIterator it = --chunks.end(), it1;
		it1 = it; --it1;
		WriteShortChunk( *it1, it->idChunk, data.GetBuffer() + it->nStart, it->nLength );
		chunks.pop_back();
		AlignDataFileSize();
	}
}

void CStructureSaver::AlignDataFileSize()
{
	SChunkLevel &last = chunks.back();
	data.SetSize( last.nStart + last.nLength );
}

int CStructureSaver::CountChunks( const chunk_id idChunk )
{
	return CountShortChunks( chunks.back(), idChunk );
}

void CStructureSaver::Start( const std::vector<SBinSaverExternalObject> &ext )
{
#ifdef CALC_SIZE
	objSizes.clear();
	szWhoSaved = "static data";
#endif
	
	CDataStream &res = *pRes;
	int nBaseSeek = res.GetPosition();
	//
	chunks.clear();
	obj.Clear();
	data.Clear();
	chunks.emplace_back();
	if ( bIsReading )
	{
		CMemoryStream version, compress;
		GetShortChunkSave( res, 4, version, nBaseSeek, false );
		nVersion = 0;
		if ( version.GetSize() == sizeof(nVersion) )
		{
			version.Seek(0);
			version.Read( &nVersion, sizeof(nVersion) );
		}
		GetShortChunkSave( res, 3, compress, nBaseSeek, false );
		bool bPacked = compress.GetSize() > 0;
		// read chunk with objects description
		GetShortChunkSave( res, 0, obj, nBaseSeek, bPacked );
		GetShortChunkSave( res, 2, data, nBaseSeek, bPacked );
		if ( nVersion > 2 )
		{
			CMemoryStream external;
			GetShortChunkSave( res, 5, external, nBaseSeek, false );
			external.Seek(0);
			while ( external.GetPosition() < external.GetSize() )
			{
				int nID;
				CObjectBase *p;
				external.Read( &nID, sizeof(nID) );
				external.Read( &p, sizeof(p) );
				externalObjects[ nID ] = p;
			}
		}
		for ( int k = 0; k < ext.size(); ++k )
			RegisterExternalObject( ext[k].pObj, ext[k].nID );

		chunks.back().nLength = data.GetSize();
		// create all objects from obj
		while ( obj.GetPosition() < obj.GetSize() )
		{
			int nTypeID = 0;
			void *pServer = 0;
			bool bValid;
			obj.Read( &nTypeID, 4 );
			obj.Read( &pServer, bMode64 ? sizeof(void*) : 4);
			obj.Read( &bValid,1 );
			CObjectBase *pObject = NObjectFactory::MakeObject( nTypeID );
			NI_ASSERT( pObject, StrFmt("Can't create object of type 0x%.8x", nTypeID) );
			if ( !pObject )
			{
				if ( IsDebuggerPresent() )
					__debugbreak();
				continue;
			}
			if ( !bValid )
			{
				// make object invalid
				CPtr<CObjectBase> pTemp( pObject );
				{
					CObj<CObjectBase> pTempObj( (CObjectBase*)pObject );
				}
				pTemp.Extract();
			}
			toStore.push_back( pObject );
			objects[pServer] = pObject;
		}
		// read information about every created object
		int nCount = CountChunks( (chunk_id) 1 );
		for ( int i = 0; i < nCount; i++ )
		{
			void *pServer = 0;
			CObjectBase *pObject;
			const bool bStartChunkResult = StartChunk( (chunk_id) 1, i + 1 );
			ASSERT( bStartChunkResult );
			DataChunk( 0, &pServer, bMode64 ? sizeof(void*) : 4, 1 );
			pObject = objects[pServer];
			ASSERT( pObject );
			if ( pObject )
			{
				if ( StartChunk( 1, 1 ) )
				{
					(*pObject)&( *this );
					// check if there are any unsaved fields
#if defined(_DEBUG) && !defined(FAST_DEBUG)
					{
						static std::vector<const std::type_info*> ignores;
						bool bPerformCheck = true;
						const std::type_info &ti = typeid( *pObject );
						for ( int k = 0; k < ignores.size(); ++k )
						{
							if ( ti == *ignores[k] )
								bPerformCheck = false;
						}
						if ( bPerformCheck )
						{
							bool bFound = false;
							const int nSize = sizeof( *pObject );
							const uint8_t *pObjAsArray = (const uint8_t*)pObject;
							for ( int i = 0; i < nSize; ++i )
							{
								if ( pObjAsArray[i] == 0xcd )
								{
									bFound = true;
									break;
								}
							}
							NI_ASSERT( !bFound, "some fields are not saved" );
							if ( bFound )
								ignores.push_back( &ti );
						}
					}
#endif
					FinishChunk();
				}
			}
			FinishChunk();
		}
		// read main objects data
		chunks.back().Clear();
		GetShortChunkSave( res, 1, data, nBaseSeek, bPacked );
		chunks.back().nLength = data.GetSize();
	}
	else
	{
		nVersion = N_SAVELOAD_VERSION;

		for ( int k = 0; k < ext.size(); ++k )
			RegisterExternalObject( ext[k].pObj, ext[k].nID );
	}
}

void CStructureSaver::Finish()
{
	CDataStream &res = *pRes;
	ASSERT( chunks.size() == 1 );
	if ( !IsReading() )
	{
#ifdef CALC_SIZE
		int nOldSize = res.GetSize();
#endif // CALC_SIZE
		
		// save standard data
		AlignDataFileSize();
		CMemoryStream version;
		version.Write( &nVersion, sizeof(nVersion) );
		WriteShortChunkSave( res, 4, version, false );
		WriteShortChunkSave( res, 1, data, bPackResult );

#ifdef CALC_SIZE
		objSizes[szWhoSaved] = res.GetSize() - nOldSize;
#endif // CALC_SIZE

		// store referenced objects
		data.Clear();
		chunks.back().Clear();
		for ( int nObject = 1; !toStore.empty(); ++nObject )
		{
			CObjectBase *pObject = toStore.front();
			toStore.pop_front();

#ifdef CALC_SIZE
			szWhoSaved = typeid( *pObject ).name();
			int nOldSize = data.GetSize();
			int nOldObjSize = obj.GetSize();
#endif // CALC_SIZE

			// save object type and its server pointer
			int nTypeID = NObjectFactory::GetObjectTypeID( pObject );
			bool bValid = IsValid( pObject );
			ASSERT( nTypeID != -1 );
			if ( nTypeID == -1 )
			{
				CFileStream f( "NotRegistered.txt", CFileStream::WIN_CREATE );
				const char *psz = typeid( *pObject ).name();
				f.Write( psz, strlen(psz) );
				continue;
			}
			obj.Write( &nTypeID, 4 );
			obj.Write( &pObject, bMode64 ? sizeof(void*) : 4 );
			obj.Write( &bValid, 1 );
			// save object data
			const bool bStartChunkResult = StartChunk( (chunk_id) 1, nObject );
			ASSERT( bStartChunkResult );
			DataChunk( 0, &pObject, bMode64 ? sizeof(void*) : 4, 1 );
			//
			if ( StartChunk( 1, 1 ) )
			{
				(*pObject)&( *this );
				FinishChunk();
			}
			FinishChunk();

#ifdef CALC_SIZE
			objSizes[szWhoSaved] += data.GetSize() - nOldSize + obj.GetSize() - nOldObjSize;
#endif // CALC_SIZE
		}
		// save data into resulting file
		WriteShortChunkSave( res, 0, obj, bPackResult );
		AlignDataFileSize();
		WriteShortChunkSave( res, 2, data, bPackResult );
		if ( bPackResult )
		{
			CMemoryStream compressVersion;
			compressVersion.Write( "A3", 3 );
			WriteShortChunkSave( res, 3, compressVersion, false );
		}
		{
			CMemoryStream external;
			for ( CExternalHash::iterator i = externalObjects.begin(); i != externalObjects.end(); ++i )
			{
				int nID = i->first;
				CObjectBase *p = i->second;
				external.Write( &nID, sizeof(nID) );
				external.Write( &p, sizeof(p) );
			}
			WriteShortChunkSave( res, 5, external, false );
		}
		res.Trunc();
	}
	obj.Clear();
	data.Clear();
	objects.clear();
	storedObjects.clear();
	toStore.clear();
	chunks.clear();
}

IBinSaver *CreateBinSaver( CDataStream *pStream, ESaverMode mode, const std::vector<SBinSaverExternalObject> &ext )
{
	return pStream == 0 ? 0 : new CStructureSaver( pStream, mode, ext );
}

IBinSaver *CreateBinSaverWithCheckers( CDataStream *pStream, const std::vector<SBinSaverExternalObject> &external, std::vector< CPtr<IDebugSaveCheckObj> > &checkers )
{
	return pStream == 0 ? 0 : new CStructureSaver( pStream, SAVER_MODE_WRITE, external, checkers );
}

#ifdef CALC_SIZE
static void Size( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	int nSize = 0;
	for ( TObjSizes::iterator iter = objSizes.begin(); iter != objSizes.end(); ++iter )
	{
		nSize += iter->second;
	}

	DbgTrc( "save_size %d", nSize );
}

static void ObjSizes( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	for ( TObjSizes::iterator iter = objSizes.begin(); iter != objSizes.end(); ++iter )
		DbgTrc( "%s\t%d", iter->first.c_str(), iter->second );
}

START_REGISTER(MissionCommands)
REGISTER_CMD( "save_size", Size )
REGISTER_CMD( "obj_sizes", ObjSizes )
FINISH_REGISTER
#endif


