#pragma once

#include "PathFractionComplex.h"
class CPathFractionArc3D;
class CPathFractionLine;

//	CPathFractionArcLine3D

// sircle arc - line path fraction (in 3D)
// v1 in x1 doesn't matter. the only thing is that whole maneuver is in 1 plane (v0, x1-x0)
class CPathFractionArcLine3D : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPathFractionArcLine3D )
		ZDATA_(CPathFractionComplexBase)
	CPtr<CPathFractionArc3D> pArc;
	CPtr<CPathFractionLine>  pLine;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&pArc); f.Add(3,&pLine); return 0; }
	bool TryCircle( const CVec3 &x0, const CVec3 &v0, const CVec3 &x1, const float fR, const int nDir, CVec3 *vT );
public:
	// creates arc and line path fractions if 3D (but manuver is flat)
	// nPathDirection = 1 => circle first, -1 => line is first
	void Init( const SPrevPathParams &prevPath, const CVec3 &x1, const float fR/*circle radius*/);
	void Init( const CVec3 &x0, const CVec3 &_v0, const CVec3 &x1, const float fR/*circle radius*/);

	virtual float GetLength() const;

	// access to inner path portions without substitute.
	CPathFractionArc3D * GetArc();
	IPathFraction * GetLine();
	virtual void GetSimplePath( CPathList *pPaths );
};

