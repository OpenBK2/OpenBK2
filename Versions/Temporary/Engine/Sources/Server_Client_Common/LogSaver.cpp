#include "stdafx.h"

#include "LogSaver.h"
#include "Misc/StrProc.h"
#include "NetPacket.h"

#include <cstdint>

#include <fmt/format.h>

static int N_SAVELOAD_VERSION = 2;

// uses the fact that order of chunks during save and during load is the same
// also requires no countchunks() use
class CLogSaver : public IBinSaver
{
	OBJECT_NOCOPY_METHODS( CLogSaver );
	std::string *pszLog;

	bool StartChunk( const chunk_id idChunk, int nChunkNumber ) override
	{
		return true;
	}

	void FinishChunk() override {}

	int CountChunks( const chunk_id idChunk ) override
	{
		return 0;
	}

	void DataChunk( const chunk_id idChunk, void *pData, int nSize, int nChunkNumber ) override
	{
		switch ( nSize )
		{
			case 1:
				*pszLog += fmt::format( " {}", (int)( *( reinterpret_cast<uint8_t *>( pData ) ) ) );
				break;
			case 2:
				*pszLog += fmt::format( " {}", (int)( *( reinterpret_cast<uint16_t *>( pData ) ) ) );
				break;
			case 4:
				*pszLog += fmt::format( " {}", (int)( *( reinterpret_cast<int *>( pData ) ) ) );
				break;
			default:
				*pszLog += " long data";
				break;
		}
	}

	void DataChunkString( std::string &data ) override
	{
		*pszLog += " \"" + data + "\"";
	}

	void DataChunkString( std::wstring &data ) override
	{
		*pszLog += " \"" + NStr::ToMBCS( data ) + "\"";
	}

	// storing/loading pointers to objects
	void StoreObject( CObjectBase *pObject ) override
	{
		if ( pObject )
		{
			std::string szObj = typeid( *pObject ).name();
			const std::string szBegin( szObj.begin(), szObj.begin() + 5 );
			if ( szBegin == "class" )
			{
				szObj.erase( szObj.begin(), szObj.begin() + 7 );
			}

			*pszLog += " ";
			*pszLog += szObj + "(";

			if ( NObjectFactory::GetObjectTypeID( pObject ) != UNKNOWN_PACKET_TYPE_ID )
			{
				pObject->operator&( *this );
			}

			*pszLog += " )";
		}
		else
		{
			*pszLog += " null";
		}
	}

	CObjectBase *LoadObject() override
	{
		return 0;
	}

public:
	bool IsReading() override
	{
		return false;
	}

	int GetVersion() const override
	{
		return N_SAVELOAD_VERSION;
	}

	CLogSaver()
		: pszLog( 0 )
	{
	}

	CLogSaver( std::string *_pszLog )
		: pszLog( _pszLog )
	{
	}
};

IBinSaver *CreateLogSaver( std::string *pszLog )
{
	return new CLogSaver( pszLog );
}
