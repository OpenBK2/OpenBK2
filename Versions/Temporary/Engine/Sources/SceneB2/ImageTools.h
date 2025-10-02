#pragma once

#include "../zlib/zconf.h"

namespace NImageTools
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SImageZipHeader
	{
		char signature[4];
		int nSizeX;
		int nSizeY;
		int nElemSize;
		int nPackLength;
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class T>
	static void SaveImageAsZip( CDataStream *pStream, const CArray2D<T> &image )
	{
		SImageZipHeader hdr;
		hdr.signature[0] = 'B'; hdr.signature[1] = '2'; hdr.signature[2] = 'F'; hdr.signature[3] = '0';
		hdr.nSizeX = image.GetSizeX();
		hdr.nSizeY = image.GetSizeY();
		hdr.nElemSize = sizeof(T);

		const int nLen = hdr.nSizeX * hdr.nSizeY * hdr.nElemSize;
		BYTE *pDstBuf = new BYTE [nLen];
		z_stream stream;
		stream.next_in = (Bytef*)&(image[0][0]);
		stream.avail_in = (uInt)nLen;
		stream.next_out = (Bytef*)pDstBuf;
		stream.avail_out = (uInt)nLen;
		stream.zalloc = (alloc_func)0;
		stream.zfree = (free_func)0;

		int err = deflateInit2( &stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_FILTERED );
		if ( err == Z_OK )
		{
			err = deflate( &stream, Z_FINISH );
			deflateEnd( &stream );
			//CRAP{ яюўхьє-Єю шэюуфр яЁш Ёрёяръютъх тючтЁр•рхЄё  "buffer error" тёхёЄю "stream end"...
			if ( (err == Z_STREAM_END) || (err == Z_BUF_ERROR) )
				err = Z_OK;
			// CRAP}
			deflateEnd( &stream );
		}
		hdr.nPackLength = stream.total_out;
		pStream->Write( &hdr, sizeof( SImageZipHeader ) );

		pStream->Write( (void *)pDstBuf, stream.total_out );

		delete [] pDstBuf;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<class T>
	static void LoadImageFromZip( CDataStream *pStream, CArray2D<T> &image )
	{
		SImageZipHeader hdr;
		pStream->Read( &hdr, sizeof( SImageZipHeader ) );

		if ( hdr.nElemSize != sizeof(T) )
		{
			NI_ASSERT( hdr.nElemSize == sizeof(T), "Wrong output image format" );
			return;
		}
		image.SetSizes( hdr.nSizeX, hdr.nSizeY );
		if ( hdr.nPackLength == 0 )
		{
			NI_ASSERT( hdr.nPackLength != 0, "Nothing was packed" );
			return;
		}

		BYTE *pSrcData = new BYTE [hdr.nPackLength];
		pStream->Read( pSrcData, hdr.nPackLength );

		// Setup the inflate stream.
		z_stream stream;
		stream.next_in = (Bytef*)pSrcData;
		stream.avail_in = (uInt)hdr.nPackLength;
		stream.next_out = (Bytef*)&(image[0][0]);
		stream.avail_out = hdr.nSizeX * hdr.nSizeY * hdr.nElemSize;
		stream.zalloc = (alloc_func)0;
		stream.zfree = (free_func)0;

		// Perform inflation. wbits < 0 indicates no zlib header inside the data.
		int err = inflateInit2( &stream, -MAX_WBITS );
		if ( err == Z_OK )
		{
			err = inflate( &stream, Z_FINISH );
			inflateEnd( &stream );
			// CRAP{ яюўхьє-Єю шэюуфр яЁш Ёрёяръютъх тючтЁр•рхЄё  "buffer error" тёхёЄю "stream end"...
			if ( (err == Z_STREAM_END) || (err == Z_BUF_ERROR) )
				err = Z_OK;
			// CRAP}
			inflateEnd( &stream );
		}

		delete [] pSrcData;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};

