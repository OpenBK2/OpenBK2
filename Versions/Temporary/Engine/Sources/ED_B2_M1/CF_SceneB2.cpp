#include "stdafx.h"

#include "MapEditorLib/ChildFrameFactory.h"
#include "CF_SceneB2.h"
#include "CFC_SceneB2.h"

REGISTER_CHILD_FRAME_IN_DLL( __CHILD_FRAME_DX_SCENE_LABEL__, CCFSceneB2 )


CCFSceneB2::CCFSceneB2()
{
	pChildWnd = new CCFCSceneB2;
}


CCFSceneB2::~CCFSceneB2()
{
	delete pChildWnd;
}


// basement storage  


