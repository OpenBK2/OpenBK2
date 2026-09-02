#include "stdafx.h"
#include <fmt/format.h>
#include <fmt/printf.h>

#include "libdb/Manipulator.h"
#include "SceneB2/TerraTools.h"
#include "Stats_B2_M1/DBPassProfile.h"
#include "Stats_B2_M1/Vis2AI.h"

#include <cstdint>

//#define _DEBUG_GENERATION

#ifdef _DEBUG_GENERATION
class CDebugMask
{
	CArray2D<uint32_t> mask;
	float fScaleCoeffX;
	float fScaleCoeffY;
	CVec2 vMin;
	CVec2 vMax;
public:
	CDebugMask() { }

	void Init( const CVec2 &_vMin, const CVec2 &_vMax )
	{
		vMin = _vMin;
		vMax = _vMax;

		const float fAISizeX = 60.0f * Vis2AI( vMax.x - vMin.x ) / AI_TILE_SIZE;
		const float fAISizeY = 60.0f * Vis2AI( vMax.y - vMin.y ) / AI_TILE_SIZE;

		fScaleCoeffX = fAISizeX / ( vMax.x - vMin.x );
		fScaleCoeffY = fAISizeY / ( vMax.y - vMin.y );

		mask.SetSizes( ceil( fAISizeX ), ceil( fAISizeY ) );
		Clear();
	}

	void Clear()
	{
		mask.FillZero();
	}

	void GetLocalCoord( const CVec2 &v, int *pnX, int *pnY )
	{
		*pnX = Clamp( (float)(v.x - vMin.x) * fScaleCoeffX, 0.0f, (float)(mask.GetSizeX() - 1) );
		*pnY = mask.GetSizeY() - Clamp( (float)(v.y - vMin.y) * fScaleCoeffY, 0.0f, (float)(mask.GetSizeY() - 1) ) - 1;
	}

	void DrawLine( const CVec2 &v1, const CVec2 &v2, const NImage::SColor &color )
	{
		int nX1, nY1, nX2, nY2;
		GetLocalCoord( v1, &nX1, &nY1 );
		GetLocalCoord( v2, &nX2, &nY2 );

		CBresenham2 line( nX1, nY1, nX2, nY2 );
		while ( !line.IsEnd() )
		{
			mask[line.GetY()][line.GetX()] = color;
			line.Next();
		}
		mask[line.GetY()][line.GetX()] = color;
	}

	void DrawPoint( const CVec2 &v, const NImage::SColor &color )
	{
		const float fShift = 0.04f;
		DrawLine( CVec2( v.x - fShift, v.y - fShift ), CVec2( v.x + fShift, v.y - fShift ), color );
		DrawLine( CVec2( v.x - fShift, v.y + fShift ), CVec2( v.x + fShift, v.y + fShift ), color );
		DrawLine( CVec2( v.x - fShift, v.y - fShift ), CVec2( v.x - fShift, v.y + fShift ), color );
		DrawLine( CVec2( v.x + fShift, v.y - fShift ), CVec2( v.x + fShift, v.y + fShift ), color );
	}

	void SaveImage( const std::string &szFileName )
	{
		CFileStream imageStream( CreateStream( szFileName, STREAM_PATH_ABSOLUTE ) );
		NImage::SaveImageAsTGA( &imageStream, mask );
	}
};
static CDebugMask mask;

#endif //_DEBUG_GENERATION

class CPointsSort
{
	CVec2 vStart;
public:
	CPointsSort( const CVec2 &v1 ) : vStart( v1 ) { }

	bool operator()( const CVec2 &v1, const CVec2 &v2 ) const
	{
		return fabs2( v1 - vStart ) < fabs2( v2 - vStart );
	}
};

struct SConnectedPoint
{
	CVec2 vPoint;
	int nNumber;

	SConnectedPoint() { }
	SConnectedPoint( const CVec2 &_vPoint, const int _nNumber )
		: vPoint( _vPoint ), nNumber( _nNumber ) { }
};

struct SPolygon
{
	std::vector<CVec2> points;
};

static const float fEps = 0.0001f;
class CPassabilityProfileCreator
{
	std::list<CSegment> segments;
	float fInfinity;

	std::list<SConnectedPoint> connectedPoints;
	std::unordered_map<int, CVec2> connNum2Point;
	CArray2D<int> connections;
	std::unordered_set<int> deleted;
	std::list<SPolygon> cover;

