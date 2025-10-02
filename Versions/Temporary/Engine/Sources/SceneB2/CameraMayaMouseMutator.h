#pragma once

#include "CameraBasicMouseMutator.h"

namespace NCamera
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CCameraMayaMouseMutator : public CCameraBasicMouseMutator
	{
		OBJECT_NOCOPY_METHODS( CCameraMayaMouseMutator )
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


