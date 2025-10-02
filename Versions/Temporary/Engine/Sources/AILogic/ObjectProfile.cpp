#include "stdafx.h"

#include "ObjectProfile.h"
#include "Common_RTS_AI/AIMap.h"
#include "Misc/Bresenham.h"
#include "System/FastMath.h"
#include "System/Commands.h"
#include "Misc/2Darray.h"

bool g_bThickLock;
START_REGISTER(ObjectProfile)
	REGISTER_VAR_EX( "thick_lock", NGlobal::VarBoolHandler, &g_bThickLock, false, STORAGE_NONE );
FINISH_REGISTER


struct STilesCollector
{
	hash_set<SVector, STilesHash> *pTilesUnder;

	STilesCollector( hash_set<SVector, STilesHash> *_pTilesUnder )
		: pTilesUnder( _pTilesUnder ) { }

	void operator()( const int x, const int y )
	{
		const SVector tile( x, y );
		if ( GetAIMap()->IsTileInside( tile ) )
			pTilesUnder->insert( tile );
	}
};

struct STilesArray2DCollector
{
	CArray2D<BYTE> *pTiles;
	SVector vTile;

	STilesArray2DCollector( SVector &_vTile, CArray2D<BYTE> *_pTiles )
		: pTiles( _pTiles ), vTile( _vTile ) { }

	void operator()( const int x, const int y )
	{
		const SVector tile( x, y );
		if ( GetAIMap()->IsTileInside( tile ) )
			(*pTiles)[y - vTile.y][x - vTile.x] = true;
	}
};

void CObjectProfile::AddCentersOfSmallPolygons()
{
	for ( int k = 0; k < profile.polygons.size(); ++k )
	{
		const vector<CVec2> &points = profile.polygons[k].verts;

		CVec2 vMassCenter( VNULL2 );
		for ( int i = 0; i < points.size() - 1; ++i )
			vMassCenter += points[i];

		vMassCenter /= float( points.size() - 1 );

		const float fMaxDist = sqr( SAIConsts::TILE_SIZE );
		bool bBig = false;
		for ( int i = 0; i < points.size() - 1 && !bBig; ++i )
		{
			const float fDist = fabs2( points[i] - vMassCenter );
			bBig = fDist >= fMaxDist;
		}

		if ( !bBig )
		{
			const SVector tile = AICellsTiles::GetTile( vCenter + (vMassCenter ^ vRotation) );
			if ( GetAIMap()->IsTileInside( tile ) )
				tilesUnder.insert( tile );
		}
	}
}

void CObjectProfile::Init( const NDb::SPassProfile &_profile, const CVec2 &_vCenter, const CVec2 &_vRotation, const bool bForceThickLock )
{
	profile = _profile;
	vCenter = _vCenter;
	vRotation = _vRotation;

	CVec2 vLeftDown( 1e10, 1e10 );
	CVec2 vRightUp( -1.0f, -1.0f );

	bool bInitted = false;
	for ( int i = 0; i < profile.polygons.size(); ++i )
	{
		for ( int j = 0; j < profile.polygons[i].verts.size(); ++j )
		{
			const CVec2 vRawVert = profile.polygons[i].verts[j];
			const CVec2 vVert = vCenter + (vRawVert ^ vRotation);

			vLeftDown.x = Min( vLeftDown.x, vVert.x );
			vLeftDown.y = Min( vLeftDown.y, vVert.y );
			vRightUp.x = Max( vRightUp.x, vVert.x );
			vRightUp.y = Max( vRightUp.y, vVert.y );
			bInitted = true;
		}
	}
	if ( !bInitted )
		return;
	aabbRect.Set( vLeftDown, vRightUp );

	SVector leftDownTile( vLeftDown.x / SConsts::TILE_SIZE, vLeftDown.y / SConsts::TILE_SIZE );
	SVector rightUpTile( vRightUp.x / SConsts::TILE_SIZE + 1, vRightUp.y / SConsts::TILE_SIZE + 1 );

	Clamp( leftDownTile.x, 0, GetAIMap()->GetSizeX() - 1 );
	Clamp( leftDownTile.y, 0, GetAIMap()->GetSizeY() - 1 );
	Clamp( rightUpTile.x, 0, GetAIMap()->GetSizeX() - 1 );
	Clamp( rightUpTile.y, 0, GetAIMap()->GetSizeY() - 1 );

	CArray2D<BYTE> passArray( rightUpTile.x - leftDownTile.x, rightUpTile.y - leftDownTile.y );
	passArray.FillZero();
	STilesArray2DCollector collector( leftDownTile, &passArray );

	for ( int i = 0; i < profile.polygons.size(); ++i )
	{
		for ( int j = 0; j < profile.polygons[i].verts.size() - 1; ++j )
		{
			const CVec2 vRawVert = profile.polygons[i].verts[j];
			const CVec2 vVert = vCenter + (vRawVert ^ vRotation);
			// by Bresenham
			const SVector startTile( AICellsTiles::GetTile( vVert ) );
			const CVec2 vNextVert( vCenter + (profile.polygons[i].verts[j+1] ^ vRotation) );
			const SVector endTile( AICellsTiles::GetTile( vNextVert ) );
			MakeLine2( startTile.x, startTile.y, endTile.x, endTile.y, collector );
		}
	}
	CArray2D<BYTE> passBorder( passArray );
	
	SVector tile;
	for ( tile.x = leftDownTile.x; tile.x <= rightUpTile.x; ++tile.x )
	{
		for ( tile.y = leftDownTile.y; tile.y <= rightUpTile.y; ++tile.y )
		{
			if ( GetAIMap()->IsTileInside( tile ) )
			{
				const CVec2 vLeftDown( tile.x * SAIConsts::TILE_SIZE, tile.y * SAIConsts::TILE_SIZE );
				const CVec2 v5( vLeftDown.x + SAIConsts::TILE_SIZE/2, vLeftDown.y + SAIConsts::TILE_SIZE/2 );
				if ( IsPointInside( v5 ) )
					tilesUnder.insert( tile );
			}
		}
	}

	if ( g_bThickLock || bForceThickLock )
	{
		// OR with border
		for ( int x = 0; x < passArray.GetSizeX(); ++x )
			for ( int y = 0; y < passArray.GetSizeY(); ++y )
			{
				passArray[y][x] |= passBorder[y][x];
				if ( passArray[y][x] )
					tilesUnder.insert( SVector( leftDownTile.x + x, leftDownTile.y + y ) );
			}
	}

	if ( IsPointInside( vCenter ) )
	{
		const SVector tile = AICellsTiles::GetTile( vCenter );
		if ( GetAIMap()->IsTileInside( tile ) )
			tilesUnder.insert( tile );
	}

	AddCentersOfSmallPolygons();

	tilesUnderVector.reserve( tilesUnder.size() );
	for ( hash_set<SVector, STilesHash>::iterator iter = tilesUnder.begin(); iter != tilesUnder.end(); ++iter )
		tilesUnderVector.push_back( *iter );
}