	//
	void GenerateStartSegments( const std::string &szFileName, const float fZEps );
	void GenerateAllSegments();
	void MakeConnections();
	void DelNotBreaks();
	void FormPolygon();
	void DelPointsInsideOfPolygon( SPolygon &polygon );
	void SimplifyPolygons();
	const int FindNumberForPoint( const CVec2 &vPoint );
	void AddEdge( const int n1, const int n2 );
public:
	CPassabilityProfileCreator(const std::string &szGrannyFileName, const float fZEps, NDb::SPassProfile *pPassProfile );
};

void CPassabilityProfileCreator::GenerateStartSegments( const std::string &szFileName, const float fZEps )
{
	std::vector<CVec3> verts;
	std::vector<STriangle> trgs;
	CVec3 vMin, vMax;

	LoadGrannyModel( szFileName, &verts, &trgs, &vMin, &vMax );
	fInfinity = 1000.0f * fabs( vMax - vMin );

#ifdef _DEBUG_GENERATION		
	mask.Init( CVec2(vMin.x, vMin.y), CVec2(vMax.x, vMax.y) );
#endif //_DEBUG_GENERATION

	const float fMaxZ = vMin.z + fZEps;

	segments.clear();
	for ( std::vector<STriangle>::iterator it = trgs.begin(); it != trgs.end(); ++it )
	{
		std::vector<CVec3> points( 4 );
		points[0] = verts[it->i1];
		points[1] = verts[it->i2];
		points[2] = verts[it->i3];
		points[3] = verts[it->i1];

		std::vector<CVec2> prjPoints;
		for ( int i = 0; i < 3; ++i )
		{
			const uint8_t cFirst = (points[i].z <= fMaxZ) ? 1 : 0;
			const uint8_t cSecond = (points[i+1].z <= fMaxZ) ? 1 : 0;

			const CVec2 vFirst( points[i].x, points[i].y );
			const CVec2 vSecond( points[i+1].x, points[i+1].y );

			if ( cFirst )
				prjPoints.push_back( vFirst );

			if ( cFirst ^ cSecond && points[i].z != points[i+1].z )
			{
				const float fT = (fMaxZ - points[i].z) / (points[i+1].z - points[i].z);
				prjPoints.push_back( vFirst + fT * (vSecond - vFirst) );
			}

			if ( cSecond )
				prjPoints.push_back( vSecond );
		}

		if ( !prjPoints.empty() )
		{
			for ( int i = 0; i < prjPoints.size(); ++i )
				Vis2AI( &(prjPoints[i]) );
			
			prjPoints.push_back( prjPoints[0] );
			for ( int i = 0; i < prjPoints.size() - 1; ++i )
			{
				if ( prjPoints[i] != prjPoints[i + 1] )
					segments.push_back( CSegment( prjPoints[i], prjPoints[i + 1] ) );
			}
		}
	}

#ifdef _DEBUG_GENERATION	
	for ( std::list<CSegment>::iterator iter = segments.begin(); iter != segments.end(); ++iter )
		mask.DrawLine( iter->p1, iter->p2, NImage::SColor( 100, 0, 255, 0 ) );

	const std::string szImageName = NFile::GetFilePath( szGrannyFileName ) + NFile::GetFileTitle( szGrannyFileName );
	mask.SaveImage( szImageName + "_1.tga" );
	mask.Clear();
#endif //_DEBUG_GENERATION
}

template<class T>
static bool IsEqual( const T &fVar, const T &fValue )
{
	return fabs( fVar - fValue ) < fEps;
}

static const float Sign( const float x )
{
	if ( fabs( x ) < fEps )
		return 0;
	else if ( x < 0 )
		return -1;
	else
		return 1;
}

static const float ExactSign( const float fX )
{
	if ( fX > 0 )
		return 1;
	else if ( fX < 0 )
		return -1;
	else
		return 0;
}

bool IsPointInside( const CVec2 &vPoint, const CVec2 &v1, const CVec2 &v2 )
{
	CLine2 line( v1, v2 );
	return 
		IsEqual( line.DistToPoint( vPoint ), 0.0f ) &&
		Sign( vPoint.x - v1.x ) * Sign( v2.x - vPoint.x ) >= 0 &&
		Sign( vPoint.y - v1.y ) * Sign( v2.y - vPoint.y ) >= 0;
}

