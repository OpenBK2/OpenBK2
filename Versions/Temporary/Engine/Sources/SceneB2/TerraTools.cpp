#include "stdafx.h"

#include "TerraTools.h"



static int GetGrannyTypedefOffset( granny_data_type_definition *pType, const char *name )
{
	int nRet = 0;
	while ( pType && pType->Type != GrannyEndMember )
	{
		if ( strcmp(name, pType->Name) == 0 )
			return nRet;
		nRet += GrannyGetMemberTypeSize( pType );
		++pType;
	}
	return -1;
}

void GetVertices( granny_mesh *pMesh, std::vector<CVec3> *pRes, CVec3 *vMin, CVec3 *vMax )
{
	int nSize = GrannyGetTotalObjectSize( pMesh->PrimaryVertexData->VertexType );
	int nPosOffset = GetGrannyTypedefOffset( pMesh->PrimaryVertexData->VertexType, GrannyVertexPositionName );
	NI_ASSERT( nPosOffset >= 0, "Granny model is invalid" );
	pRes->resize( pMesh->PrimaryVertexData->VertexCount );
	char *pUntypedVertices = (char*)( pMesh->PrimaryVertexData->Vertices );

	if ( pMesh->PrimaryVertexData->VertexCount <= 0 )
	{
		vMin->Set( 0, 0, 0 );
		vMax->Set( 0, 0, 0 );
	}

	vMin->Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
	vMax->Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );

	for ( int k = 0; k < pMesh->PrimaryVertexData->VertexCount; ++k )
	{
		char *pVertex = pUntypedVertices + k * nSize;
		CVec3 &dst = (*pRes)[k];
		memcpy( &dst, pVertex + nPosOffset, 3 * sizeof(float) );
		vMin->Minimize( dst );
		vMax->Maximize( dst );
	}
}

void GetTriangles( granny_mesh *pMesh, std::vector<STriangle> *pRes )
{
	granny_tri_topology *pTopol = pMesh->PrimaryTopology;
	int nTriCount = 0;
	for ( int i = 0; i < pTopol->GroupCount; ++i )
		nTriCount += pTopol->Groups[i].TriCount;
	pRes->resize( nTriCount );
	if ( nTriCount == 0 )
		return;
	int ind = 0;
	for ( int i = 0; i < nTriCount; ++i )
	{
		(*pRes)[i].i1 = pTopol->Indices[ ind ];
		(*pRes)[i].i2 = pTopol->Indices[ ind + 1 ];
		(*pRes)[i].i3 = pTopol->Indices[ ind + 2 ];
		ind += 3;
	}
}

void LoadGrannyModel( const std::string &szFileName, std::vector<CVec3> *pVerts, std::vector<STriangle> *pTrgs, CVec3 *vMin, CVec3 *vMax )
{
	granny_file *pFile = GrannyReadEntireFile( szFileName.c_str() );
	NI_ASSERT( pFile, StrFmt("Can't read model from %s", szFileName) );
	granny_file_info *pInfo = GrannyGetFileInfo( pFile );
	NI_ASSERT( pInfo, StrFmt("Can't read model info from %s", szFileName) );
	if ( pInfo->MeshCount > 0 )
	{
		pVerts->reserve( 512 );
		pTrgs->reserve( 512 );
		vMin->Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
		vMax->Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
		std::vector<CVec3> singleVerts;
		std::vector<STriangle> singleTrgs;
		CVec3 vSingleMin, vSingleMax;
		for ( int nMeshIndex = 0; nMeshIndex < pInfo->MeshCount; ++nMeshIndex )
		{
			granny_mesh *pMesh = pInfo->Meshes[nMeshIndex];
			GetVertices( pMesh, &singleVerts, &vSingleMin, &vSingleMax );
			GetTriangles( pMesh, &singleTrgs );
			const int nVertsOffs = pVerts->size();
			for ( std::vector<CVec3>::const_iterator it = singleVerts.begin(); it != singleVerts.end(); ++it )
			{
				pVerts->push_back( *it );
			}
			for ( std::vector<STriangle>::const_iterator it = singleTrgs.begin(); it != singleTrgs.end(); ++it )
			{
				pTrgs->push_back( STriangle(it->i1 + nVertsOffs, it->i2 + nVertsOffs, it->i3 + nVertsOffs) );
			}
			vMin->Minimize( vSingleMin );
			vMax->Maximize( vSingleMax );
		}
	}
	GrannyFreeFile( pFile );
}

