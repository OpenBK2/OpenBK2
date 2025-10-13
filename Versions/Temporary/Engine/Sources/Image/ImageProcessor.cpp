#include "stdafx.h"

#include "ImageProcessor.h"

#include "ImageBMP.h"
#include "ImageTGA.h"
#include "ImageDDS.h"

#include <cstdint>

namespace NImage
{

// ************************************************************************************************************************ //
// **
// ** image save/load
// **
// **
// **
// ************************************************************************************************************************ //

bool LoadAnyImage( CArray2D<uint32_t> *pRes, CDataStream *pStream )
{
	NI_ASSERT( pStream != 0, "Can't load to NULL stream" );
	//
	if ( NImage::RecognizeFormatDDS(pStream) )
		return NImage::LoadImageDDS( pRes, pStream );
	if ( NImage::RecognizeFormatBMP(pStream) )
		return NImage::LoadImageBMP( pRes, pStream );
	else if ( NImage::RecognizeFormatTGA(pStream) )
		return NImage::LoadImageTGA( pRes, pStream );
	return false;
}

// ************************************************************************************************************************ //
// **
// ** subimage copying
// **
// **
// **
// ************************************************************************************************************************ //

bool Copy( const CArray2D<uint32_t> &src, const CTRect<long> *pSrcRect, CArray2D<uint32_t> &dst, const CTPoint<long> &dstPos )
{
	const CTRect<long> rcRect = (pSrcRect == 0) ? CTRect<long>( 0, 0, src.GetSizeX(), src.GetSizeY() ) : *pSrcRect;
	//
	if ( (dstPos.x + rcRect.Width() > dst.GetSizeX()) || (dstPos.y + rcRect.Height() > dst.GetSizeY()) )
	{
		NI_ASSERT( (dstPos.x + rcRect.Width() > dst.GetSizeX()) || (dstPos.y + rcRect.Height() > dst.GetSizeY()), "Wrong image size" );
		return false;
	}
	//
	for ( int j = 0; j < rcRect.Height(); ++j )
		memcpy( &(dst[dstPos.y + j][dstPos.x]), &(src[rcRect.top + j][rcRect.left]), rcRect.Width() * sizeof(uint32_t) );
	//
	return true;
}

bool CopyAB( const CArray2D<uint32_t> &src, const CTRect<long> *pSrcRect, CArray2D<uint32_t> &dst, const CTPoint<long> &dstPos )
{
	const CTRect<long> rcRect = (pSrcRect == 0) ? CTRect<long>( 0, 0, src.GetSizeX(), src.GetSizeY() ) : *pSrcRect;
	//
	if ( (dstPos.x + rcRect.Width() > dst.GetSizeX()) || (dstPos.y + rcRect.Height() > dst.GetSizeY()) )
	{
		NI_ASSERT( (dstPos.x + rcRect.Width() > dst.GetSizeX()) || (dstPos.y + rcRect.Height() > dst.GetSizeY()), "Wrong image size" );
		return false;
	}
	//
	for ( int j = 0; j < rcRect.Height(); ++j )
	{
		const uint32_t *pSrcColors = &( src[rcRect.top + j][rcRect.left] );
		uint32_t *pDstColors = &( dst[dstPos.y + j][dstPos.x] );
		for ( int i = 0; i < rcRect.Width(); ++i )
		{
			const uint32_t srcA = ( pSrcColors[i] >> 24 ) & 0x000000ff;
			const uint32_t srcR = ( pSrcColors[i] >> 16 ) & 0x000000ff;
			const uint32_t srcG = ( pSrcColors[i] >>  8 ) & 0x000000ff;
			const uint32_t srcB = ( pSrcColors[i]       ) & 0x000000ff;
			const uint32_t dstA = ( pDstColors[i] >> 24 ) & 0x000000ff;
			const uint32_t dstR = ( pDstColors[i] >> 16 ) & 0x000000ff;
			const uint32_t dstG = ( pDstColors[i] >>  8 ) & 0x000000ff;
			const uint32_t dstB = ( pDstColors[i]       ) & 0x000000ff;
			pDstColors[i] = ( (std::max)(srcA, dstA) << 24 ) |
											( ((srcR*srcA + dstR*(255 - srcA)) / 255) << 16 ) |
											( ((srcR*srcA + dstR*(255 - srcA)) / 255) << 16 ) |
											( ((srcG*srcA + dstG*(255 - srcA)) / 255) <<  8 ) |
											( ((srcB*srcA + dstB*(255 - srcA)) / 255) );
		}
	}
	//
	return true;
}

}

