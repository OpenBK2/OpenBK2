#include "stdafx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "EditorScene.h"
#include "MarkerSet.h"
#include "../Stats_B2_M1/Vis2AI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//						MARKER SET
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SMarkerSet::SMarkerSet(	const CVec3 &rvBuildingPos, const CVec2 &rvBuildingOrigin ) :
	bIsInScene( false ),
	vBuildingPos( rvBuildingPos ),
	vBuildingOrigin( rvBuildingOrigin )
{
	xAxisID[0] = -1;
	xAxisID[1] = -1;
	yAxisID[0] = -1;
	yAxisID[1] = -1;
	zAxisID[0] = -1;
	zAxisID[1] = -1;
	arrowID[0] = -1;
	arrowID[1] = -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SMarkerSet::~SMarkerSet()
{
	if ( bIsInScene )
		DetachFromScene();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SMarkerSet::AttachToScene( bool bActive, const vector<SMarkerPoint> &points )
{
	const CVec4 CLR_X( 0.5f,	0,		0,		1.0f );
	const CVec4 CLR_Y( 0,			0.5f,	0,		1.0f );
	const CVec4 CLR_Z( 0,			0,		0.5f,	1.0f );
	const CVec4 CLR_A( 0.5f,	0.5f,	0.5f,	1.0f );

	const CVec4 CLR_X_ACTIVE( 1.0f,		0,		0,		1.0f );
	const CVec4 CLR_Y_ACTIVE( 0,			1.0f,	0,		1.0f );
	const CVec4 CLR_Z_ACTIVE( 0,			0,		1.0f,	1.0f );
	const CVec4 CLR_A_ACTIVE( 1.0f,		1.0f,	1.0f,	1.0f );

	const float F_AXIS_SIZE = 20.0f;
	const float F_ARROW_SIZE = 50.0f;

	if ( points.empty() )
		return;

	IEditorScene *pScene = EditorScene();
	if ( !pScene )
		return;

	int nIdx = (bActive ? 1 : 0);
	int nBaseIdx = (bActive ? 0x1000 : 0x2000);
	
	CVec4 clrX = ( bActive ? CLR_X_ACTIVE : CLR_X );
	CVec4 clrY = ( bActive ? CLR_Y_ACTIVE : CLR_Y );
	CVec4 clrZ = ( bActive ? CLR_Z_ACTIVE : CLR_Z );
	CVec4 clrA = ( bActive ? CLR_A_ACTIVE : CLR_A );

	vector<CVec3> polyline;
	polyline.resize( points.size() * 2 );

	vector<WORD> indices;
	indices.resize( points.size() * 2 );

	int k = 0;
	for ( vector<SMarkerPoint>::const_iterator i = points.begin(); i != points.end(); ++i )
	{
		CVec3 v = i->pos;
		
		polyline[k] = v;
		polyline[k+1] = v + F_AXIS_SIZE * V3_AXIS_X; 

		indices[k] = k;
		indices[k+1] = k+1;

		k += 2;
	}
	xAxisID[nIdx] = pScene->AddIndexedPolyline( nBaseIdx + nIdx, polyline, indices, clrX, false );

	k = 0;
	for ( vector<SMarkerPoint>::const_iterator i = points.begin(); i != points.end(); ++i )
	{
		CVec3 v = i->pos;
		polyline[k] = v;
		polyline[k+1] = v + F_AXIS_SIZE * V3_AXIS_Y; 
		k += 2;
	}
	++nBaseIdx;
	yAxisID[nIdx] = pScene->AddIndexedPolyline( nBaseIdx + nIdx, polyline, indices, clrY, false );

	k = 0;
	for ( vector<SMarkerPoint>::const_iterator i = points.begin(); i != points.end(); ++i )
	{
		CVec3 v = i->pos;
		polyline[k] = v;
		polyline[k+1] = v + F_AXIS_SIZE * V3_AXIS_Z; 
		k += 2;
	}
	++nBaseIdx;
	zAxisID[nIdx] = pScene->AddIndexedPolyline( nBaseIdx + nIdx, polyline, indices, clrZ, false );

	k = 0;
	for ( vector<SMarkerPoint>::const_iterator i = points.begin(); i != points.end(); ++i )
	{
		CVec3 v = i->pos;
		polyline[k] = v;
		float fR = i->fDir;
		const float F_3PI2 = (3.0f * FP_PI2);
		if ( fR >= F_3PI2 )
			fR -= F_3PI2;
		else
			fR += FP_PI2;
		CVec3 dv( cos(fR), sin(fR), 0.01f );
		polyline[k+1] = v + dv * F_ARROW_SIZE;
		k += 2;
	}
	++nBaseIdx;
	arrowID[nIdx] = pScene->AddIndexedPolyline( ++nBaseIdx + nIdx, polyline, indices, clrA, false );

	bIsInScene = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SMarkerSet::AttachToScene()
{
	AttachToScene( false, points );
	AttachToScene( true, activePoints );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SMarkerSet::DetachFromScene()
{
	if ( !bIsInScene ) 
		return;

	IEditorScene *pScene = EditorScene();
	if ( !pScene )
		return;

	pScene->RemovePolyline( xAxisID[0] );
	pScene->RemovePolyline( yAxisID[0] );
	pScene->RemovePolyline( zAxisID[0] );
	pScene->RemovePolyline( arrowID[0] );

	pScene->RemovePolyline( xAxisID[1] );
	pScene->RemovePolyline( yAxisID[1] );
	pScene->RemovePolyline( zAxisID[1] );
	pScene->RemovePolyline( arrowID[1] );

	xAxisID[0] = -1;
	xAxisID[1] = -1;
	yAxisID[0] = -1;
	yAxisID[1] = -1;
	zAxisID[0] = -1;
	zAxisID[1] = -1;
	arrowID[0] = -1;
	arrowID[1] = -1;

	bIsInScene = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SMarkerSet::AddMarker( const CVec3 &rPos, 
													 const float fDir, 
													 EDirMeasure eDirMeasure, 							
													 bool bActive, 
													 bool bUseBuildingPos, 
													 bool bUseOrigin )
{
	IEditorScene *pScene = EditorScene();
	if ( !pScene )
		return;

	SMarkerPoint point;

	point.pos = rPos; // в AI координатах

	if ( bUseOrigin )
	{
		swap( point.pos.x, point.pos.y );
	}

	int nAITileX = point.pos.x / AI_TILE_SIZE;
	int nAITileY = point.pos.y / AI_TILE_SIZE;
	float fTileHeight = EditorScene()->GetTileHeight( nAITileX, nAITileY ); 

	point.pos.y += fTileHeight;
    
	if ( bUseBuildingPos )
	{
		point.pos += vBuildingPos;
	}

	if ( bUseOrigin )
	{
		const CVec2 origin = CVec2(vBuildingOrigin.y, -vBuildingOrigin.x) ^ CVec2( 0, 1 );
		point.pos -= CVec3( origin.x, origin.y, 0.0f );
	}

	switch ( eDirMeasure )
	{
	case DIR_IN_DEGREES:
		{
			const float F_GRAD_2_RAD = FP_2PI / 360.0f;
			point.fDir = F_GRAD_2_RAD *fDir;
		}
		break;
	case DIR_IN_RADIAN:
		{
			point.fDir = fDir;
		}
		break;
	case DIR_IN_AIGRAD:
		{
			point.fDir = AI2VisRad(fDir);
		}
		break;
	}

    if ( bActive )
		activePoints.push_back( point );
	else
		points.push_back( point );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
