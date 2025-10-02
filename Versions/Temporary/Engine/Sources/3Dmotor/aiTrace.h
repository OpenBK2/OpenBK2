#ifndef __aiTrace_H_
#define __aiTrace_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "aiObject.h"
#include "aiInterval.h"
namespace NAI
{

class CTracer
{
	struct SRefTriangle
	{
		const CVec3 a, a2, a3;
		//
		SRefTriangle( const CVec3 &_a, const CVec3 &_a2, const CVec3 &_a3 ): a(_a), a2(_a2), a3(_a3) {}
	};
	CVec4 ptAxis1, ptAxis2;
	CVec3 ptOrig, ptDir, ptDirNormalized;
	vector<SInterval> &intersections;
	//
	SRefTriangle GetTriangle( const SConvexHull &e, const SHMatrix &pos, const STriangle &t );
	SInterval::SCrossPoint CalcCross( const SRefTriangle &t );
	void TraceEntity( const SConvexHull &e, vector<SInterval::SCrossPoint> *pEnter, vector<SInterval::SCrossPoint> *pExit );
		
public:
	CTracer( vector<SInterval> &_intersections ): intersections(_intersections) {}
	bool TestSphere( const CVec3 &ptCenter, float fR );
	const CVec3& GetDir() const { return ptDir; }
	const CVec3& GetOrigin() const { return ptOrig; }
	void InitProjection( const CRay &r );
	void InitProjection( const CVec3 &ptFrom, const CVec3 &ptDir );
	void TraceEntity( const vector<SConvexHull> &e, bool bTerrain );
	void TraceEntity( const SConvexHull &e, bool bTerrain );
};

}
#endif

