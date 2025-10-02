#pragma once

#include "PathFractionComplex.h"
class CPathFractionArcLine3D;

//	CPahtFractionArcLineArc3D

// path is suitable if 
// 1) direction change is small ( < Pi/2 )
// 2) direction difference of v0 and x1-x0 is small
// 3) distance is long ( > 2R )
class CPahtFractionArcLineArc3D : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPahtFractionArcLineArc3D )

	ZDATA_(CPathFractionComplexBase)
	CPtr<CPathFractionArcLine3D> pStart;
	CPtr<CPathFractionArcLine3D> pFinish;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&pStart); f.Add(3,&pFinish); return 0; }

public:

	void Init( const SPrevPathParams &prevPath, const CVec3 &x1, const CVec3 &v1, const float fR1, const float fR2 );

	void Init( const CVec3 &x0, const CVec3 &x1,			// coordinates
		const CVec3 &v0, const CVec3 &v1,			// directions
		const float r0, const float r1 );			// turn radii
	virtual void GetSimplePath( CPathList *pPaths );
};

