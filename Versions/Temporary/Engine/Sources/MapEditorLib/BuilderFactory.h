#if !defined(__BUILDER_FACTORY__)
#define __BUILDER_FACTORY__
#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( Builder );
#define REGISTER_BUILDER_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( Builder, name, classname )
#define REGISTER_BUILDER_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( Builder, name, classname )

#endif // #if !defined(__BUILDER_FACTORY__)
