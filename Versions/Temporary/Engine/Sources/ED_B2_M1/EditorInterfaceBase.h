#if !defined(__CAMERA_MOVEMENT_INTERFACE__)
#define __CAMERA_MOVEMENT_INTERFACE__
#pragma once

#include "..\ED_Common\GameInputInterface.h"
interface ICamera;

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

#endif // !defined(__CAMERA_MOVEMENT_INTERFACE__)

