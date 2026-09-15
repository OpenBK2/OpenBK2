#pragma once

#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/Interface_Widget.h"

#include "ED_Common_export.h"

class CChildFrameWndBase;
struct ISceneSurface;

class ED_COMMON_EXPORT CChildFrameBase : public IChildFrame
{
	// The viewport's window, made with the document window and deleted after
	// it: see SceneSurface.h.
	ISceneSurface *pSurface;

	void DeleteSurface();

protected:
	IFrameWindow *pwndChildFrame;
	// The viewport, made and deleted by the derived child frame.
	CChildFrameWndBase *pChildWnd;

public:
	CChildFrameBase();
	virtual ~CChildFrameBase();

	//IChildFrame interface
	virtual bool Create();
	virtual void Destroy();
	virtual void Enter();
	virtual void Leave();
};


