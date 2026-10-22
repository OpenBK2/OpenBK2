#include "stdafx.h"

#include "ChunklessSaver.h"

// UTF16LEToWide and WideToUTF16LE, for the wide strings in the stream
#include "port/unicode.h"

//#define CHECK_CHUNK_ORDER
#define PARANOID_UNITIALIZED_CHECK
extern int N_SAVELOAD_VERSION;

// uses the fact that order of chunks during save and during load is the same
// also requires no countchunks() use
class CChunklessSerializer : public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CChunklessSerializer );
	CPtr<IPointerSerialization> pTracker;
	// Defensive defaults also cover the factory/default construction path.
	CMemoryStream *pRes = nullptr;
	ESaverMode mode{};

	void CheckChunk( const chunk_id idChunk, int nChunkNumber )
	{
#ifdef CHECK_CHUNK_ORDER
		if ( mode == SAVER_MODE_READ )
		{
			chunk_id id;
			int nChunk;
			pRes->Read( &id, 1 );
			pRes->Read( &nChunk, 4 );
			NI_ASSERT( id == idChunk && nChunk == nChunkNumber, "serialization order differs in read/write" );
		}
		else
		{
			if ( !pRes )
			{
				return;
			}
			pRes->Write( &idChunk, 1 );
			pRes->Write( &nChunkNumber, 4 );
		}
#endif
	}

	bool StartChunk( const chunk_id idChunk, int nChunkNumber ) override
	{
		CheckChunk( idChunk, nChunkNumber );
		return true;
	}

	void FinishChunk() override
	{
		CheckChunk( 0, -1 );
	}

	int CountChunks( const chunk_id idChunk ) override
	{
		ASSERT( 0 );
		return 0;
	}

	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) override
	{
		CheckChunk( idChunk, nChunkNumber );
		if ( mode == SAVER_MODE_READ )
		{
			pRes->Read( pData, nSize );
		}
		else
		{
			if ( !pRes )
			{
				return;
			}
#ifdef PARANOID_UNITIALIZED_CHECK
			// check unitialized
			int nCount = 0;
			for ( int k = 0; k < nSize; ++k )
			{
				nCount += ( (const unsigned char *)pData )[k] == 0xcd;
			}
			ASSERT( nCount != nSize );
			nCount = 0;
			for ( int k = 0; k < nSize; ++k )
			{
				nCount += ( (const unsigned char *)pData )[k] == 0xcc;
			}
			ASSERT( nCount != nSize );
#endif
			pRes->Write( pData, nSize );
		}
	}

	void DataChunkString( std::string &data ) override
	{
		int nSize;
		if ( mode == SAVER_MODE_READ )
		{
			pRes->Read( &nSize, 4 );
			data.resize( nSize );
			if ( nSize != 0 )
			{
				pRes->Read( &data[0], nSize );
			}
		}
		else
		{
			if ( !pRes )
			{
				return;
			}
			nSize = data.size();
			pRes->Write( &nSize, 4 );
			if ( !data.empty() )
			{
				pRes->Write( &data[0], nSize );
			}
		}
	}

	// Two bytes per character in the stream, which is UTF-16LE and not wchar_t,
	// the same way CStructureSaver writes it to a file and CNetSaver to the
	// wire. The two are the same type on Windows and the conversion is the copy
	// this used to do; off Windows wchar_t is four bytes, so the old code wrote
	// the first half of the buffer's bytes and read two bytes into each
	// four-byte character.
	//
	// The count stays a count of UTF-16 code units, which is what a Windows
	// wstring's size was, so the stream layout is unchanged on both platforms.
	void DataChunkString( std::wstring &data ) override
	{
		int nSize;
		if ( mode == SAVER_MODE_READ )
		{
			pRes->Read( &nSize, 4 );
			if ( nSize > 0 )
			{
				std::string utf16( size_t( nSize ) * 2, '\0' );
				pRes->Read( &utf16[0], nSize * 2 );
				data = UTF16LEToWide( utf16.data(), utf16.size() );
			}
			else
			{
				data.clear();
			}
		}
		else
		{
			if ( !pRes )
			{
				return;
			}
			const std::string utf16 = WideToUTF16LE( data );
			nSize = int( utf16.size() / 2 );
			pRes->Write( &nSize, 4 );

			if ( nSize != 0 )
			{
				pRes->Write( utf16.data(), nSize * 2 );
			}
		}
	}

	// storing/loading pointers to objects
	void StoreObject( CObjectBase *pObject ) override
	{
#ifndef TRACK_OBJECTS_STATISTICS
		ASSERT( pObject == 0 || IsValid( pObject ) );
#endif
		ASSERT( IsValid( pTracker ) );
		if ( !IsValid( pTracker ) )
		{
			return;
		}
		int nID = pTracker->GetObjectID( pObject );
		if ( pRes )
		{
			pRes->Write( &nID, 4 );
		}
	}

	CObjectBase *LoadObject() override
	{
		ASSERT( IsValid( pTracker ) );
		if ( !IsValid( pTracker ) )
		{
			return 0;
		}
		int nID;
		pRes->Read( &nID, 4 );
		CObjectBase *pRes = pTracker->GetObject( nID );
		return pRes;
	}

public:
	bool IsReading() override
	{
		return mode == SAVER_MODE_READ;
	}

	int GetVersion() const override
	{
		return N_SAVELOAD_VERSION;
	}

	CChunklessSerializer()
		: pRes( 0 )
	{
	}

	CChunklessSerializer( IPointerSerialization *_pTracker, CMemoryStream *_pRes, ESaverMode _mode )
		: pTracker( _pTracker )
		, pRes( _pRes )
		, mode( _mode )
	{
	}
};

IBinSaver *CreateChunklessSaver( IPointerSerialization *pPtr, CMemoryStream *pStream, ESaverMode mode )
{
	return new CChunklessSerializer( pPtr, pStream, mode );
}
