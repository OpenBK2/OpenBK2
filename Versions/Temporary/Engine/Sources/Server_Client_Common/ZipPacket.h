#pragma once

#include "Server_Client_Common/NetPacket.h"

#include <zlib.h>

class CZipPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CZipPacket )
	ZDATA
public:
	// uint32_t and not zlib's uLongf, which is unsigned long: four bytes on
	// Windows and eight on Linux, in a packet both have to agree on. zlib still
	// gets a uLongf of its own in the .cpp, where its out-parameters need one.
	uint32_t nUnzippedSize;
	CMemoryStream buffer;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nUnzippedSize );
		f.Add( 3, &buffer );
		return 0;
	}

	CZipPacket()
		: nUnzippedSize( 0 )
	{
	}

	CZipPacket( const int nClientID )
		: CNetPacket( nClientID )
		, nUnzippedSize( 0 )
	{
	}

	void Zip( const CMemoryStream &inStream );
	void UnZip( CMemoryStream *pOutStream ) const;
};