void ScanFill( CArray2D<BYTE> *pMask, const BYTE nColor )
{
	int nHigh, nLow, x;
	bool bFill;
	for ( int y = 0; y < pMask->GetSizeY(); ++y )
	{
		x = 0;
		bFill = false;
		while ( x < pMask->GetSizeX() )
		{
			while ( (*pMask)[y][x] != nColor )
			{
				if ( bFill )
					(*pMask)[y][x] = nColor;
				if ( ++x >= pMask->GetSizeX() )
					break;
			}
			if ( x < pMask->GetSizeX() )
			{
				nHigh = 0;
				nLow = 0;
				if ( x > 0 )
				{
					if ( y > 0 )
						if ( (*pMask)[y-1][x-1] == nColor )
							++nLow;
					if ( y < (pMask->GetSizeY() - 1) )
						if ( (*pMask)[y+1][x-1] == nColor )
							++nHigh;
				}
				while ( (*pMask)[y][x] == nColor )
				{
					if ( y > 0 )
						if ( (*pMask)[y-1][x] == nColor )
							++nLow;
					if ( y < (pMask->GetSizeY() - 1) )
						if ( (*pMask)[y+1][x] == nColor )
							++nHigh;
					if ( ++x >= pMask->GetSizeX() )
						break;
				}
				if ( x < pMask->GetSizeX() )
				{
					if ( y > 0 )
						if ( (*pMask)[y-1][x] == nColor )
							++nLow;
					if ( y < (pMask->GetSizeY() - 1) )
						if ( (*pMask)[y+1][x] == nColor )
							++nHigh;
				}
				if ( (nLow > 0) && (nHigh > 0) )
					bFill = !bFill;
			}
		}
	}
}

void ScanFillInArea( CArray2D<BYTE> *pMask, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, const BYTE nColor )
{
	NI_ASSERT( (nMinX >= 0) && (nMaxX < pMask->GetSizeX()) && (nMinY >= 0) && (nMaxY < pMask->GetSizeY()), "Wrong area coordinates" );
	NI_ASSERT( (nMinX <= nMaxX) && (nMinY <= nMaxY), "Wrong area coordinates" );
	int nHigh, nLow, x;
	bool bFill;
	for ( int y = nMinY; y <= nMaxY; ++y )
	{
		x = nMinX;
		bFill = false;
		while ( x <= nMaxX )
		{
			while ( (*pMask)[y][x] != nColor )
			{
				if ( bFill )
					(*pMask)[y][x] = nColor;
				if ( ++x > nMaxX )
					break;
			}
			if ( x <= nMaxX )
			{
				nHigh = 0;
				nLow = 0;
				if ( x > nMinX )
				{
					if ( y > nMinY )
						if ( (*pMask)[y-1][x-1] == nColor )
							++nLow;
					if ( y < nMaxY )
						if ( (*pMask)[y+1][x-1] == nColor )
							++nHigh;
				}
				while ( (*pMask)[y][x] == nColor )
				{
					if ( y > nMinY )
						if ( (*pMask)[y-1][x] == nColor )
							++nLow;
					if ( y < nMaxY )
						if ( (*pMask)[y+1][x] == nColor )
							++nHigh;
					if ( ++x > nMaxX )
						break;
				}
				if ( x <= nMaxX )
				{
					if ( y > nMinY )
						if ( (*pMask)[y-1][x] == nColor )
							++nLow;
					if ( y < nMaxY )
						if ( (*pMask)[y+1][x] == nColor )
							++nHigh;
				}
				if ( ( nLow > 0 ) && ( nHigh > 0 ) )
					bFill = !bFill;
			}
		}
	}
}

