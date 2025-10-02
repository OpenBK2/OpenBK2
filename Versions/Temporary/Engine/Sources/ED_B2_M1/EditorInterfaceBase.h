#pragma once

#include "ED_Common/GameInputInterface.h"
struct ICamera;

class CEditorInterfaceBase : public CGameInputInterface
{
public:	
	CEditorInterfaceBase();
	//
	virtual void ToggleCameraControl( const SGameMessage &msg, bool bEnableCameraControl );
	// IInterfaceBase
	virtual bool ProcessEvent( const struct SGameMessage &msg );
	virtual void Step( bool bAppActive );
};


