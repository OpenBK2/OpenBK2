#pragma once

#include "PathFractionComplex.h"
#include "PlanePathMath.h"

//	CPathFractionArc

// horisontal manuver
class CPathFractionArc : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPathFractionArc )
	ZDATA_(CPathFractionComplexBase)
	CDirectedCircle circle;								// initial circle
	CVec2 x0, x1;												// from point 1 to 2
	float fLength;
	float fZ;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&circle); f.Add(3,&x0); f.Add(4,&x1); f.Add(5,&fLength); f.Add(6,&fZ); return 0; }

public:

	void Init( const CDirectedCircle &_circle, const CVec2 &_x0, const CVec2 &_x1, const float _fZ );

	virtual CVec3 GetPoint( const float fDist ) const;
	virtual CVec3 GetTangent( const float fDist ) const;
	virtual CVec3 GetNormale( const float fDist ) const;
	virtual float GetLength() const 
	{ 
		return fLength; 
	}
	
	virtual void GetSimplePath( CPathList *pPaths ) { NI_ASSERT( false, "wrong call" ); }
};

