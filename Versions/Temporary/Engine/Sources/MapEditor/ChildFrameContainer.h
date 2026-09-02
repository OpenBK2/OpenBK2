#pragma once

#include "MapEditorLib/Interface_ChildFrame.h"

class CChildFrameContainer : public IChildFrameContainer
{
	OBJECT_NOCOPY_METHODS( CChildFrameContainer );
	CPtr<IChildFrame> pActiveChildFrame;
	std::string szActiveChildFrameTypeName;
	//
public:
	~CChildFrameContainer();

	// IChildFrameContainer
	bool CanCreate( const std::string &rszChildFrameTypeName );
	bool IsActive( const std::string &rszChildFrameTypeName );
	bool Create( const std::string &rszChildFrameTypeName );
	void Destroy();
	void Enter();
	void Leave();
};



