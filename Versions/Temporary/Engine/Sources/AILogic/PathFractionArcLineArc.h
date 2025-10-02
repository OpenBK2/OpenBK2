#pragma once

#include "PathFractionComplex.h"

class CPathFractionArc;
class CPathFractionLine;
struct CDirectedCircle;

//	CPahtFractionArcLineArc

// s-shaped path (circle arc - line - circle arc )
class CPahtFractionArcLineArc : public CPathFractionComplexBase
{
	OBJECT_BASIC_METHODS( CPahtFractionArcLineArc )

	ZDATA_(CPathFractionComplexBase)
	CPtr<CPathFractionArc> pStart;
	CPtr<CPathFractionArc> pFinish;
	CPtr<CPathFractionLine>  pLine;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CPathFractionComplexBase*)this); f.Add(2,&pStart); f.Add(3,&pFinish); f.Add(4,&pLine); return 0; }

	void Init( const CDirectedCircle &_start, const CVec2 &_vStart1, const CVec2 &_vStart2,
		const CDirectedCircle &_finish, const CVec2 &_vFinish1, const CVec2 &_vFinish2, const float _fZ );

	void TryPath( const CVec2 &x0, const CVec2 &x1, 
		const CDirectedCircle &r1, const CDirectedCircle &o1, 
		CPahtFractionArcLineArc *pPath, CPahtFractionArcLineArc *pBest,
		bool *bInitted, const float _fZ ) const;

	void Init( const CVec2 &x0, const CVec2 &x1, const CVec2 &v0, const CVec2 &v1,
		const float fR0, const float fR1, const float _fZ );

public:

	void Init( const SPrevPathParams &prevPath, const CVec3 &x1, const CVec3 &v1, const float fR1, const float fR2 );
	void Init( const CVec3 &x0, const CVec3 &x1, const CVec3 &v0, const CVec3 &v1, const float fR0, const float fR1 );

	// from vStart2 to vFinish1 plane travels by line
	CPahtFractionArcLineArc() {  }

	virtual float GetLength() const;
	virtual void GetSimplePath( CPathList *pPaths );
};
