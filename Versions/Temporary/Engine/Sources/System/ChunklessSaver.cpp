#include "stdafx.h"

#include "ChunklessSaver.h"

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

	void DataChunkString( std::wstring &data ) override
	{
		int nSize;
		if ( mode == SAVER_MODE_READ )
		{
			pRes->Read( &nSize, 4 );
			data.resize( nSize );

			if ( nSize != 0 )
			{
				pRes->Read( &data[0], nSize * 2 );
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
				pRes->Write( &data[0], nSize * 2 );
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
