
#pragma once

#include "InterfaceScreenBase.h"

class CDummyInterface : public CInterfaceScreenBase
{
	OBJECT_NOCOPY_METHODS( CDummyInterface );
public:	
	CDummyInterface();
	virtual ~CDummyInterface();
	//
};

INTERFACE_COMMAND_DECLARE( CICDummy, CDummyInterface );


