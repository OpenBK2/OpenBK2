#if !defined(__CHILD_FRAME__CONTAINER__)
#define __CHILD_FRAME__CONTAINER__
#pragma once

#include "..\MapEditorLib\Interface_ChildFrame.h"

class CChildFrameContainer : public IChildFrameContainer
{
	OBJECT_NOCOPY_METHODS( CChildFrameContainer );
	CPtr<IChildFrame> pActiveChildFrame;
	string szActiveChildFrameTypeName;
	//
public:
	~CChildFrameContainer();

	// IChildFrameContainer
	bool CanCreate( const string &rszChildFrameTypeName );
	bool IsActive( const string &rszChildFrameTypeName );
	bool Create( const string &rszChildFrameTypeName );
	void Destroy();
	void Enter();
	void Leave();
};

#endif // !defined(__CHILD_FRAME__CONTAINER__)

