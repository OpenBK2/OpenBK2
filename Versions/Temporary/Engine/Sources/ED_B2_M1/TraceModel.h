#pragma once

struct STriangleForTrace
{
	CVec3 vertices[3];
	std::string szBodyPart;
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
	std::string szBodyPart;
	//
	SModelSurfacePoint() :
		vPos( VNULL3 ),
		vNormal( VNULL3 )
	{
	}
};

struct granny_file_info;
int TraceModel( std::vector<SModelSurfacePoint> *pSurfacePoints, granny_file_info *pFile );
bool TraceModel( std::vector<SModelSurfacePoint> *pSurfacePoints, const std::string &rszGeometryResourceName );
//////////////////////////////////////////////////////////////////////////////////////`////////////////////////////////////////