static void GetIntersections( const CVec2 &v11, const CVec2 &v12, const CVec2 &v21, const CVec2 &v22, std::list<CVec2> *pRes )
{
	pRes->clear();

	const CLine2 line1( v11, v12 );
	const CLine2 line2( v21, v22 );

	const float fDet = Det( line1.a, line2.a, line1.b, line2.b );

	if ( IsPointInside( v11, v21, v22 ) )
		pRes->push_back( v11 );
	if ( IsPointInside( v12, v21, v22 ) )
		pRes->push_back( v12 );
	if ( IsPointInside( v21, v11, v12 ) )
		pRes->push_back( v21 );
	if ( IsPointInside( v22, v11, v12 ) )
		pRes->push_back( v22 );

	if ( fDet != 0.0f )
	{
		CVec2 vRes;
		vRes.x = -Det( line1.c, line2.c, line1.b, line2.b ) / fDet;
		vRes.y = -Det( line1.a, line2.a, line1.c, line2.c ) / fDet;

		if ( IsPointInside( vRes, v11, v12 ) && IsPointInside( vRes, v21, v22 ) )
			pRes->push_back( vRes );
	}
}

void CPassabilityProfileCreator::GenerateAllSegments()
{
	std::list<CSegment> newSegments;

	for ( std::list<CSegment>::iterator iter = segments.begin(); iter != segments.end(); ++iter )
	{
		if ( iter->p2 - iter->p1 != VNULL2 )
		{
			std::list<CVec2> newPoints;
			newPoints.push_back( iter->p1 );

			for ( std::list<CSegment>::iterator iter1 = segments.begin(); iter1 != segments.end(); ++iter1 )
			{
				if ( iter1->p1 - iter1->p2 != VNULL2 )
				{
					std::list<CVec2> points;
					const CVec2 &p1 = iter->p1;
					const CVec2 &p2 = iter->p2;
					const CVec2 &p3 = iter1->p1;
					const CVec2 &p4 = iter1->p2;

					GetIntersections( p1, p2, p3, p4, &points );
					newPoints.splice( newPoints.end(), points );
				}
			}
			newPoints.push_back( iter->p2 );

			newPoints.sort( CPointsSort( iter->p1 ) );

			std::list<CVec2>::iterator pred = newPoints.begin();
			std::list<CVec2>::iterator cur = pred;
			++cur;
			for ( ; cur != newPoints.end(); ++cur, ++pred )
			{
				if ( *pred != *cur )
					newSegments.push_back( CSegment( *pred, *cur ) );
			}
		}
	}

	segments.swap( newSegments );

#ifdef _DEBUG_GENERATION
	for ( std::list<CSegment>::iterator iter = segments.begin(); iter != segments.end(); ++iter )
		mask.DrawLine( iter->p1, iter->p2, NImage::SColor( 255, 0, 255, 0 ) );
	mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );
#endif //_DEBUG_GENERATION
}

const int CPassabilityProfileCreator::FindNumberForPoint( const CVec2 &vPoint )
{
	int nNumber = -1;
	int nMaxNumber = -1;
	for ( std::list<SConnectedPoint>::iterator iter = connectedPoints.begin(); iter != connectedPoints.end() && nNumber == -1; ++iter )
	{
		if ( IsEqual( vPoint, iter->vPoint ) )
			nNumber = iter->nNumber;

		nMaxNumber = (std::max)( nMaxNumber, iter->nNumber );
	}

	return nNumber == -1 ? nMaxNumber + 1 : nNumber;
}

void CPassabilityProfileCreator::AddEdge( const int n1, const int n2 )
{
	connections[n1][n2] = 1;;
	connections[n2][n1] = 1;
}

