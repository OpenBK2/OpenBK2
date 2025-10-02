#include "StdAfx.h"
#include "ImageBMP.h"
#include "ImageInternal.h"


enum EBitmapCompression
{
 BC_RGB       = 0,
 BC_RLE8      = 1,
 BC_RLE4      = 2,
 BC_BITFIELDS = 3
};

inline long GetShort( int nOffset, BYTE *pBuff )
{
	return *reinterpret_cast<short*>( pBuff + nOffset );
}
inline long GetWord( int nOffset, BYTE *pBuff )
{
	return *reinterpret_cast<WORD*>( pBuff + nOffset );
}
inline long GetLong( int nOffset, BYTE *pBuff )
{
	return *reinterpret_cast<long*>( pBuff + nOffset );
}
inline long GetDWord( int nOffset, BYTE *pBuff )
{
	return *reinterpret_cast<DWORD*>( pBuff + nOffset );
}

bool NImage::RecognizeFormatBMP( CDataStream *pStream )
{
	char signature[2];
	if ( pStream->GetPosition() + 2 >= pStream->GetSize() )
		return false;

	pStream->Read( signature, 2 );
	pStream->Seek( pStream->GetPosition() - 2 );
	return (signature[0] == 'B') && (signature[1] == 'M');
}

bool NImage::LoadImageBMP( CArray2D<DWORD> *pRes, CDataStream *pStream )
{
	int i;
	long nFileStart = pStream->GetPosition();
	BYTE header[54];
	pStream->Read( header, 54 );
	// read all necessary data
	DWORD dwFileSize = GetDWord( 2, header );
	DWORD dwOffset = GetDWord( 10, header );
	WORD wNumPlanes = GetWord( 26, header );
	WORD wNumBits = GetWord( 28, header );
	DWORD dwCompression = GetDWord( 30, header );
	DWORD dwNumColors = GetDWord( 46, header );
	DWORD dwWidth = GetDWord( 18, header );
	DWORD dwHeight = GetDWord( 22, header );
	vector<DWORD> image( dwWidth * dwHeight );
	// load image
	if ( wNumPlanes != 1 )
		return false;
	//
	DWORD *pImageData = 0;
	//
	if ( wNumBits == 8 )                  // palettized image with 256 color palette
	{
		// read palette (used colors)
		DWORD palette[256];
		dwNumColors = dwNumColors > 0 ? dwNumColors : 256;
		memset( palette, 0, dwNumColors * sizeof(DWORD) );
		for ( i=0; i<dwNumColors; ++i )
		{
			BYTE color[4];
			pStream->Read( color, 4 );
			// BMP file have reversed (read turned inside out) palette format: BGRA instead of ARGB
			palette[i] = DWORD( 255 << 24 ) | (DWORD(color[2]) << 16) | (DWORD(color[1]) << 8) | DWORD(color[0]);
		}
		// seek to image data
		pStream->Seek( nFileStart + dwOffset );
		// read data
		if ( dwCompression == 0 )           // unpacked values
		{
      int nEndOfLineSkip = (dwFileSize - 54 - dwNumColors*4) / dwHeight - dwWidth;
			vector<BYTE> dataline( dwWidth );
			// image data pointer must point to the last line of the image
			pImageData = &(image[0]) + dwWidth*dwHeight - dwWidth - 1;
			//
      for ( i=0; i<dwHeight; ++i )
      {
				if ( pStream->GetPosition() + dwWidth >= pStream->GetSize() )
					return false;
				pStream->Read( &(dataline[0]), dwWidth );
				pImageData = &(image[0]) + ( dwHeight - i - 1 ) * dwWidth;
				// convert image from palette to ARGB
				for ( vector<BYTE>::const_iterator pos = dataline.begin(); pos != dataline.end(); ++pos )
					*pImageData++ = palette[ *pos ];
				// skip 'left' bytes
				if ( nEndOfLineSkip )
					pStream->Seek( nEndOfLineSkip );
      }
		}
		else if ( dwCompression == 1 )      // RLE packing
		{
			vector<BYTE> imagedata( dwWidth * dwHeight );
			bool bDone = false;
      BYTE *d = &(imagedata[0]);
      DWORD x = 0, y = dwHeight - 1;

      while ( !bDone )
      {
        BYTE rlepair[2];
				pStream->Read( rlepair, 2 );

        if ( *rlepair )                 // escaped
        {
          if ( rlepair[1] == 0 )        // end of line
          {
            x = 0;
            y--;
						d = &(imagedata[0]) + y*dwWidth;
          }
					else if ( rlepair[1] == 1 )   // end of bitmap
            bDone = true;
          else if ( rlepair[1] == 2 )   // goto xy
          {
						pStream->Read( rlepair, 2 );
            x += rlepair[0];
            y -= rlepair[1];
            d = &(imagedata[0]) + y*dwWidth + x;
          }
					else                          // run of data (even padded)
          {
						pStream->Read( d, rlepair[1] );

            if ( rlepair[1] & 1 )
							pStream->Read( rlepair, 1 );
            d += rlepair[1];
            x += rlepair[1];
          }
        }
				else                            // run of colors
        {
          memset( d, rlepair[1], rlepair[0] );
          d += rlepair[0];
          x += rlepair[0];
        }
      }
			// depalettize image
			pImageData = &( image[0] );
			for ( vector<BYTE>::const_iterator pos = imagedata.begin(); pos != imagedata.end(); ++pos )
				*pImageData++ = palette[ *pos ];
		}
	}
	else if ( (wNumBits == 16) || (wNumBits == 32) )  // treat palette as a 3 DWORDs with 'red', 'green' and 'blue' bit masks respectively
	{
		if ( dwCompression != BC_BITFIELDS )
			return false;
		// read palette ('red', 'green' and 'blue' bit masks)
		DWORD palette[3];
		pStream->Read( palette, sizeof(DWORD)*3 );
		SPixelConvertInfo pci( 0, palette[0], palette[1], palette[2] );
		//
    int nEndOfLineSkip = ( dwFileSize - 54 - /* dwNumColors*4 */ 3*4 ) / dwHeight - dwWidth*2;
    WORD x, y;

		pStream->Seek( nFileStart + dwOffset );

    for ( y=0; y<dwHeight; ++y )
    {
			DWORD *pImageData = &( image[0] ) + ( dwHeight - y - 1 ) * dwWidth;
      for ( x=0; x<dwWidth; ++x )
      {
        WORD data;
				pStream->Read( &data, 2 );
				*pImageData++ = pci.DecompColor( data ) | 0xFF000000;
      }
      if ( nEndOfLineSkip )   // each row is 0-padded to a 4-byte boundary
				pStream->Seek( pStream->GetPosition() + nEndOfLineSkip ); 
    }
	}
	else if ( wNumBits == 24 )
	{
    int nEndOfLineSkip = ( dwFileSize - 54 )/dwHeight - dwWidth*3;
    WORD x, y;

		pStream->Seek( nFileStart + dwOffset );

    for ( y=0; y<dwHeight; ++y )
    {
			DWORD *pImageData = &( image[0] ) + ( dwHeight - y - 1 ) * dwWidth;
      for ( x=0; x<dwWidth; ++x )
      {
        BYTE d[3];
				pStream->Read( d, 3 );
				*pImageData++ = 0xFF000000 | (DWORD(d[2]) << 16) | (DWORD(d[1]) << 8) | DWORD( d[0] );
      }

      if ( nEndOfLineSkip )   // each row is 0-padded to a 4-byte boundary
				pStream->Seek( pStream->GetPosition() + nEndOfLineSkip );
    }
	}
	else if ( wNumBits == 1 )
  {
    int nEndOfLineSkip = ( dwFileSize - 54 )/dwHeight - dwWidth/8;
    WORD x, y;

		pStream->Seek( pStream->GetPosition() + nFileStart + dwOffset );

    for ( y=0; y<dwHeight; ++y )
    {
			DWORD *pImageData = &( image[0] ) + ( dwHeight - y - 1 ) * dwWidth;
      for ( x=0; x<dwWidth/8; ++x )
      {
        BYTE d;
				pStream->Read( &d, 1 );
        for ( int i=0; i<8; ++i )
        {
          DWORD dwValue = (d & 0x80) == 0 ? 0 : 0xFF;
				  *pImageData++ = 0xFF000000 | (dwValue << 16) | (dwValue << 8) | dwValue;
          d <<= 1;
        }
      }

      if ( nEndOfLineSkip )   // each row is 0-padded to a 4-byte boundary
				pStream->Seek( pStream->GetPosition() + nEndOfLineSkip );
    }
  }
  else
		return false;

	//return new CImage( dwWidth, dwHeight, image );
	pRes->SetSizes( dwWidth, dwHeight );
	memcpy( &(*pRes)[0][0], &image[0], sizeof(image[0]) * dwWidth * dwHeight );
	return true;
}


