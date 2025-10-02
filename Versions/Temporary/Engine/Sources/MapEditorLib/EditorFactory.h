#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( Editor );
#define REGISTER_EDITOR_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( Editor, name, classname )
#define REGISTER_EDITOR_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( Editor, name, classname )