void DrawLine( CArray2D<BYTE> *pArray, const int nX1, const int nY1, const int nX2, const int nY2, const BYTE nColor )
{
	const int nDx = abs( nX2 - nX1 );
	const int nDy = abs( nY2 - nY1 );
	const int nSx = nX2 >= nX1 ? 1 : -1;
	const int nSy = nY2 >= nY1 ? 1 : -1;
	if ( nDy <= nDx )
	{
		int d = ( nDy << 1 ) - nDx;
		const int d1 = nDy << 1;
		const int d2 = ( nDy - nDx ) << 1;
		if ( (nX1 >= 0) && (nX1 < pArray->GetSizeX()) && (nY1 >= 0) && (nY1 < pArray->GetSizeY()) )
			(*pArray)[nY1][nX1] = nColor;
		for ( int x = nX1 + nSx, y = nY1, i = 1; i <= nDx; i++, x += nSx )
		{
			if ( d > 0 )
			{
				d += d2;
				y += nSy;
			}
			else
				d += d1;
			if ( (x >= 0) && (x < pArray->GetSizeX()) && (y >= 0) && (y < pArray->GetSizeY()) )
				(*pArray)[y][x] = nColor;
		}
	}
	else
	{
		int d = ( nDx << 1 ) - nDy;
		const int d1 = nDx << 1;
		const int d2 = ( nDx - nDy ) << 1;
		if ( (nX1 >= 0) && (nX1 < pArray->GetSizeX()) && (nY1 >= 0)	&& (nY1 < pArray->GetSizeY()) )
			(*pArray)[nY1][nX1] = nColor;
		for ( int x = nX1, y = nY1 + nSy, i = 1; i <= nDy; i++, y += nSy )
		{
			if ( d > 0 )
			{
				d += d2;
				x += nSx;
			}
			else
				d += d1;
			if ( (x >= 0) && (x < pArray->GetSizeX()) && (y >= 0) && (y < pArray->GetSizeY()) )
				(*pArray)[y][x] = nColor;
		}
	}
}

static int LineFill( CArray2D<BYTE> *pArray, const int x, const int y, const int nDir, const int nPrevXl, const int nPrevXr, const BYTE nColor )
{
	if ( (x < 0) ||( y < 0) || (x >= pArray->GetSizeX()) || (y >= pArray->GetSizeY()) )
		return x;

	int xl = x;
	int xr = x;
	int c;

	c = nColor + 1;
	while ( (c != nColor) && (xl > 0) )
	{
		c = (*pArray)[y][--xl];
	}
	/*do c = arr[y][--xl];
	while ( ( c != nColor ) && ( xl > 0 ) );*/
	if ( c == nColor )
		xl++;

	c = nColor + 1;
	while ( (c != nColor) && (xr < (pArray->GetSizeX() - 1)) )
	{
		c = (*pArray)[y][++xr];
	}
	/*do c = arr[y][++xr];
	while ( ( c != nColor ) && ( xr < ( arr.GetSizeX() - 1 ) ) );*/
	if ( c == nColor )
		xr--;

	for ( int i = xl; i <= xr; ++i )
		(*pArray)[y][i] = nColor;

	if ( ((y + nDir) >= 0) && ((y + nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= xr; ++i )
		{
			c = (*pArray)[y + nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y + nDir, nDir, xl, xr, nColor );
		}
	}
	if ( ((y - nDir) >= 0) && ((y - nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= nPrevXl; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y - nDir, -nDir, xl, xr, nColor );
		}
		for ( int i = nPrevXr; i <= xr; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y - nDir, -nDir, xl, xr, nColor );
		}
	}
	return xr;
}

