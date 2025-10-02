#if !defined(__CHILD_FRAME_FACTORY__)
#define __CHILD_FRAME_FACTORY__
#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( ChildFrame );
#define REGISTER_CHILD_FRAME_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( ChildFrame, name, classname )
#define REGISTER_CHILD_FRAME_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( ChildFrame, name, classname )

#endif // #if !defined(__CHILD_FRAME_FACTORY__)

