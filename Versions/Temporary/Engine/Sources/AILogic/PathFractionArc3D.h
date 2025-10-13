#pragma once

#include "PathFractionComplex.h"
#include "PlanePathMath.h"

#include <cstdint>

//	CPathFractionArc3D

class CPathFractionArc3D : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPathFractionArc3D );
	ZDATA_(CPathFractionComplexBase)
	CVec3 i, j, k;													// local coordinate system ( i = v0, j = R - x0, k = i^j )
	CDirectedCircle circle;									// in (i,j,k) coordinate system (center = (0,R) )

	CVec3 x0;																// begin of arc fraction
	CVec3 x1;																// end of arc path fraction and start of line fraction
	float fLength;
	bool bNegative;
	uint16_t nDiff;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&i); f.Add(3,&j); f.Add(4,&k); f.Add(5,&circle); f.Add(6,&x0); f.Add(7,&x1); f.Add(8,&fLength); f.Add(9,&bNegative); f.Add(10,&nDiff); return 0; }
public:
	CPathFractionArc3D() : fLength( 0 ) { }
	
	void Init( const CVec3 &_i, const CVec3 &_j, const CVec3 &_k,
		const CDirectedCircle &_circle, const CVec3 &_x0, const CVec3 &_x1, const float _fLength,
		const uint16_t _nDiff );

	CVec3 GetPoint( const float fDist ) const;
	CVec3 GetTangent( const float fDist ) const;
	CVec3 GetNormale( const float fDist ) const;

	float GetLength() const { return fLength; }
	CVec3 GetEndPoint() const;
	CVec3 GetStartPoint() const;
	CVec3 GetEndTangent() const;
	CVec3 GetStartTangent() const;
	
	void GetSimplePath( CPathList *pPaths );
	void Negate() { bNegative = true; }
};

