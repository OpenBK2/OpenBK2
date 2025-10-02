#ifndef __DUMMY_INTERFACE_H__
#define __DUMMY_INTERFACE_H__

#pragma ONCE

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

#endif // __DUMMY_INTERFACE_H__

