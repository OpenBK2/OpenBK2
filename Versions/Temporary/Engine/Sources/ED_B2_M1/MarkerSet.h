#if !defined(__MARKER_SET__)
#define __MARKER_SET__
#pragma once

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "EditorScene.h"

enum EDirMeasure
{
	DIR_IN_DEGREES,
	DIR_IN_RADIAN,
	DIR_IN_AIGRAD
};

//						MARKER SET

struct SMarkerPoint
{
	CVec3 pos;
	float fDir;

	SMarkerPoint() :
		pos( VNULL3 ),
		fDir( 0 )
	{
	}
};

struct SMarkerSet : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SMarkerSet );

	void AttachToScene( bool bActive, const vector<SMarkerPoint> &points );
public:
	SMarkerSet(){}
	//
	int xAxisID[2];
	int yAxisID[2];
	int zAxisID[2];
	int arrowID[2];
	//
	bool bIsInScene;
	//
	vector<SMarkerPoint> points;
	vector<SMarkerPoint> activePoints;
	CVec3 vBuildingPos;
	CVec2 vBuildingOrigin;
	//

	SMarkerSet( const CVec3 &rvBuildingPos, const CVec2 &rvBuildingOrigin );
	~SMarkerSet();
	//
	void AttachToScene();
	void DetachFromScene();
	//
	void AddMarker( const CVec3 &rPos, const float fDir, EDirMeasure eDirMeasure, 
					bool bActive, bool bUseBuildingPos, bool bUseOrigin );
	//
};

inline IEditorScene* GetEditorScene()
{
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "GetScene(): pScene == 0" );
	return pScene;
}

#endif // #if !defined(__MARKER_SET__)

