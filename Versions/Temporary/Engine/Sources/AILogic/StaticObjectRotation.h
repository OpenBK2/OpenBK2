#pragma once


template <class TCont, class TArray2D>
class CRotatedArray2D
{
	TArray2D &array;
	int nRotation;

	int operator&( IBinSaver & saver ) { return 0; } // dissalow serialize
public:

	CRotatedArray2D( TArray2D &_array, const int _nRotation )
		: array( _array ), nRotation( _nRotation )
	{
	}

	int GetMinX() const 
	{
		switch( nRotation )
		{
		case 0: return 0;
		case 1: return -(array.GetSizeY() - 1);
		case 2: return -(array.GetSizeX() - 1 );
		case 3: return 0;
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return 0;
	}

	int GetMinY() const
	{
		switch( nRotation )
		{
		case 0: return 0;
		case 1: return 0;
		case 2: return -(array.GetSizeY() - 1);
		case 3: return -(array.GetSizeX() - 1);
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return 0;
	}

	int GetMaxX() const
	{
		switch( nRotation )
		{
		case 0: return (array.GetSizeX()-1);
		case 1: return 0;
		case 2: return 0;
		case 3: return (array.GetSizeY()-1);
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return 0;
	}

	int GetMaxY() const
	{
		switch( nRotation )
		{
		case 0: return (array.GetSizeY()-1);
		case 1: return (array.GetSizeX()-1);
		case 2: return 0;
		case 3: return 0;
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return 0;
	}

	const TCont & Get( const int nX, const int nY ) const
	{
		switch( nRotation )
		{
		case 0: return array[nY][nX];
		case 1: return array[-nX][nY];
		case 2: return array[-nY][-nX];
		case 3: return array[nX][-nY];
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return array[0][0];
	}

	TCont & Get( const int nX, const int nY ) 
	{
		switch( nRotation )
		{
		case 0: return array[nY][nX];
		case 1: return array[-nX][nY];
		case 2: return array[-nY][-nX];
		case 3: return array[nX][-nY];
		}
		NI_ASSERT( false, StrFmt( "wrong rotation %i", nRotation ) );
		return array[0][0];
	}
	int GetSizeX() const 
	{ 
		if ( nRotation == 0 || nRotation == 2 )
			return array.GetSizeX(); 
		return array.GetSizeY();
	}
	int GetSizeY() const 
	{ 
		if ( nRotation == 0 || nRotation == 2 )
			return array.GetSizeY(); 
		return array.GetSizeX();
	}
};

class CRotatedVec2 : public CVec2
{
public:
	CRotatedVec2( const CVec2 &_vOrigin, const WORD _wAngle )
		: CVec2( CVec2(_vOrigin.y, -_vOrigin.x) ^ GetVectorByDirection( _wAngle ) )
	{
	}
};

typedef CRotatedArray2D<BYTE, const CArray2D<BYTE> > CRotatedArray2DBYTEConst;
typedef CRotatedArray2D<BYTE, CArray2D<BYTE> > CRotatedArray2DBYTE;


template <class TCont, class TArray2D>
class CSmoothRotatedArray2D
{
	TArray2D *array;						// passability array
	SAIAngle wAngle;								// object rotation ange
	CVec2 vCenter;							// object center (int AI world coordinates)
	CVec2 vOrigin;							// passability origin
	CVec2 vRotationVector, vRotationVectorBack;
	int nMinX, nMinY, nMaxX, nMaxY;
	float fSin, fCos;
public:
	// array - passability/visibility
	CSmoothRotatedArray2D() { }
	void Clear()
	{
		*array

	}
	CSmoothRotatedArray2D( TArray2D &_array, const WORD _wAngle, const CVec2 &_vOrigin, const CVec2 &_vCenter )
	{
		Init( _array, _wAngle, _vOrigin, _vCenter );
	}
	void Init( TArray2D &_array, const WORD _wAngle, const CVec2 &_vOrigin, const CVec2 &_vCenter )
	{
		array = &_array;
		wAngle = _wAngle;
		vCenter = _vCenter;
		vOrigin = _vOrigin;
	
		vRotationVector = CVec2( NMath::Cos( 2 * PI * (_wAngle) / 65536.0f ), NMath::Sin( 2 * PI * (_wAngle) / 65536.0f ) );
		vRotationVectorBack = CVec2( NMath::Cos( 2 * PI * (65536 - _wAngle) / 65536.0f ), NMath::Sin( 2 * PI * ( 65536 - _wAngle) / 65536.0f ) );
		std::vector<CVec2> vVertices;
		
		CVec2 vPt = -vOrigin;
		vVertices.push_back( ( vPt ^ vRotationVector) + vCenter );
		
		vPt = CVec2(-vOrigin.x + ( array->GetSizeX() ) * SConsts::TILE_SIZE, -vOrigin.y );
		vVertices.push_back( ( vPt ^ vRotationVector ) + vCenter );

		vPt = CVec2(-vOrigin.x, -vOrigin.y + ( array->GetSizeY() ) * SConsts::TILE_SIZE);
		vVertices.push_back( ( vPt ^ vRotationVector ) + vCenter );
		
		vPt = CVec2(-vOrigin.x + ( array->GetSizeX() ) * SConsts::TILE_SIZE, -vOrigin.y + ( array->GetSizeY() ) * SConsts::TILE_SIZE );
		vVertices.push_back( ( vPt ^ vRotationVector ) + vCenter );

		nMinX = nMaxX = vVertices[0].x;
		nMinY = nMaxY = vVertices[0].y;
		for ( int i = 1; i < vVertices.size(); ++i )
		{
			nMinX = nMinX > vVertices[i].x ? vVertices[i].x : nMinX;
			nMaxX = nMaxX < vVertices[i].x ? vVertices[i].x : nMaxX;

			nMinY = nMinY > vVertices[i].y ? vVertices[i].y : nMinY;
			nMaxY = nMaxY < vVertices[i].y ? vVertices[i].y : nMaxY;
		}
	}

	// get Min-Max X-Y return AI wolrd points
	int GetMinX() const { return nMinX; }
	int GetMinY() const { return nMinY; }
	int GetMaxX() const { return nMaxX; }
	int GetMaxY() const { return nMaxY; }

	// point is in AI world coordinates
	TCont GetVal( const CVec2 &vPoint ) const
	{
		const CVec2 vRelativePoint( vPoint - vCenter );
		const CVec2 vRotatedPoint( vRelativePoint ^ vRotationVectorBack );
		const CVec2 vPassabilityPoint( vRotatedPoint + vOrigin );
		const SVector vP( vPassabilityPoint.x / SConsts::TILE_SIZE, vPassabilityPoint.y / SConsts::TILE_SIZE );
		if ( vP.x >= 0 && vP.y >= 0 && vP.x < array->GetSizeX()&& vP.y < array->GetSizeY() )
			return (*array)[vP.y][vP.x];
		return TCont(0);
	}
};


