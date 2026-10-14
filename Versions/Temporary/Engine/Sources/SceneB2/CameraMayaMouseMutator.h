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
	bool NeedUpdate() override
	{
		return true;
	}

	void Recalc() override;

	int operator&( IBinSaver &saver ) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace NCamera
