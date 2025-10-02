#pragma once

#include "..\Misc\Bresenham.h"

//#define HEIGHT_MULTIPLYER 100

class CWarFogTracer
{
	CGlobalWarFog *pWarFog;
protected:
	const bool IsTileInside( const SVector &vTile ) const { return pWarFog->IsTileInside( vTile ); }
	const int GetStaticObjectAtTile( const SVector &vTile ) { return pWarFog->GetStaticObjectAtTile( vTile ); }
	
	const int GetHeight( const SVector &vTile ) const { return pWarFog->GetHeight( vTile ); }
	const int GetUnitHeight() const { return pWarFog->GetUnitHeight(); }
public:
	CWarFogTracer( CGlobalWarFog *_pWarFog ) : pWarFog( _pWarFog ) {}
};

template<class TVisitor>
class CWarFogTracerInternal : public CWarFogTracer
{
	TVisitor visitor;

public:
	CWarFogTracerInternal( CGlobalWarFog *pWarFog, const TVisitor &_visitor ) : CWarFogTracer( pWarFog ), visitor( _visitor ) {}

/*
	const bool TraceRay( const SVector &vOrigin, const SVector &vOffset )
	{
		CBres bres;
		bres.InitPoint( SVector( 0, 0 ) , vOffset );
		float fTan = -FP_MAX_VALUE;
		int nObjectTileID = -1;
		const float fOriginHeight = GetHeight( vOrigin ) + GetUnitHeight();

		do 
		{
			bres.MakePointStep();
			const SVector checkPoint = bres.GetDirection();
			const SVector tile = vOrigin + checkPoint;

			if ( !IsTileInside( tile ) )
			{
				visitor( checkPoint, false );
				return visitor.GetReturnValue();
			}
			else
			{
				const float fDeltaLen = fabs( checkPoint.x, checkPoint.y );
				const float fDeltaHeight = ( GetHeight( tile ) - fOriginHeight );
				const float fTanUnitNew =  ( fDeltaHeight + GetUnitHeight() ) / fDeltaLen;
				if ( fTanUnitNew >= fTan )
				{
					const float fTanNew = fDeltaHeight / fDeltaLen;
					if ( fTanNew > fTan )
						fTan = fTanNew;
					visitor( checkPoint, true );
					nObjectTileID = GetStaticObjectAtTile( tile );
				}
				else
					visitor( checkPoint, nObjectTileID > -1 && nObjectTileID == GetStaticObjectAtTile( tile ) );
			}
		} while( bres.GetDirection() != vOffset );
		return visitor.GetReturnValue();
	}
*/

// TraceRay with integers
	const bool TraceRay( const SVector &vOrigin, const SVector &vOffset )
	{
		CBres bres;
		bres.InitPoint( SVector( 0, 0 ) , vOffset );
		bres.MakePointStep();
		const int nUnitHeight = GetUnitHeight();
		int nLength0 = 1;
		int nLength = 1;

		int nOriginHeight = GetHeight( vOrigin ) + nUnitHeight;
		const SVector tile0 = bres.GetDirection() + vOrigin;
		if ( !IsTileInside( tile0 ) )
		{
			visitor( bres.GetDirection(), false );
			return visitor.GetReturnValue();
		}

		int nDeltaHeight0 = GetHeight( tile0 ) - nOriginHeight;
		int p1 = nDeltaHeight0;
		int p3 = nUnitHeight;

		visitor( bres.GetDirection(), true );
		int nObjectTileID = GetStaticObjectAtTile( tile0 );
		do 
		{
			bres.MakePointStep();
			const SVector checkPoint = bres.GetDirection();
			const SVector tile = vOrigin + checkPoint;

			if ( !IsTileInside( tile ) )
			{
				visitor( checkPoint, false );
				return visitor.GetReturnValue();
			}
			else
			{
				const int nDeltaHeight = GetHeight( tile ) - nOriginHeight;
				const int p2 = nDeltaHeight * nLength0;
				++nLength;
				p1 += nDeltaHeight0;
				const int dp = p2 - p1;
				if ( dp >= 0 )
				{
					nDeltaHeight0 = nDeltaHeight;
					nLength0 = nLength;
					p1 = nDeltaHeight0 * nLength;
					p3 = nUnitHeight * nLength0;

					visitor( checkPoint, true );
					nObjectTileID = GetStaticObjectAtTile( tile );
				}
				else if ( dp >= -p3 )
				{
					visitor( checkPoint, true );
					nObjectTileID = GetStaticObjectAtTile( tile );
				}
				else
					visitor( checkPoint, nObjectTileID > -1 && nObjectTileID == GetStaticObjectAtTile( tile ) );
			}
		} while( bres.GetDirection() != vOffset );
		return visitor.GetReturnValue();
	}

	void TraceOctuple( const SVector &vOrigin, const SVector &vOffset )
	{
		TraceRay( vOrigin, vOffset );
		TraceRay( vOrigin, SVector( -vOffset.x,  vOffset.y ) );
		TraceRay( vOrigin, SVector(  vOffset.x, -vOffset.y ) );
		TraceRay( vOrigin, SVector( -vOffset.x, -vOffset.y ) );

		TraceRay( vOrigin, SVector(  vOffset.y,  vOffset.x ) );
		TraceRay( vOrigin, SVector( -vOffset.y,  vOffset.x ) );
		TraceRay( vOrigin, SVector(  vOffset.y, -vOffset.x ) );
		TraceRay( vOrigin, SVector( -vOffset.y, -vOffset.x ) );
	}

	void TraceCircle( const SVector &vOrigin, const int nRadius )
	{
		int x = 0;
		int y = nRadius;
		int d = 3 - 2*nRadius;
		while ( x <= y )
		{
			TraceOctuple( vOrigin, SVector( x, y ) );
			if ( d < 0 )
			{
				d += 4*x + 6;
			}
			else
			{
				d += 4*( x-y ) + 10;
				y--;
				TraceOctuple( vOrigin, SVector( x, y ) );
			}
			x++;
		}
	}
};


