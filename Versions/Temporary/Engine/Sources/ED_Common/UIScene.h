#if !defined(__UI_SCENE__)
#define __UI_SCENE__
#pragma once



namespace NGScene
{
	class I2DGameView;
};
interface IWindow;

interface IUIScene : public CObjectBase
{
	enum { tidTypeID = 0x160B8D80 };
	virtual void Create() = 0;
	virtual void Clear() = 0;
	virtual void AddWindow( IWindow *pWindow ) = 0;
	virtual void RemoveWindow( IWindow *pWindow ) = 0;
	virtual void Draw() = 0;
	virtual NGScene::I2DGameView *GetG2DView() = 0;
};

IUIScene *CreateUIScene();

#endif // !defined(__UI_SCENE__)


