#pragma once

#include "ED_Common/UIScene.h"

class CUISceneB2 : public IUIScene
{
	OBJECT_NOCOPY_METHODS( CUISceneB2 )

	// members
public:
	CUISceneB2();
	~CUISceneB2() override;
	// IUIScene
	void Create() override;
	void Clear() override;
	void AddWindow( IWindow *pWindow ) override;
	void RemoveWindow( IWindow *pWindow ) override;
	void Draw() override;
	NGScene::I2DGameView *GetG2DView() override;
};
