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
	bool ProcessEvent( const struct SGameMessage &msg ) override;
	void Step( bool bAppActive ) override;
};
