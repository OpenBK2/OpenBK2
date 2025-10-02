#include "StdAfx.h"
#include "SceneB2/Camera.h"
#include "Stats_B2_M1/Vis2AI.h"

CVec3 CorrectPosByCameraAndHeight( const CVec3 &vCenter )
{
	CVec3 vVisPos = vCenter;

	//Now compensate for the different height
	CVec3 vOldAnchor = Camera()->GetAnchor();
	CVec3 vLook = vOldAnchor - Camera()->GetPos();
	Vis2AI( &vOldAnchor );
	Vis2AI( &vLook );
	if ( vLook.z != 0.0f ) 
	{
		float fCoeff = vLook.x / vLook.z;
		vVisPos.x += ( vOldAnchor.z - vCenter.z ) * fCoeff;

		fCoeff = vLook.y / vLook.z;
		vVisPos.y += ( vOldAnchor.z - vCenter.z ) * fCoeff;

		vVisPos.z = vOldAnchor.z;
	}

	return vVisPos;
}


