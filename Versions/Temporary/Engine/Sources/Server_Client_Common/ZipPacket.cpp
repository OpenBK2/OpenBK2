#include "stdafx.h"

#include "ZipPacket.h"
#include "../zlib/zlib.h"

const int COMPRESSION_LEVEL = ( Z_BEST_SPEED + Z_BEST_COMPRESSION ) / 2;

void CZipPacket::Zip( const CMemoryStream &inStream )
{
	const float fConst = 1.1f;
	uLongf nSize = fConst * inStream.GetSize() + 12;
	buffer.SetSize( nSize );
	nUnzippedSize = inStream.GetSize();
	compress2( buffer.GetBufferForWrite(), &nSize, inStream.GetBuffer(), nUnzippedSize, COMPRESSION_LEVEL );
	buffer.SetSize( nSize );
}

void CZipPacket::UnZip( CMemoryStream *pOutStream ) const
{
	pOutStream->SetSize( nUnzippedSize );
	uLongf nNewSize = nUnzippedSize;
	uncompress( pOutStream->GetBufferForWrite(), &nNewSize, buffer.GetBuffer(), buffer.GetSize() );
	NI_ASSERT( nNewSize == nUnzippedSize, "Size after decompression differs!" )
}