void CPassabilityProfileCreator::MakeConnections()
{
	int nMaxPoint = -1;
	for ( std::list<CSegment>::iterator iter = segments.begin(); iter != segments.end(); ++iter )
	{
		const CVec2 v1 = iter->p1;
		const CVec2 v2 = iter->p2;

		if ( !IsEqual( v1, v2 ) )
		{
			connectedPoints.push_back( SConnectedPoint( v1, FindNumberForPoint( v1 ) ) );
			const int n1 = connectedPoints.back().nNumber; 
			connNum2Point[n1] = v1;

			connectedPoints.push_back( SConnectedPoint( v2, FindNumberForPoint( v2 ) ) );
			const int n2 = connectedPoints.back().nNumber; 
			connNum2Point[n2] = v2;

			nMaxPoint = (std::max)( nMaxPoint, (std::max)( n1, n2 ) );
		}
	}

	connections.SetSizes( nMaxPoint + 1, nMaxPoint + 1 );
	connections.FillZero();

	std::list<SConnectedPoint>::iterator iter = connectedPoints.begin();
	while ( iter != connectedPoints.end() )
	{
		const int n1 = iter->nNumber;
		++iter;
		const int n2 = iter->nNumber;
		++iter;
		AddEdge( n1, n2 );
	}

	/*
	mask.Clear();
	for ( std::unordered_map<int, CVec2>::iterator iter = connNum2Point.begin(); iter != connNum2Point.end(); ++iter )
	{
	mask.DrawPoint( iter->second, NImage::SColor( 255,255,0,0 ) );
	const int n = iter->first;
	for ( int i = 0; i < connections.GetSizeX(); ++i )
	{
	if ( connections[n][i] )
	mask.DrawLine( connNum2Point[n], connNum2Point[i], NImage::SColor( 255, 0, 0, 255 ) );
	}
	}
	mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );
	*/
}

void CPassabilityProfileCreator::DelNotBreaks()
{
	std::unordered_set<int> goodPoints;

	for ( int i = 0; i < connections.GetSizeX(); ++i )
	{
		bool bOnOneLine = true;
		bool bFoundLine = false;
		CLine2 line;

		//mask.DrawPoint( connNum2Point[i], NImage::SColor( 255, 0, 0, 255 ) );
		//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

		for ( int j = 0; j < connections.GetSizeY() && bOnOneLine; ++j )
		{
			//mask.DrawPoint( connNum2Point[j], NImage::SColor( 255, 0, 255, 0 ) );
			//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

			if ( connections[i][j] == 1 )
			{
				if ( !bFoundLine )
				{
					line = CLine2( connNum2Point[i], connNum2Point[j] );
					bFoundLine = true;
				}
				else
					bOnOneLine = fabs( line.DistToPoint( connNum2Point[j] ) ) < fEps;
			}

			//mask.DrawPoint( connNum2Point[j], NImage::SColor( 255, 0, 0, 0 ) );
			//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );
		}

		if ( !bOnOneLine )
			goodPoints.insert( i );

		//mask.DrawPoint( connNum2Point[i], NImage::SColor( 255, 0, 0, 0 ) );
		//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );
	}

	//for ( std::unordered_set<int>::iterator iter = goodPoints.begin(); iter != goodPoints.end(); ++iter )
	//	mask.DrawPoint( connNum2Point[*iter], NImage::SColor( 255, 255, 255, 0 ) );
	//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

	for ( int k = 0; k < connections.GetSizeX(); ++k )
	{
		if ( goodPoints.find( k ) == goodPoints.end() )
		{
			for ( int i = 0; i < connections.GetSizeX(); ++i )
			{
				for ( int j = 0; j < connections.GetSizeY(); ++j )
				{
					if ( connections[i][k] == 1 && connections[k][j] == 1 )
						connections[i][j] = 1;
				}
			}
		}
	}

	std::unordered_map<int, std::unordered_set<int> > connTable;
	for ( int i = 0; i < connections.GetSizeX(); ++i )
	{
		if ( goodPoints.find( i ) != goodPoints.end() )
		{
			for ( int j = 0; j < connections.GetSizeX(); ++j )
			{
				if ( i != j && connections[i][j] == 1 && goodPoints.find( j ) != goodPoints.end() )
					connTable[i].insert( j );
			}
		}
	}

	std::unordered_set<int> toDelete;
	for ( int i = 0; i < connections.GetSizeX(); ++i )
	{
		if ( !connTable.empty() && connTable[i].size() < 2 )
			toDelete.insert( i );
	}

	while ( !toDelete.empty() )
	{
		std::unordered_set<int> toDeleteNew;
		for ( std::unordered_set<int>::iterator iter = toDelete.begin(); iter != toDelete.end(); ++iter )
		{
			const int i = *iter;
			for ( std::unordered_set<int>::iterator iter1 = connTable[i].begin(); iter1 != connTable[i].end(); ++iter1 )
			{
				const int j = *iter1;
				connTable[j].erase( i );
				if ( !connTable[j].empty() && connTable[j].size() < 2 )
					toDeleteNew.insert( j );
			}
			connTable[i].clear();
		}

		std::swap( toDelete, toDeleteNew );
	}

	connections.FillZero();
	for ( int i = 0; i < connections.GetSizeX(); ++i )
	{
		if ( connTable[i].empty() )
			connNum2Point.erase( i );
		else
		{
			for ( std::unordered_set<int>::iterator iter = connTable[i].begin(); iter != connTable[i].end(); ++iter )
				connections[i][*iter] = 1;
		}
	}
}

