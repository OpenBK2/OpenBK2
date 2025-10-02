#pragma once
#include "Main/MainLoop.h"


class CInterfaceCommandDeclare : public IInterfaceCommand
{
	CPtr<IInterfaceBase> pInterface;

public:
	CInterfaceCommandDeclare( IInterfaceBase *_pInterface ) : pInterface( _pInterface ) {}
	void Exec()
	{
		NMainLoop::ResetStack();
		NMainLoop::PushInterface( pInterface );
	}
};


#define INTERFACE_COMMAND_DECLARE( CommandClass, InterfaceClass )					\
class CommandClass : public CInterfaceCommandDeclare											\
{																																					\
OBJECT_NOCOPY_METHODS( CommandClass );																		\
public:																																		\
	CommandClass() : CInterfaceCommandDeclare( 0 )	{}											\
	CommandClass( InterfaceClass * pI ) : CInterfaceCommandDeclare( pI ) {}	\
};


