#pragma once

// Stats_B2_M1/CameraRunTypes.h is no longer included: NDb::EScriptCameraRunType
// was only here for the run dialog's data, and that dialog is gone. The game
// still uses the type, through AILogic's SCRunTime and SCRunSpeed.
#include "SceneB2/CameraInternal.h"

//
//
//			SCRIPT CAMERA WINDOW DATA STRUCT
//
//

struct SScriptCameraWindowData
{
	std::vector<NCamera::CCameraPlacement> scriptCameras;
	int nCurrentCamera;

	// SCA_CAMERA_RUN was last, and went with the run dialog it opened; nothing
	// earlier moved. These are never saved, but the order is kept anyway.
	enum EScriptCameraLastAction
	{
		SCA_UNKNOWN,
		SCA_NO_ACTIONS,
		SCA_CAMERA_ADD,
		SCA_CAMERA_SAVE,
		SCA_CAMERA_DELETE,
		SCA_CAMERA_CHANGE,
		SCA_CAMERA_JUMP,
	};
	EScriptCameraLastAction eLastAction;

	SScriptCameraWindowData()
		: nCurrentCamera( -1 ),
		eLastAction( SCA_NO_ACTIONS )
	{
	}
	void Clear()
	{
		scriptCameras.clear();
		nCurrentCamera = -1;
		eLastAction = SCA_NO_ACTIONS;
	}
};



