#include "stdafx.h"

#include "NetPacket.h"
#include "NetSaver.h"

#include "Server_Client_Common_export.h"
#include "System_export.h"

//*******************************************************************
//*                     CUnknownPacket                              *
//*******************************************************************

class CUnknownPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CUnknownPacket );

public:
	CMemoryStream stream;
	int nTypeID;

	ZDATA
	ZEND int operator&( IBinSaver &f ) override
	{
		NI_ASSERT( false, "can't serialize unknown packet" );
		return 0;
	}

	CUnknownPacket() {}
};

REGISTER_SAVELOAD_CLASS( SERVER_CLIENT_COMMON, UNKNOWN_PACKET_TYPE_ID, CUnknownPacket );

//*******************************************************************
//*													NetSaver                                 *
//*******************************************************************

SYSTEM_EXPORT EXTERNVAR int N_SAVELOAD_VERSION;

// uses the fact that order of chunks during save and during load is the same
// also requires no countchunks() use
class CNetSaver : public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CNetSaver );
	CMemoryStream *pRes;
	ESaverMode mode;

	bool StartChunk( const chunk_id idChunk, int nChunkNumber ) override
	{
		return true;
	}

	void FinishChunk() override {}

	int CountChunks( const chunk_id idChunk ) override
	{
		ASSERT( 0 );
		return 0;
	}

	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) override
	{
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
		ASSERT( pObject == 0 || IsValid( pObject ) );
		if ( !pRes )
		{
			return;
		}

		if ( pObject == 0 )
		{
			TPACKET_ID cTypeID = 255;
			pRes->Write( &cTypeID, sizeof( cTypeID ) );
		}
		else
		{
			const int nTypeID = NObjectFactory::GetObjectTypeID( pObject );
			if ( nTypeID == UNKNOWN_PACKET_TYPE_ID )
			{
				CUnknownPacket *pPacket = dynamic_cast<CUnknownPacket *>( pObject );

				TPACKET_ID cTypeID = pPacket->nTypeID;
				pRes->Write( &cTypeID, sizeof( cTypeID ) );
				pRes->WriteFrom( pPacket->stream );
			}
			else
			{
				//				NI_ASSERT( nTypeID >= 0 && nTypeID < 255, StrFmt( "wrong packet type id (%d), range should be [0..254]", nTypeID ) );
				//				if ( nTypeID >= 0 && nTypeID < 255 )
				{
					TPACKET_ID cTypeID = nTypeID;
					pRes->Write( &cTypeID, sizeof( cTypeID ) );
					pObject->operator&( *this );
				}
			}
		}
	}

	CObjectBase *LoadObject() override
	{
		TPACKET_ID cTypeID;
		pRes->Read( &cTypeID, sizeof( cTypeID ) );
		if ( cTypeID == 255 )
		{
			return 0;
		}
		else
		{
			if ( NObjectFactory::IsRegistered( cTypeID ) )
			{
				CObjectBase *pObject = MakeObject<CObjectBase>( cTypeID );
				if ( pObject )
				{
					pObject->operator&( *this );
				}

				return pObject;
			}
			// not registered packet, load as raw stream
			else
			{
				CUnknownPacket *pPacket = new CUnknownPacket();
				pPacket->nTypeID = cTypeID;
				pRes->ReadTo( &( pPacket->stream ), pRes->GetSize() - pRes->GetPosition() );

				return pPacket;
			}
		}
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

	CNetSaver()
		: pRes( 0 )
	{
	}

	CNetSaver( CMemoryStream *_pRes, ESaverMode _mode )
		: pRes( _pRes )
		, mode( _mode )
	{
	}
};

IBinSaver *CreateNetSaver( CMemoryStream *pStream, ESaverMode mode )
{
	return new CNetSaver( pStream, mode );
}
