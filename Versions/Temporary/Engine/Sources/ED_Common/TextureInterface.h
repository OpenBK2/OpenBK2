#ifndef __TEXTURE_RUN_MODE_INTERFACE__
#define __TEXTURE_RUN_MODE_INTERFACE__

#include "RunModeInterfaceBase.h"

class CTextureInterface : public CRunModeInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CTextureInterface );
public:	
	// life-cycle
	CTextureInterface();
	virtual ~CTextureInterface();
};

INTERFACE_COMMAND_DECLARE( CTexturelIC, CTextureInterface )

#endif // __TEXTURE_RUN_MODE_INTERFACE__

