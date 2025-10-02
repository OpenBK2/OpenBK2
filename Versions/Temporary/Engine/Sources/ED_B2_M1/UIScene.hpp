#if !defined(__UI_SCENE_B2__)
#define __UI_SCENE_B2__
#pragma once


#include "../ED_Common/UIScene.h"

class CUISceneB2 : public IUIScene
{
	OBJECT_NOCOPY_METHODS( CUISceneB2 )

	// members
public:
	CUISceneB2();
	virtual ~CUISceneB2();
	// IUIScene
	void Create();
	void Clear();
	void AddWindow( IWindow *pWindow );
	void RemoveWindow( IWindow *pWindow );
	void Draw();
	NGScene::I2DGameView *GetG2DView();
};

#endif // !defined(__UI_SCENE_B2__)


