#pragma once

#include "MapEditorLib/Interface_ChildFrame.h"

class CChildFrameContainer : public IChildFrameContainer
{
	OBJECT_NOCOPY_METHODS( CChildFrameContainer );
	CPtr<IChildFrame> pActiveChildFrame;
	std::string szActiveChildFrameTypeName;
	//
public:
	~CChildFrameContainer() override;

	// IChildFrameContainer
	bool CanCreate( const std::string &rszChildFrameTypeName ) override;
	bool IsActive( const std::string &rszChildFrameTypeName ) override;
	bool Create( const std::string &rszChildFrameTypeName ) override;
	void Destroy() override;
	void Enter() override;
	void Leave() override;
};
