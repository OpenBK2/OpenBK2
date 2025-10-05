#pragma once
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
	std::vector<SInterval> &intersections;
	//
	SRefTriangle GetTriangle( const SConvexHull &e, const SHMatrix &pos, const STriangle &t );
	SInterval::SCrossPoint CalcCross( const SRefTriangle &t );
	void TraceEntity( const SConvexHull &e, std::vector<SInterval::SCrossPoint> *pEnter, std::vector<SInterval::SCrossPoint> *pExit );
		
public:
	CTracer( std::vector<SInterval> &_intersections ): intersections(_intersections) {}
	bool TestSphere( const CVec3 &ptCenter, float fR );
	const CVec3& GetDir() const { return ptDir; }
	const CVec3& GetOrigin() const { return ptOrig; }
	void InitProjection( const CRay &r );
	void InitProjection( const CVec3 &ptFrom, const CVec3 &ptDir );
	void TraceEntity( const std::vector<SConvexHull> &e, bool bTerrain );
	void TraceEntity( const SConvexHull &e, bool bTerrain );
};

}

