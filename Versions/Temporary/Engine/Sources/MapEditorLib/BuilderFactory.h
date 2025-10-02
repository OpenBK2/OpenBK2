#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( Builder );
#define REGISTER_BUILDER_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( Builder, name, classname )
#define REGISTER_BUILDER_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( Builder, name, classname )


