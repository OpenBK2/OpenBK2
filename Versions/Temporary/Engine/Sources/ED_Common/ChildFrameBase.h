#pragma once

#include "MapEditorLib/Interface_ChildFrame.h"

class CChildFrameBase : public IChildFrame
{
protected:
	SECWorksheet *pwndChildFrame;
	CWnd *pChildWnd;

public:
	CChildFrameBase();
	virtual ~CChildFrameBase();

	//IChildFrame interface
	virtual bool Create();
	virtual void Destroy();
	virtual void Enter();
	virtual void Leave();
};


