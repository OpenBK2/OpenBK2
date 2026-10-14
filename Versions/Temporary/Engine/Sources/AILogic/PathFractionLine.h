#pragma once

#include "PlanePathFraction.h"

//	CPathFractionLine

class CPathFractionLine : public IPathFraction
{
	OBJECT_BASIC_METHODS( CPathFractionLine )
	ZDATA
	CVec3 x0, x1;
	CVec3 v0;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fLength = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &x0 );
		f.Add( 3, &x1 );
		f.Add( 4, &v0 );
		f.Add( 5, &fLength );
		return 0;
	}

public:
	CPathFractionLine()
		: fLength( 0 )
	{
	}

	// FOR 2D PATH
	void Init( const CVec2 &_x0, const CVec2 &_x1, const float _fZ );
	void Init( const CVec3 &_x0, const CVec3 &_x1 );

	float GetLength() const override
	{
		return fLength;
	}

	CVec3 GetPoint( const float fDist ) const override
	{
		return x0 + v0 * fDist;
	}

	CVec3 GetTangent( const float fDist ) const override
	{
		return v0;
	}

	CVec3 GetNormale( const float fDist ) const override
	{
		const CVec3 vXY = CVec3( -v0.y, v0.x, 0 );
		CVec3 vNorm( v0 ^ vXY );
		//if ( vNorm.z == 0.0f )
		//return V3_AXIS_Z;
		Normalize( &vNorm );
		return vNorm;
	}

	CVec3 GetStartPoint() const override
	{
		return x0;
	}

	CVec3 GetStartTangent() const override
	{
		return v0;
	}

	CVec3 GetEndPoint() const override
	{
		return x1;
	}

	CVec3 GetEndTangent() const override
	{
		return v0;
	}

	void GetSimplePath( CPathList *pPaths ) override
	{
		NI_ASSERT( false, "wrong call" );
	}
};
