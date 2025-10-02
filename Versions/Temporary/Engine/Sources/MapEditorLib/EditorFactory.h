#if !defined(__EDITOR_FACTORY__)
#define __EDITOR_FACTORY__
#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( Editor );
#define REGISTER_EDITOR_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( Editor, name, classname )
#define REGISTER_EDITOR_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( Editor, name, classname )

#endif // #if !defined(__EDITOR_FACTORY__)
