#pragma once

#include <algorithm>
#include <cstdint>

class CAddPointFunctional
{
	std::vector<SSpiralPoint> *pPoints;
	CArray2D1Bit *pMask;
	int nRadius;
	int nOffset;

protected:
	void SetPoint( int x, int y )
	{
		if ( pMask->GetData( x+nOffset, y+nOffset ) == 0 )
		{
			pPoints->push_back( SSpiralPoint( x, y, nRadius ) );
			pMask->SetData( x+nOffset, y+nOffset );
		}
	}

public:
	CAddPointFunctional( std::vector<SSpiralPoint> *_pPoints, CArray2D1Bit *_pMask, int _nRadius ) : pPoints( _pPoints ), pMask( _pMask ), nRadius( _nRadius ) { nOffset = ( pMask->GetSizeX()-1 ) / 2; }
	void operator()( int x, int y ) { SetPoint( x, y ); }
};

class CAddPoint8Functional : public CAddPointFunctional
{
public:
	CAddPoint8Functional( std::vector<SSpiralPoint> *_pPoints, CArray2D1Bit *_pMask, int _nRadius ) : CAddPointFunctional( _pPoints, _pMask, _nRadius ) {}

	void operator()( int x, int y )
	{
		SetPoint(  x,  y );
		SetPoint( -x,  y );
		SetPoint(  x, -y );
		SetPoint( -x, -y );
		SetPoint(  y,  x );
		SetPoint( -y,  x );
		SetPoint(  y, -x );
		SetPoint( -y, -x );
	}
};

struct SSpiralPointSort
{
	bool operator()( const SSpiralPoint &pPoint1, const SSpiralPoint &pPoint2 ) const 
	{ 
		if ( pPoint1.nRadius == pPoint2.nRadius )
		{
			if ( pPoint1.sector.wStartAngle == pPoint2.sector.wStartAngle )
				return false;
			else
				return ( pPoint1.sector.wStartAngle < pPoint2.sector.wStartAngle );
		}
		else
			return ( pPoint1.nRadius < pPoint2.nRadius );
	}
};

void GenerateSpiral( std::vector<SSpiralPoint> &spiral, CArray2D<int> &spiralCoords, std::vector<int> &lengths, const int nMaxRadius )
{
	spiral.clear();
	spiralCoords.Clear();
	lengths.clear();

	CArray2D1Bit mask;
	mask.SetSizes( 2 * nMaxRadius + 1, 2 * nMaxRadius + 1 );
	mask.FillZero();
	spiralCoords.SetSizes( 2 * nMaxRadius + 1, 2 * nMaxRadius + 1 );
	spiralCoords.FillEvery( -1 );

	// fill spiral with radius, offset and center angle
	CAddPointFunctional funcZero( &spiral, &mask, 0 );
	funcZero( 0, 0 );
	if ( nMaxRadius > 0 )
	{
		lengths.push_back( 0 );
		for ( int nRadius = 1; nRadius <= nMaxRadius; ++nRadius )
		{
			CAddPoint8Functional funcCurrentRadius( &spiral, &mask, nRadius );
			int x = 0;
			int y = nRadius;
			int d = 3 - 2*nRadius;
			while ( x <= y )
			{
				funcCurrentRadius( x, y );
				if ( d < 0 )
					d += 4*x + 6;
				else
				{
					d += 4*(x-y) + 10;
					y--;
					funcCurrentRadius( x, y );
				}
				x++;
			}
			lengths.push_back( spiral.size() );
		}

		// sort spiral and calculate segments for every cell
		std::sort( spiral.begin(), spiral.end(), SSpiralPointSort() );
		spiral[0].sector.wStartAngle = 0;
		spiral[0].sector.wEndAngle = 65535;
		int nStartCell = 1;			// first cell with current radius
		spiralCoords[nMaxRadius][nMaxRadius] = 0;
		for ( int i = 1; i < spiral.size(); ++i )
		{
			spiralCoords[ spiral[i].vOffset.y + nMaxRadius ][ spiral[i].vOffset.x + nMaxRadius ] = i;
			if ( i == spiral.size()-1 || spiral[i+1].nRadius > spiral[i].nRadius )
			{
				int nAngle = ( spiral[nStartCell].sector.wStartAngle - ( 65535 - spiral[i].sector.wEndAngle ) ) / 2;
				if ( nAngle < 0 )
					nAngle += 65535;
				spiral[i].sector.wEndAngle = nAngle;
				spiral[nStartCell].sector.wStartAngle = nAngle;
				nStartCell = i+1;
			}
			else
			{
				const uint16_t wAngle = ( spiral[i].sector.wEndAngle + spiral[i+1].sector.wStartAngle ) / 2;
				spiral[i].sector.wEndAngle = wAngle;
				spiral[i+1].sector.wStartAngle = wAngle;
			}
		}
	}
}


