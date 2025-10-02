#pragma once

#include "../Server_Client_Common/NetPacket.h"

#include <zlib.h>

class CZipPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CZipPacket )
	ZDATA
public:
		uLongf nUnzippedSize;
		CMemoryStream buffer;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nUnzippedSize); f.Add(3,&buffer); return 0; }
	CZipPacket() : nUnzippedSize( 0 ) {}
	CZipPacket( const int nClientID ) : CNetPacket( nClientID ), nUnzippedSize( 0 ) {}

	void Zip( const CMemoryStream &inStream );
	void UnZip( CMemoryStream *pOutStream ) const;
};