void CPassabilityProfileCreator::FormPolygon()
{
	SConnectedPoint point;
	bool bFirst = true;
	for ( std::unordered_map<int, CVec2>::iterator iter = connNum2Point.begin(); iter != connNum2Point.end(); ++iter )
	{
		if ( deleted.find( iter->first ) == deleted.end() )
		{
			const CVec2 vPoint = iter->second;
			if ( bFirst || vPoint.y < point.vPoint.y || vPoint.y == point.vPoint.y && vPoint.x < point.vPoint.x )
			{
				point.vPoint= vPoint;
				point.nNumber = iter->first;
				bFirst = false;
			}
		}
	}

	cover.emplace_back();
	SPolygon &polygon = cover.back();
	polygon.points.push_back( point.vPoint );
	SConnectedPoint predPoint( CVec2( point.vPoint.x - fInfinity, point.vPoint.y ), -1 );

	do
	{
		//mask.DrawPoint( point.vPoint, NImage::SColor( 255, 255, 0, 0 ) );
		//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

		bool bFirst = true;
		SConnectedPoint bestPoint;
		bestPoint.nNumber = -1;
		for ( int i = 0; i < connections.GetSizeX(); ++i )
		{
			if ( connections[point.nNumber][i] == 1 && deleted.find( i ) == deleted.end() )
			{				
				const SConnectedPoint nextPoint( connNum2Point[i], i );

				//mask.DrawPoint( predPoint.vPoint, NImage::SColor( 255, 0, 0, 255 ) );
				//mask.DrawPoint( point.vPoint, NImage::SColor( 255, 255, 0, 0 ) );
				//mask.DrawPoint( nextPoint.vPoint, NImage::SColor( 255, 255, 255, 0 ) );
				//if ( !bFirst )
				//	mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 255, 0 ) );
				//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

				if ( point.nNumber != nextPoint.nNumber && predPoint.nNumber != nextPoint.nNumber //&&
					//						!IsPointInside( nextPoint.vPoint, predPoint.vPoint, point.vPoint ) //&&
					/*( bFirst || !IsPointInside( nextPoint.vPoint, point.vPoint, bestPoint.vPoint ) )*/ )
				{
					if ( bFirst )
					{
						//						mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
						bestPoint = nextPoint;
						bFirst = false;
					}
					else
					{
						if ( TriangleArea2( predPoint.vPoint, point.vPoint, nextPoint.vPoint ) == 0 )
						{
							if ( TriangleArea2( predPoint.vPoint, point.vPoint, bestPoint.vPoint ) > 0 ||
								IsPointInside( bestPoint.vPoint, point.vPoint, nextPoint.vPoint ) )
							{
								//mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
								bestPoint = nextPoint;
							}
						}
						else if ( TriangleArea2( predPoint.vPoint, point.vPoint, nextPoint.vPoint ) <= 0 )
						{
							if ( TriangleArea2( predPoint.vPoint, point.vPoint, bestPoint.vPoint ) > 0 )
							{
								//mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
								bestPoint = nextPoint;
							}
							else if ( TriangleArea2( point.vPoint, bestPoint.vPoint, nextPoint.vPoint ) < 0 )
							{
								//mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
								bestPoint = nextPoint;
							}
						}
						else if ( TriangleArea2( predPoint.vPoint, point.vPoint, bestPoint.vPoint ) > 0 )
						{
							if ( TriangleArea2( point.vPoint, bestPoint.vPoint, nextPoint.vPoint ) < 0 )
							{
								//mask.DrawPoint( bestPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
								bestPoint = nextPoint;
							}
						}
					}
				}

				//mask.DrawPoint( nextPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
			}
		}

		//mask.DrawPoint( predPoint.vPoint, NImage::SColor( 255, 0, 0, 0 ) );
		// polygon not found
		if ( bestPoint.nNumber == -1 )
			return;

		predPoint = point;
		point = bestPoint;
		polygon.points.push_back( bestPoint.vPoint );

		//mask.DrawPoint( predPoint.vPoint, NImage::SColor( 255, 0, 0, 255 ) );
	}
	while ( !IsEqual( polygon.points.front(), polygon.points.back() ) && polygon.points.size() <= connNum2Point.size() + 10 );
}

