#include "stdafx.h"

#include "Misc/2Darray.h"
#include "Tools_Image.h"

#include <cstdint>

void NImage::Copy( CArray2D<uint32_t> *pDestination, const CArray2D<uint32_t> &rSource, const CTPoint<int> &rStartPoint )
{
	for ( int nXIndex = 0; ( nXIndex < pDestination->GetSizeX() ) && ( nXIndex < ( rSource.GetSizeX() - rStartPoint.x ) ); ++nXIndex )
	{
		for ( int nYIndex = 0; ( nYIndex < pDestination->GetSizeY() ) && ( nYIndex < ( rSource.GetSizeY() - rStartPoint.y ) ); ++nYIndex )
		{
			( *pDestination )[nYIndex][nXIndex] = rSource[nYIndex + rStartPoint.y][nXIndex + rStartPoint.x];
		}
	}
}


