#if !defined( __TRACE_MODEL__ )
#define __TRACE_MODEL__
#pragma once

struct STriangleForTrace
{
	CVec3 vertices[3];
	string szBodyPart;
	//
	STriangleForTrace()
	{
		vertices[0] = VNULL3;
		vertices[1] = VNULL3;
		vertices[2] = VNULL3;
	}
};

struct SModelSurfacePoint
{
	CVec3 vPos;
	CVec3 vNormal;
	string szBodyPart;
	//
	SModelSurfacePoint() :
		vPos( VNULL3 ),
		vNormal( VNULL3 )
	{
	}
};

struct granny_file_info;
int TraceModel( vector<SModelSurfacePoint> *pSurfacePoints, granny_file_info *pFile );
bool TraceModel( vector<SModelSurfacePoint> *pSurfacePoints, const string &rszGeometryResourceName );
//////////////////////////////////////////////////////////////////////////////////////`////////////////////////////////////////
#endif // #if !defined( __TRACE_MODEL__ )

