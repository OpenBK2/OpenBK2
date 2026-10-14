#pragma once

#include "RunModeInterfaceBase.h"

class CTextureInterface : public CRunModeInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CTextureInterface );

public:
	// life-cycle
	CTextureInterface();
	~CTextureInterface() override;
};

INTERFACE_COMMAND_DECLARE( CTexturelIC, CTextureInterface )
