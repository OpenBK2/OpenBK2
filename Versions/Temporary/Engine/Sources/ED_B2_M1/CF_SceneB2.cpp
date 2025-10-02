#include "stdafx.h"

#include "../MapEditorLib/ChildFrameFactory.h"
#include "CF_SceneB2.h"
#include "CFC_SceneB2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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


