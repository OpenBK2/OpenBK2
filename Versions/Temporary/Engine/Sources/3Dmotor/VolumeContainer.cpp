#include "StdAfx.h"
#include "VolumeContainer.h"
namespace NCollider
{
vector<int> fetchBufferArray;
SSkipColliderAnalyzer skipAnalyzer;

// CVolumeContainer

void CVolumeContainer::Init( const CVec3 &_ptMin, const CVec3 &ptMax, float fRegularSize )
{
	ptMin = _ptMin;
	ASSERT( ptMax.x > ptMin.x && ptMax.y > ptMin.y && ptMax.z > ptMin.z );
	float fSize = Max( ptMax.x - ptMin.x, Max( ptMax.y - ptMin.y, ptMax.z - ptMin.z ) );
	int nDepth = 1;
	while ( fSize > fRegularSize && nDepth < 6 )
	{
		fSize /= 2;
		++nDepth;
	}
	volumeData.resize( nDepth );
	for ( int i = 0; i < nDepth; ++i )
	{
		int nSide = 1 << i;
		volumeData[i].tris.resize( nSide * nSide * nSide );
		for ( int k = 0; k < volumeData[i].tris.size(); ++k )
			volumeData[i].tris[k].resize(0);
	}
	int nSide = 1 << (nDepth - 1);
	fSize1x = nSide / ( ptMax.x - ptMin.x );
	fSize1y = nSide / ( ptMax.y - ptMin.y );
	fSize1z = nSide / ( ptMax.z - ptMin.z );
	fetchMark.clear();
	nLastFetch = 1;
}

void CVolumeContainer::GetCoords( SIntCoords *pRes, const CVec3 &src )
{
	pRes->x = Float2Int( ( src.x - ptMin.x ) * fSize1x - 0.5f );
	pRes->y = Float2Int( ( src.y - ptMin.y ) * fSize1y - 0.5f );
	pRes->z = Float2Int( ( src.z - ptMin.z ) * fSize1z - 0.5f );
}

vector<int> CVolumeContainer::fetchBufferArray;
void CVolumeContainer::Fetch( const SBound &bound )
{
	if ( volumeData.size() == 1 )
	{
		pFetched = &volumeData[0].tris[0];
		nFetched = pFetched->size();
		return;
	}
	pFetched = &fetchBufferArray;
	fetchBufferArray.resize( Max( fetchBufferArray.size(), fetchMark.size() ) );
	nFetched = 0;
	if ( fetchMark.empty() )
		return;
	//SIntCoords mn, mx;
	CVec3 ptMin( bound.s.ptCenter - bound.ptHalfBox );
	CVec3 ptMax( bound.s.ptCenter + bound.ptHalfBox );
	SVolumeBounds b;
	GetCoords( &b.mn, ptMin );
	GetCoords( &b.mx, ptMax );
	ClipVolume( &b );
	int nDepth = volumeData.size();
	int *pFetchMark = &fetchMark[0];
	int *pFetchBufferPtr = &fetchBufferArray[0];
	for ( int nLevel = 0; nLevel < nDepth; ++nLevel )
	{
		SLevelContainers &level = volumeData[nLevel];
		int nShift = nDepth - 1 - nLevel;
		int nZ1 = b.mn.z >> nShift, nZ2 = b.mx.z >> nShift;
		int nY1 = b.mn.y >> nShift, nY2 = b.mx.y >> nShift;
		int nX1 = b.mn.x >> nShift, nX2 = b.mx.x >> nShift;
		for ( int nZ = nZ1; nZ <= nZ2; ++nZ )
		{
			int nZIdx = nZ << (nLevel+nLevel);
			for ( int nY = nY1; nY <= nY2; ++nY )
			{
				int nYZIdx = nZIdx + ( nY << nLevel );
				for ( int nX = nX1; nX <= nX2; ++nX )
				{
					int nIdx = nYZIdx + nX, nLastFetchReg = nLastFetch;
					vector<int> &data = level.tris[nIdx];
					for ( vector<int>::iterator i = data.begin(), iEnd = data.end(); i != iEnd; ++i )
					{
						int nRes = *i;
						ASSERT( nRes < fetchMark.size() );
						if ( pFetchMark[nRes] != nLastFetchReg )
						{
							pFetchMark[nRes] = nLastFetchReg;
							ASSERT( ( pFetchBufferPtr - &fetchBufferArray[0] ) < fetchBufferArray.size() );
							*pFetchBufferPtr++ = nRes;
						}
					}
				}
			}
		}
	}
	++nLastFetch;
	nFetched = pFetchBufferPtr - &fetchBufferArray[0];
}

void CVolumeContainer::MakeVolume( SVolumeBounds *pRes, const SIntCoords &a, const SIntCoords &b, const SIntCoords &c )
{
	pRes->mn.x = Min( a.x, Min( b.x, c.x ) );
	pRes->mn.y = Min( a.y, Min( b.y, c.y ) );
	pRes->mn.z = Min( a.z, Min( b.z, c.z ) );
	pRes->mx.x = Max( a.x, Max( b.x, c.x ) );
	pRes->mx.y = Max( a.y, Max( b.y, c.y ) );
	pRes->mx.z = Max( a.z, Max( b.z, c.z ) );
}

void CVolumeContainer::InitFirstPt( SVolumeBounds *pRes, const SHMatrix &pos, const CVec3 &coord )
{
	SIntCoords pt;
	CVec3 ptReal;
	pos.RotateHVector( &ptReal, coord );
	GetCoords( &pt, ptReal );
	pRes->mn.x = pRes->mx.x = pt.x;
	pRes->mn.y = pRes->mx.y = pt.y;
	pRes->mn.z = pRes->mx.z = pt.z;
}

void CVolumeContainer::EnlargeVolumeBounds( SVolumeBounds *pRes, const SHMatrix &pos, const CVec3 &coord )
{
	SIntCoords pt;
	CVec3 ptReal;
	pos.RotateHVector( &ptReal, coord );
	GetCoords( &pt, ptReal );
	if ( pt.x > pRes->mx.x )
		pRes->mx.x = pt.x;
	if ( pt.x < pRes->mn.x )
		pRes->mn.x = pt.x;
	if ( pt.y > pRes->mx.y )
		pRes->mx.y = pt.y;
	if ( pt.y < pRes->mn.y )
		pRes->mn.y = pt.y;
	if ( pt.z > pRes->mx.z )
		pRes->mx.z = pt.z;
	if ( pt.z < pRes->mn.z )
		pRes->mn.z = pt.z;
}

void CVolumeContainer::MakeVolume( SVolumeBounds *pRes, const SHMatrix &pos, const SBound &bound )
{
	InitFirstPt( pRes, pos, bound.s.ptCenter - bound.ptHalfBox );
	EnlargeVolumeBounds( pRes, pos, bound.s.ptCenter + bound.ptHalfBox );
}

void CVolumeContainer::MakeVolume( SVolumeBounds *pRes, const SHMatrix &pos, const vector<CVec3> &coords )
{
	ASSERT( coords.size() > 0 );
	InitFirstPt( pRes, pos, coords[0] );
	for ( int i = 1; i < coords.size(); ++i )
		EnlargeVolumeBounds( pRes, pos, coords[i] );
}

bool CVolumeContainer::IsOut( const SVolumeBounds &t )
{
	int nMax = ( 1 << ( volumeData.size() - 1 ) ) - 1;
	return t.mx.x < 0 || t.mx.y < 0 || t.mx.z < 0 || t.mn.x > nMax || t.mn.y > nMax || t.mn.z > nMax;
}

void CVolumeContainer::ClipVolume( SVolumeBounds *pRes )
{
	int nMax = ( 1 << ( volumeData.size() - 1 ) ) - 1;
	pRes->mn.x = Max( 0, pRes->mn.x ); pRes->mn.y = Max( 0, pRes->mn.y ); pRes->mn.z = Max( 0, pRes->mn.z );
	pRes->mx.x = Min( nMax, pRes->mx.x ); pRes->mx.y = Min( nMax, pRes->mx.y ); pRes->mx.z = Min( nMax, pRes->mx.z );
}

void CVolumeContainer::Add( const SVolumeBounds &b, int nData )
{
	int nD = b.GetSize();
	int nDepth = volumeData.size();
	int nLevel = nDepth - 1;
	while ( nLevel > 0 && nD > 1 )
	{
		nLevel--;
		nD >>= 1;
	}
	int nShift = nDepth - 1 - nLevel;
	for ( int nZ = b.mn.z >> nShift; nZ <= (b.mx.z >> nShift); ++nZ )
	{
		for ( int nY = b.mn.y >> nShift; nY <= (b.mx.y >> nShift); ++nY )
		{
			for ( int nX = b.mn.x >> nShift; nX <= (b.mx.x >> nShift); ++nX )
			{
				int nIdx = ( nZ << (nLevel+nLevel) ) + ( nY <<nLevel ) + nX;
				volumeData[nLevel].tris[nIdx].push_back( nData );
			}
		}
	}
	if ( nData >= fetchMark.size() )
		fetchMark.resize( nData + 1, 0 );
}
}
