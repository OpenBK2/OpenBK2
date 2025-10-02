#include "StdAfx.h"
#include "../ui/commandparam.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "UIScene.hpp"
#include "../ED_Common/UIVisitor.hpp"

#include "EditorScene.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CUISceneB2::CUISceneB2()
{
}


CUISceneB2::~CUISceneB2()
{
}


void CUISceneB2::Create()
{
}


void CUISceneB2::Clear()
{
	EditorScene()->RemoveAllScreens();
}


void CUISceneB2::AddWindow( IWindow *_pWindow )
{
	EditorScene()->AddScreen( _pWindow );
}


void CUISceneB2::RemoveWindow( IWindow *_pWindow )
{
	EditorScene()->RemoveScreen( _pWindow );
}


void CUISceneB2::Draw()
{
	// ui draw already implemented in B2-scene
}


NGScene::I2DGameView *CUISceneB2::GetG2DView()
{
	return EditorScene()->GetG2DView();
}


IUIScene *CreateUIScene()
{
	IUIScene *pUIScene = new CUISceneB2;
	pUIScene->Create();
	return pUIScene;
}