void CPassabilityProfileCreator::DelPointsInsideOfPolygon( SPolygon &polygon )
{
	if ( polygon.points.size() == 1 )
	{
		for ( std::unordered_map<int, CVec2>::iterator iter = connNum2Point.begin(); iter != connNum2Point.end(); ++iter )
		{
			const CVec2 &vPoint = iter->second;
			if ( IsEqual( vPoint, polygon.points.front() ) )
				deleted.insert( iter->first );
		}
	}
	else
	{
		const std::vector<CVec2> &points = polygon.points;
		for ( std::unordered_map<int, CVec2>::iterator iter = connNum2Point.begin(); iter != connNum2Point.end(); ++iter )
		{
			const CVec2 &vPoint = iter->second;

			bool bInside = false;
			int nCount = 0;
			for ( int i = 0; i < points.size() - 1 && !bInside; ++i )
			{
				if ( IsPointInside( vPoint, points[i], points[i+1] ) )
					bInside = true;
				else if ( points[i].y != points[i+1].y )
				{
					if ( points[i].y == vPoint.y || points[i+1].y == vPoint.y )
					{
						if ( points[i].y == vPoint.y && points[i].y > points[i+1].y && points[i].x > vPoint.x ||
							points[i+1].y == vPoint.y && points[i+1].y > points[i].y && points[i+1].x > vPoint.x )
						{
							++nCount;
						}
					}
					else if ( ExactSign( vPoint.y - points[i].y ) * ExactSign( points[i+1].y - vPoint.y ) > 0 )
					{
						const float fT = (vPoint.y - points[i].y)/(points[i+1].y - points[i].y);
						const float fX = points[i].x + fT * (points[i+1].x - points[i].x);

						if ( fX > vPoint.x )
							++nCount;
					}
				}
			}

			if ( bInside || nCount%2 == 1 )
				deleted.insert( iter->first );
		}
	}
}

void CPassabilityProfileCreator::SimplifyPolygons()
{
	const float fTolerance = 0.05f;

	std::list<SPolygon>::iterator iter = cover.begin();
	while ( iter != cover.end() )
	{
		SPolygon &polygon = *iter;
		if ( polygon.points.size() >= 3 )
		{
			//mask.Clear();
			//for ( int i = 0; i < polygon.points.size() - 1; ++i )
			//	mask.DrawLine( polygon.points[i], polygon.points[i+1], NImage::SColor( 255, 255, 255, 0 ) );
			//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

			bool bChanged;
			do
			{
				bChanged = false;
				std::vector<int> toDelete( polygon.points.size(), 0 );

				CVec2 vPredPoint = polygon.points[0];
				CVec2 vPoint = polygon.points[1];
				CLine2 line( vPredPoint, vPoint );

				for ( int i = 2; i < polygon.points.size(); ++i )
				{
					const CVec2 &vNextPoint = polygon.points[i];

					//mask.DrawPoint( vPredPoint, NImage::SColor( 255, 0, 0, 255 ) );
					//mask.DrawPoint( vPoint, NImage::SColor( 255, 255, 0, 0 ) );
					//mask.DrawPoint( vNextPoint, NImage::SColor( 255, 0, 255, 255 ) );
					//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

					//mask.DrawPoint( vPredPoint, NImage::SColor( 255, 0, 0, 0 ) );
					//mask.DrawPoint( vPoint, NImage::SColor( 255, 0, 0, 0 ) );
					//mask.DrawPoint( vNextPoint, NImage::SColor( 255, 0, 0, 0 ) );
					//mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" ); 

					if ( fabs( line.DistToPoint(vNextPoint) ) < fTolerance )
					{
						toDelete[i-1] = 1;
						bChanged = true;
					}
					else
					{
						line = CLine2( vPoint, vNextPoint );
						vPredPoint = vPoint;
					}

					vPoint = vNextPoint;
				}

				int j = 0;
				for ( int i = 0; i < polygon.points.size(); ++i )
				{
					if ( toDelete[i] != 1 )
						polygon.points[j++] = polygon.points[i];
				}

				polygon.points.resize( j );
			}
			while ( bChanged );
		}

		if ( polygon.points.size() < 3 )
			iter = cover.erase( iter );
		else
			++iter;
	}
}

