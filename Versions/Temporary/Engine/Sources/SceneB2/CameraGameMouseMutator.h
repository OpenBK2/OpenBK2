#pragma once

#include "CameraBasicMouseMutator.h"

namespace NCamera
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CCameraGameMouseMutator : public CCameraBasicMouseMutator
	{
		OBJECT_NOCOPY_METHODS( CCameraGameMouseMutator )
		//
		float GetPitchDelta();
		float GetYawDelta();
		float GetForwardDelta();
		float GetStrafeDelta();
		float GetZoomDelta();
	public:
		bool NeedUpdate() { return true; }
		void Recalc();

		int operator&( IBinSaver &saver );
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


