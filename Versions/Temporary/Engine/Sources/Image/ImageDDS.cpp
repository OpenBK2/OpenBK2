#include "stdafx.h"

#include "ImageDDS.h"

#include "ImageInternal.h"
#include "DDS.h"
#include "GUnpackDXT.h"

#include <cstdint>

#pragma comment( linker, "/NODEFAULTLIB:libc.lib" )

namespace NImage
{

bool RecognizeFormatDDS( CDataStream *pStream )
{
	uint32_t dwSignature = 0;;

	if ( pStream->GetPosition() + 4 >= pStream->GetSize() )
		return false;

	pStream->Read( &dwSignature, 4 );
	pStream->Seek( pStream->GetPosition() - 4 );
	return dwSignature == SDDSFileHeader::SIGNATURE;
}

// ************************************************************************************************************************ //
// **
// ** ARGB subformats decoding
// **
// **
// **
// ************************************************************************************************************************ //

static void DecompressARGB( uint32_t *pRes, const SDDSHeader &hdr, const uint8_t *pCompBytes )
{
	SPixelConvertInfo pci( hdr.ddspf.dwABitMask, hdr.ddspf.dwRBitMask, hdr.ddspf.dwGBitMask, hdr.ddspf.dwBBitMask );
	//
	switch ( hdr.ddspf.dwRGBBitCount ) 
	{
		case 16:
			{
				uint16_t *pSrc = (uint16_t*)pCompBytes;
				for ( int i = 0; i < hdr.dwWidth * hdr.dwHeight; ++i )
					*pRes++ = pci.DecompColor( *pSrc++ );
			}
			break;
		case 24:
			for ( int i = 0; i < hdr.dwWidth * hdr.dwHeight; ++i )
				*pRes++ = ( pCompBytes[i*3 + 0] << 16 ) | ( pCompBytes[i*3 + 1] << 8 ) | ( pCompBytes[i*3 + 2] );
			break;
		case 32:
			NI_ASSERT( false, "better read it directly to image" );
	}
}

// ************************************************************************************************************************ //
// **
// ** main load function - load DDS image and unpack it to ARGB8888 format
// **
// **
// **
// ************************************************************************************************************************ //

bool LoadImageDDS( CArray2D<uint32_t> *pRes, CDataStream *pStream )
{
	// skip signature
	pStream->Seek( 4 );
	// read header
	SDDSHeader hdr;
	pStream->Read( &hdr, sizeof(hdr) );
	// check for sub-formats
	// DXT#
	if ( hdr.ddspf.dwFlags & DDS_FOURCC ) 
	{
		int nCompSize = 0, nDxt = 0;
		switch ( hdr.ddspf.dwFourCC ) 
		{
			case MAKEFOURCC('D','X','T','1'):
				nDxt = 1;
				nCompSize = hdr.dwWidth * hdr.dwHeight / 2;	// 4 BPP
				break;
			case MAKEFOURCC('D','X','T','2'):
				nDxt = 2;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','3'):
				nDxt = 3;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','4'):
				nDxt = 4;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
			case MAKEFOURCC('D','X','T','5'):
				nDxt = 5;
				nCompSize = hdr.dwWidth * hdr.dwHeight;	// 8 BPP
				break;
		}
		// decompress
		if ( nCompSize > 0 ) 
		{

			uint8_t *buffer = new uint8_t[nCompSize];
			pStream->Read( buffer, nCompSize );
			UnpackDXT( nDxt, hdr.dwWidth, hdr.dwHeight, buffer, pRes );
			delete []buffer;
		}
		else
			return false;
	}
	else if ( ((hdr.ddspf.dwFlags & DDS_ARGB) == DDS_ARGB) || ((hdr.ddspf.dwFlags & DDS_ARGB) == DDS_RGB) ) 
	{
		pRes->SetSizes( hdr.dwWidth, hdr.dwHeight );
		if ( hdr.ddspf.dwRGBBitCount == 32 )	// directly read to image - this is ARGB8888
			pStream->Read( &(*pRes)[0][0], hdr.dwWidth * hdr.dwHeight * 4 );
		else
		{
			int nCompSize = hdr.dwWidth * hdr.dwHeight * hdr.ddspf.dwRGBBitCount / 8;
			uint8_t *buffer = new uint8_t[nCompSize];
			pStream->Read( buffer, nCompSize );
			DecompressARGB( &(*pRes)[0][0], hdr, buffer );
			delete []buffer;
		}
		return true;
	}
	return false;
}

}