static int LineFillEx( CArray2D<BYTE> *pArray, const int x, const int y, const int nDir, const int nPrevXl, const int nPrevXr,
											 const BYTE nColor, const BYTE nBorderColor )
{
	if ( (x < 0) || (y < 0) || (x >= pArray->GetSizeX()) || (y >= pArray->GetSizeY()) )
		return x;

	int xl = x;
	int xr = x;
	int c;

	c = nColor + 1;
	while ( (c != nColor) && (c != nBorderColor) && (xl > 0) )
	{
		c = (*pArray)[y][--xl];
	}
	if ( xl > 0 ) xl++;

	c = nColor + 1;
	while ( (c != nColor) && (c != nBorderColor) && (xr < (pArray->GetSizeX() - 1)) )
	{
		c = (*pArray)[y][++xr];
	}
	if ( xr < (pArray->GetSizeX() - 1) )
		xr--;

	for ( int i = xl; i <= xr; ++i )
		(*pArray)[y][i] = nColor;

	if ( ((y + nDir) >= 0) && ((y + nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= xr; ++i )
		{
			c = (*pArray)[y + nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = LineFillEx( pArray, i, y + nDir, nDir, xl, xr, nColor, nBorderColor );
		}
	}
	if ( ((y - nDir) >= 0) && ((y - nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= nPrevXl; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = LineFillEx( pArray, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor );
		}
		for ( int i = nPrevXr; i <= xr; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = LineFillEx( pArray, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor );
		}
	}
	return xr;
}

static int LineFillInArea( CArray2D<BYTE> *pArray, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY,
													 const int x, const int y, const int nDir, const int nPrevXl, const int nPrevXr, const BYTE nColor )
{
	if ( (x < nMinX) || (y < nMinY) || (x > nMaxX) || (y >= nMaxY) )
		return x;

	int xl = x;
	int xr = x;
	int c;

	c = nColor + 1;
	while ( (c != nColor) && (xl > nMinX) )
	{
		c = (*pArray)[y][--xl];
	}

	if ( c == nColor )
		xl++;

	c = nColor + 1;
	while ( (c != nColor) && (xr < nMaxX) )
	{
		c = (*pArray)[y][++xr];
	}

	if ( c == nColor )
		xr--;

	for ( int i = xl; i <= xr; ++i )
		(*pArray)[y][i] = nColor;

	if ( ((y + nDir) >= nMinY) && ((y + nDir) <= nMaxY) )
	{
		for ( int i = xl; i <= xr; ++i )
		{
			c = (*pArray)[y + nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y + nDir, nDir, xl, xr, nColor );
		}
	}
	if ( ((y - nDir) >= nMinY) && ((y - nDir) <= nMaxY) )
	{
		for ( int i = xl; i <= nPrevXl; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y - nDir, -nDir, xl, xr, nColor );
		}
		for ( int i = nPrevXr; i <= xr; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( c != nColor )
				i = LineFill( pArray, i, y - nDir, -nDir, xl, xr, nColor );
		}
	}
	return xr;
}

void SimpleFill( CArray2D<BYTE> *pArray, const int x, const int y, const BYTE nColor, const BYTE nBorderColor )
{
	LineFillEx( pArray, x, y, 1, x, x, nColor, nBorderColor );
}

static int CheckFill( CArray2D<BYTE> *pArray, const int x, const int y, const int nDir, const int nPrevXl, const int nPrevXr,
											const BYTE nColor, const BYTE nBorderColor, int *nFlag )
{
	if ( (x < 0) || (y < 0) || (x >= pArray->GetSizeX()) || (y >= pArray->GetSizeY()) )
	{
		(*nFlag) = false;
		return x;
	}

	if ( (y == 0) || (y == (pArray->GetSizeY() - 1)) )
		(*nFlag) = false;

	int xl = x;
	int xr = x;
	int c;

	c = 65535;
	while ( (c != nColor) && (c != nBorderColor) && (xl > 0) )
	{
		c = (*pArray)[y][--xl];
	}
	if ( (c != nColor) && (c != nBorderColor) )
		(*nFlag) = false;
	else
		++xl;

	c = 65535;
	while ( (c != nColor) && (c != nBorderColor) && (xr < (pArray->GetSizeX() - 1)) )
	{
		c = (*pArray)[y][++xr];
	}
	if ( (c != nColor) && (c != nBorderColor) )
		(*nFlag) = false;
	else
		--xr;

	for ( int i = xl; i <= xr; ++i )
		(*pArray)[y][i] = nColor;

	if ( ((y + nDir) >= 0) && ((y + nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= xr; ++i )
		{
			c = (*pArray)[y + nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFill( pArray, i, y + nDir, nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
	}
	if ( ((y - nDir) >= 0) && ((y - nDir) < pArray->GetSizeY()) )
	{
		for ( int i = xl; i <= nPrevXl; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFill( pArray, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
		for ( int i = nPrevXr; i <= xr; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFill( pArray, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
	}
	return xr;
}

static int CheckFillInArea( CArray2D<BYTE> *pArray, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY,
														const int x, const int y, const int nDir, const int nPrevXl, const int nPrevXr,
														const BYTE nColor, const BYTE nBorderColor, int *nFlag )
{
	if ( (x < nMinX) || (y < nMinY) || (x > nMaxX) || (y > nMaxY) )
	{
		(*nFlag) = false;
		return x;
	}

	if ( (y == nMinY) || (y == nMaxY) )
		(*nFlag) = false;

	int xl = x;
	int xr = x;
	int c;

	c = 65535;
	while ( (c != nColor) && (c != nBorderColor) && (xl > nMinX) )
	{
		c = (*pArray)[y][--xl];
	}
	if ( (c != nColor) && (c != nBorderColor) )
		(*nFlag) = false;
	else
		++xl;

	c = 65535;
	while ( (c != nColor) && (c != nBorderColor) && (xr < nMaxX) )
	{
		c = (*pArray)[y][++xr];
	}
	if ( (c != nColor) && (c != nBorderColor) )
		(*nFlag) = false;
	else
		--xr;

	for ( int i = xl; i <= xr; ++i )
		(*pArray)[y][i] = nColor;

	if ( ((y + nDir) >= nMinY) && ((y + nDir) <= nMaxY) )
	{
		for ( int i = xl; i <= xr; ++i )
		{
			c = (*pArray)[y + nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFillInArea( pArray, nMinX, nMinY, nMaxX, nMaxY, i, y + nDir, nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
	}
	if ( ((y - nDir) >= nMinY) && ((y - nDir) <= nMaxY) )
	{
		for ( int i = xl; i <= nPrevXl; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFillInArea( pArray, nMinX, nMinY, nMaxX, nMaxY, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
		for ( int i = nPrevXr; i <= xr; ++i )
		{
			c = (*pArray)[y - nDir][i];
			if ( (c != nColor) && (c != nBorderColor) )
				i = CheckFillInArea( pArray, nMinX, nMinY, nMaxX, nMaxY, i, y - nDir, -nDir, xl, xr, nColor, nBorderColor, nFlag );
		}
	}
	return xr;
}

void WiseFill( CArray2D<BYTE> *pMask, const BYTE nColor )
{
	NI_ASSERT( nColor != 0xff, "Color 0xff is forbidden for this type of filling" );
	int x, nFlag;
	for ( int y = 0; y < pMask->GetSizeY(); ++y )
	{
		x = 0;
		while ( x < pMask->GetSizeX() )
		{
			while ( (*pMask)[y][x] != nColor )
			{
				if ( ++x >= pMask->GetSizeX() )
					break;
			}
			if ( x >= pMask->GetSizeX() )
				break;
			while ( (*pMask)[y][x] == nColor )
			{
				if ( ++x >= pMask->GetSizeX() )
					break;
			}
			if ( x >= pMask->GetSizeX() )
				break;
			if ( (*pMask)[y][x] != 0xff )
			{
				nFlag = true;
				CheckFill( pMask, x, y, 1, x, x, 0xff, nColor, &nFlag );
				if ( nFlag )
					LineFill( pMask, x, y, 1, x, x, nColor );
			}
		}
	}
	for ( int g = 0; g < pMask->GetSizeY(); ++g )
	{
		for ( int i = 0; i < pMask->GetSizeX(); ++i )
		{
			if ( (*pMask)[g][i] != nColor )
				(*pMask)[g][i] = 0;
		}
	}
}

void WiseFillInArea( CArray2D<BYTE> *pMask, const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, const BYTE nColor )
{
	NI_ASSERT( (nMinX >= 0) && (nMaxX < pMask->GetSizeX()) && (nMinY >= 0) && (nMaxY < pMask->GetSizeY()), "Wrong area coordinates" );
	NI_ASSERT( (nMinX <= nMaxX) && (nMinY <= nMaxY), "Wrong area coordinates" );
	NI_ASSERT( nColor != 0xff, "Color 0xff is forbidden for this type of filling" );
	int x, nFlag;
	for ( int y = nMinY; y <= nMaxY; ++y )
	{
		x = nMinX;
		while ( x <= nMaxX )
		{
			while ( (*pMask)[y][x] != nColor )
			{
				if ( ++x > nMaxX )
					break;
			}
			if ( x > nMaxX )
				break;
			while ( (*pMask)[y][x] == nColor )
			{
				if ( ++x > nMaxX )
					break;
			}
			if ( x > nMaxX )
				break;
			if ( (*pMask)[y][x] != 0xff )
			{
				nFlag = true;
				CheckFillInArea( pMask, nMinX, nMinY, nMaxX, nMaxY, x, y, 1, x, x, 0xff, nColor, &nFlag );
				if ( nFlag )
					LineFillInArea( pMask, nMinX, nMinY, nMaxX, nMaxY, x, y, 1, x, x, nColor );
			}
		}
	}
	for ( int g = nMinY; g <= nMaxY; ++g )
	{
		for ( int i = nMinX; i <= nMaxX; ++i )
		{
			if ( (*pMask)[g][i] != nColor )
				(*pMask)[g][i] = 0;
		}
	}
}


