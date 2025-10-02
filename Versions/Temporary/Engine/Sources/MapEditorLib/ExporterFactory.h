#if !defined(__EXPORTER_FACTORY__)
#define __EXPORTER_FACTORY__
#pragma once
#include "DefaultFactoryHdr.h"

DECLARE_FACTORY( Exporter );
#define REGISTER_EXPORTER_IN_EXE( name, classname ) REGISTER_ME_OBJECT_IN_EXE( Exporter, name, classname )
#define REGISTER_EXPORTER_IN_DLL( name, classname ) REGISTER_ME_OBJECT_IN_DLL( Exporter, name, classname )

#endif // #if !defined(__EXPORTER_FACTORY__)