CPassabilityProfileCreator::CPassabilityProfileCreator(const std::string &szGrannyFileName, const float fZEps, NDb::SPassProfile *pPassProfile )
: fInfinity( 0.0f )
{
	GenerateStartSegments( szGrannyFileName, fZEps );
	GenerateAllSegments();
	if ( !segments.empty() )
	{
		MakeConnections();
		DelNotBreaks();

		while ( deleted.size() != connNum2Point.size() )
		{
			FormPolygon();

			SPolygon &polygon = cover.back();
			//		mask.DrawPoint( polygon.points[polygon.points.size()-1], NImage::SColor( 255, 255, 0, 0 ) );
			//		mask.SaveImage( "c:\\m1\\Geometries\\debug.tga" );

			if ( polygon.points.size() > connNum2Point.size() + 5 )
			{
				cover.pop_back();
				break;
			}
			else
			{
				DelPointsInsideOfPolygon( polygon );
				// don't need the polygon from only one point
				if ( polygon.points.size() == 1 )
					cover.pop_back();
			}
		}
	}

	SimplifyPolygons();

	pPassProfile->polygons.clear();
	pPassProfile->polygons.resize( cover.size() );
	int nCnt = 0;
	for ( std::list<SPolygon>::iterator iter = cover.begin(); iter != cover.end(); ++iter, ++nCnt )
	{
		SPolygon &polygon = *iter;
		pPassProfile->polygons[nCnt].verts.swap( polygon.points );
	}
}

bool CreateObjectPassabilityProfile( const std::string &szGrannyFileName, const float fZEps, NDb::SPassProfile *pPassProfile )
{
	CPassabilityProfileCreator profileCreator( szGrannyFileName, fZEps, pPassProfile );

#ifdef _DEBUG_GENERATION
	mask.Clear();
	for ( std::list<SPolygon>::iterator iter = cover.begin(); iter != cover.end(); ++iter )
	{
		const SPolygon &polygon = *iter;
		for ( int i = 0; i < polygon.points.size() - 1; ++i )
		{
			mask.DrawPoint( polygon.points[i], NImage::SColor( 255, 255, 0, 0 ) );
			mask.DrawLine( polygon.points[i], polygon.points[i+1], NImage::SColor( 255, 255, 255, 0 ) );
		}
	}
	mask.SaveImage( szImageName + "_2.tga" );
#endif //_DEBUG_GENERATION

	return true;
}

void SavePassProfile( const NDb::SPassProfile &passProfile, const std::string &_szPrefix, const std::string &szFieldName, IManipulator *pManipulator )
{
	const std::string szPrefix = _szPrefix.empty() ? _szPrefix : _szPrefix + ".";
	const std::string szStructName = szPrefix + szFieldName + ".polygons";
	pManipulator->RemoveNode( szStructName );//szPrefix + "PassProfile.polygons" );
	if ( !passProfile.polygons.empty() )
	{
		for ( int i = 0; i < passProfile.polygons.size(); ++i )
		{
			pManipulator->InsertNode( szStructName /* szPrefix + "PassProfile.polygons"*/, i );

			for ( int j = 0; j < passProfile.polygons[i].verts.size(); ++j )
			{
				std::string szNode = fmt::sprintf( (szStructName + ".[%d].verts").c_str(), i );//    szPrefix + "PassProfile.polygons.[%d].verts").c_str(), i );
				pManipulator->InsertNode( szNode, j );

				szNode += fmt::format( ".[{}].", j );
				pManipulator->SetValue( szNode + "x", CVariant( passProfile.polygons[i].verts[j].x ) );
				pManipulator->SetValue( szNode + "y", CVariant( passProfile.polygons[i].verts[j].y ) );
			}
		}
	}
}


