#pragma once

#include "CameraBasicMouseMutator.h"

namespace NCamera
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CCameraGameMouseMutator : public CCameraBasicMouseMutator
	{
		OBJECT_NOCOPY_METHODS( CCameraGameMouseMutator )
		// Transient mouse input: never resume an unfinished zoom after loading a save.
		float fZoomTargetDistance = 0.0f;
		bool bSmoothZoomActive = false;
		//
		float GetPitchDelta();
		float GetYawDelta();
		float GetForwardDelta();
		float GetStrafeDelta();
		float GetZoomDistance( float fTimeDiff, float fRealSensetivity );
	public:
		CCameraGameMouseMutator();
		void SetDistance( float fDistance ) override;
		bool NeedUpdate() { return true; }
		void Recalc();

		int operator&( IBinSaver &saver );
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


