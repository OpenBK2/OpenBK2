#include "StdAfx.h"

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "Tools_SceneGeometry.h"
#include "../MapEditorLib/Tools_Geometry.h"
#include "../SceneB2/Camera.h"
#include "EditorScene.h"
#include "../stats_b2_m1/Vis2AI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


float GetDistanceTo3DLine( const CVec3 &rvPoint, CVec2 &rvScreenPos )
{
	CVec3 vOrigin = VNULL3;
	CVec3 vDirection = VNULL3;
	Camera()->GetProjectiveRay( &vOrigin, &vDirection, rvScreenPos );
	Vis2AI( &vOrigin );
	//Vis2AI( &vDirection );
	return GetDistanceTo3DLine( rvPoint, vOrigin, vDirection );
}


void Get2DPosOnMapHeights( CVec3 *pvPos, const CVec2 &rvScreenPos )
{
	//NI_ASSERT( ( EditorScene() != 0 ) && ( EditorScene()->GetTerrain() != 0 ), "Get2DPosOnMapHeights(), Terrain not loaded" );
	if ( pvPos )
	{
		CVec3 vNear, vFar;
		Camera()->GetProjectiveRayPoints( &vNear, &vFar, rvScreenPos );
		Vis2AI( &vNear );
		Vis2AI( &vFar );
		if ( !EditorScene()->GetIntersectionWithTerrainForEditor( pvPos, vNear, vFar ) )
		{
			( *pvPos ) == VNULL3;
		}
		else
		{
			pvPos->z = 0.0f;
		}
	}
}


void Get3DPosOnMapHeights( CVec3 *pvPos, const CVec2 &rvScreenPos )
{
	Get2DPosOnMapHeights( pvPos, rvScreenPos );
	UpdateTerrainHeight( pvPos );
}


float GetTerrainHeight( float x, float y )
{
	return EditorScene()->GetZ( x, y );
}


void UpdateTerrainHeight( CVec3 *pvPos )
{
	EditorScene()->UpdateZ( pvPos );
}

//
//		CAMERA UTILS
//

void GetCameraPosition( CVec3 *pvCamAnchor )
{
	*pvCamAnchor = Camera()->GetAnchor();
}

void SetCameraPosition( const CVec3 &rvCamAnchor )
{
	CObj<ICamera> pCamera = Camera();
	CVec3 vCamNewPos = rvCamAnchor;
	AI2Vis( &vCamNewPos );
	pCamera->SetAnchor( vCamNewPos );
}

// basement storage  


