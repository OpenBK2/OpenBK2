#include "StdAfx.h"

#include "LoadImage.h"
#include "Misc/2DArray.h"
#include "Image/Targa.h"

using namespace NImage;

inline void FlipY( CArray2D<BYTE> &image )
{
	const int nSizeX = image.GetSizeX();
	const int nSizeY = image.GetSizeY();
	BYTE *data = &( image[0][0] );
	vector<BYTE> dataline( nSizeX );
	for ( int i = 0; i < nSizeY/2; ++i )
	{
		memcpy( &(dataline[0]), &(data[i*nSizeX]), nSizeX * sizeof(BYTE) );
		memcpy( &(data[i*nSizeX]), &(data[(nSizeY - i - 1)*nSizeX]), nSizeX * sizeof(BYTE) );
		memcpy( &(data[(nSizeY - i - 1)*nSizeX]), &(dataline[0]), nSizeX * sizeof(BYTE) );
	}
}


bool LoadGrayTGAImage( CDataStream *pStream, CArray2D<BYTE> &data )
{
	NI_ASSERT( pStream->IsOk(), "LoadGrayTGAImage - Can't load image" );
	//
	STGAFileHeader hdr;
	pStream->Read( &hdr, sizeof(hdr) );
	NI_ASSERT( hdr.cImageType == TGAIT_BLACK_WHITE, "Can only load grayscale TGA file" );
	NI_ASSERT( hdr.imagespec.cPixelDepth == 8, "Can't read non-8 bit gray image" );
	// skip image ID
	if ( hdr.cIDLength != 0 )
		pStream->Seek( pStream->GetPosition() + hdr.cIDLength );
	//
	if (( data.GetSizeX() != hdr.imagespec.wImageWidth ) ||
			( data.GetSizeY() != hdr.imagespec.wImageHeight ))
		data.SetSizes( hdr.imagespec.wImageWidth, hdr.imagespec.wImageHeight );

	const int nRequested = data.GetSizeX() * data.GetSizeY();
	if ( hdr.imagespec.descriptor.cTopToBottomOrder == 0 ) 
	{
		for ( int i = data.GetSizeY() - 1; i >= 0; --i )
			pStream->Read( &(data[i][0]), data.GetSizeX() * sizeof(BYTE) );
	}
	else
		pStream->Read( &(data[0][0]), nRequested );

	return true;
}


bool AppendGrayTGAImageAtBottom( CDataStream *pStream, CArray2D<BYTE> &data, int nPosY )
{
	NI_ASSERT( pStream->IsOk(), "AppendGrayTGAImageAtBottom - Can't load image" );
	//
	STGAFileHeader hdr;
	pStream->Read( &hdr, sizeof(hdr) );
	NI_ASSERT( hdr.cImageType == TGAIT_BLACK_WHITE, "Can only load grayscale TGA file" );
	NI_ASSERT( hdr.imagespec.cPixelDepth == 8, "Can't read non-8 bit gray image" );
	NI_ASSERT( (hdr.imagespec.wImageWidth == data.GetSizeX()) && ((nPosY + hdr.imagespec.wImageHeight <= data.GetSizeY())), "Incorrect image size or position" );
	// skip image ID
	if ( hdr.cIDLength != 0 )
		pStream->Seek( pStream->GetPosition() + hdr.cIDLength );
	//
	const int nRequested = int( hdr.imagespec.wImageWidth ) * int( hdr.imagespec.wImageHeight );
	if ( hdr.imagespec.descriptor.cTopToBottomOrder == 0 ) 
	{
		for ( int i = nPosY + int(hdr.imagespec.wImageHeight) - 1; nPosY >= 0; --i )
			pStream->Read( &(data[i][0]), hdr.imagespec.wImageWidth * sizeof(BYTE) );
	}
	else
		pStream->Read( &(data[nPosY][0]), nRequested );

	return true;
}


bool SaveGrayTGAImage( CDataStream *pStream, CArray2D<BYTE> &data )
{
	NI_VERIFY( pStream->IsOk(), "SaveGrayTGAImage - Can't save image", return false )
	// compose and write header
	STGAFileHeader hdr;
	Zero( hdr );
	hdr.cImageType = TGAIT_BLACK_WHITE;
	hdr.imagespec.cPixelDepth = 8;
	hdr.imagespec.wImageWidth = data.GetSizeX();
	hdr.imagespec.wImageHeight = data.GetSizeY();
	hdr.imagespec.descriptor.cTopToBottomOrder = 1;
	pStream->Write( &hdr, sizeof(hdr) );
	// write main data
	pStream->Write( &(data[0][0]), data.GetSizeX() * data.GetSizeY() );
	// compose and write TGA file footer
	STGAFileFooter footer;
	Zero( footer );
	memcpy( footer.cSignature, "TRUEVISION-XFILE", 16 );
	footer.cReservedCharacter = '.';
	pStream->Write( &footer, sizeof(footer) );
	//
	return true;
}