CObjectProfile::CObjectProfile( const NDb::SPassProfile &profile, const CVec3 &vCenter3D, const WORD wDir, const bool bForceThickLock )
{
	// client calculates dir slightly different from AI
	const float fAngle = float(wDir) / 65536 * FP_2PI;
	Init( profile, CVec2( vCenter3D.x, vCenter3D.y ), CVec2( NMath::Cos(fAngle), NMath::Sin(fAngle) ), bForceThickLock );
}

CObjectProfile::CObjectProfile( const SRect &unitRect, const bool bForceThickLock )
{
	NDb::SPassProfile profile;
	profile.polygons.resize( 1 );
	profile.polygons[0].verts.resize( 5 );

	profile.polygons[0].verts[0] = unitRect.v1 - unitRect.center;
	profile.polygons[0].verts[1] = unitRect.v2 - unitRect.center;
	profile.polygons[0].verts[2] = unitRect.v3 - unitRect.center;
	profile.polygons[0].verts[3] = unitRect.v4 - unitRect.center;
	profile.polygons[0].verts[4] = profile.polygons[0].verts[0];

	Init( profile, unitRect.center, CVec2( 1, 0 ), bForceThickLock );
}

static const float fEps = 0.00001f;

static const float SignEps( const float x )
{
	if ( fabs( x ) < fEps )
		return 0;
	else if ( x < 0 )
		return -1;
	else
		return 1;
}

static bool IsPointInsideSegment( const CVec2 &vPoint, const CVec2 &v1, const CVec2 &v2 )
{
	CLine2 line( v1, v2 );
	return 
		fabs( line.DistToPoint( vPoint ) ) < fEps &&
		SignEps( vPoint.x - v1.x ) * SignEps( v2.x - vPoint.x ) >= 0 &&
		SignEps( vPoint.y - v1.y ) * SignEps( v2.y - vPoint.y ) >= 0;
}


bool CObjectProfile::IsPointInside( const CVec2 &_vPoint ) const
{
	CVec2 vPoint = _vPoint - vCenter;
	const CVec2 vBackwardDir( vRotation.x, -vRotation.y );
	vPoint ^= vBackwardDir;

	for ( int k = 0; k < profile.polygons.size(); ++k )
	{
		const vector<CVec2> &points = profile.polygons[k].verts;

		int nCount = 0;
		for ( int i = 0; i < points.size() - 1; ++i )
		{
			if ( IsPointInsideSegment( vPoint, points[i], points[i+1] ) )
				return true;

			if ( points[i].y != points[i+1].y )
			{
				if ( points[i].y == vPoint.y || points[i+1].y == vPoint.y )
				{
					if ( points[i].y == vPoint.y && points[i].y > points[i+1].y && points[i].x > vPoint.x ||
						points[i+1].y == vPoint.y && points[i+1].y > points[i].y && points[i+1].x > vPoint.x )
					{
						++nCount;
					}
				}
				else if ( Sign( vPoint.y - points[i].y ) * Sign( points[i+1].y - vPoint.y ) > 0 )
				{
					const float fT = (vPoint.y - points[i].y)/(points[i+1].y - points[i].y);
					const float fX = points[i].x + fT * (points[i+1].x - points[i].x);

					if ( fX > vPoint.x )
						++nCount;
				}
			}
		}

		if ( nCount%2 == 1 )
			return true;
	}

	return false;
}

bool CObjectProfile::IsTileInside( const SVector &tile ) const
{
	return tilesUnder.find( tile ) != tilesUnder.end();
}

bool CObjectProfile::IsWeakIntersected( const SRect &unitRect ) const
{
	if ( IsPointInside( unitRect.v1 ) || IsPointInside( unitRect.v2 ) || IsPointInside( unitRect.v3 ) || IsPointInside( unitRect.v4 ) )
		return true;
	
	for ( int k = 0; k < profile.polygons.size(); ++k )
	{
		const vector<CVec2> &points = profile.polygons[k].verts;
		for ( int i = 0; i < points.size() - 1; ++i )
		{
			const CVec2 vPoint = vCenter + (points[i] ^ vRotation);
			if ( unitRect.IsPointInside( vPoint ) )
				return true;
		}
	}

	return false;
}

REGISTER_SAVELOAD_CLASS( 0x30156B00, CObjectProfile )

