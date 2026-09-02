#pragma once

#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "EditorScene.h"

#include <zconf.h>

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

	void AttachToScene( bool bActive, const std::vector<SMarkerPoint> &points );
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
	std::vector<SMarkerPoint> points;
	std::vector<SMarkerPoint> activePoints;
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


