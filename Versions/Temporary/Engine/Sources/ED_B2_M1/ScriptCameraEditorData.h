#pragma once

#include "Stats_B2_M1/CameraRunTypes.h"
#include "SceneB2/CameraInternal.h"

//
//
//			SCRIPT CAMERA WINDOW DATA STRUCT
//
//

struct SScriptCameraWindowData
{
	vector<NCamera::CCameraPlacement> scriptCameras;
	int nCurrentCamera;
	NDb::EScriptCameraRunType eRunType;

	enum EScriptCameraLastAction
	{
		SCA_UNKNOWN,
		SCA_NO_ACTIONS,
		SCA_CAMERA_ADD,
		SCA_CAMERA_SAVE,
		SCA_CAMERA_DELETE,
		SCA_CAMERA_CHANGE,
		SCA_CAMERA_JUMP,
		SCA_CAMERA_RUN
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

//
//	Dialog data structure
//

struct SScriptCameraRunDlgData
{
	vector<NCamera::CCameraPlacement> scriptCameras;
	NDb::EScriptCameraRunType eRunType;
	int nStartCamera;
	int nFinishCamera;
	float fTime;
	float fLSpeed;
	float fASpeed;
	int nTargetScriptID;
	float fSpline1;
	float fSpline2;

	SScriptCameraRunDlgData()
	{
		Clear();
	}
	//
	void Clear()
	{
		scriptCameras.clear();
		eRunType = NDb::SCRT_DIRECT_MOVE;
		nStartCamera = -1;
		nFinishCamera = -1;
		fTime = 0;
		fLSpeed = 0;
		fASpeed = 0;
		nTargetScriptID = -1;
		fSpline1 = 0;
		fSpline2 = 0;
	}
	//
	const NCamera::CCameraPlacement &GetScriptCamera( int nCamera )
	{
		//NI_ASSERT( IsValidCamera(nCamera), "Script Cameras out of range call\n" );
		if ( IsValidCamera(nCamera) )
			return scriptCameras[nCamera];
		else
			return scriptCameras[0];
	}
	//
	const NCamera::CCameraPlacement &GetStartCamera()
	{
		return GetScriptCamera( nStartCamera );
	}
	//
	const NCamera::CCameraPlacement &GetFinishCamera()
	{
		return GetScriptCamera( nFinishCamera );
	}
	//
	bool IsValidCamera( int nCamera )
	{
		return  (nCamera >= 0) && (nCamera < scriptCameras.size());
	}
};



