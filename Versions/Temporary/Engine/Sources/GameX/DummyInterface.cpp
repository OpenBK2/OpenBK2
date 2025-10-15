#include "stdafx.h"

#include "DummyInterface.h"
#include "GameXClassIDs.h"

#include "GameX_export.h"

// ************************************************************************************************************************ //
// **
// ** dummy(empty) interface command
// **
// **
// **
// ************************************************************************************************************************ //

CDummyInterface::CDummyInterface()
: CInterfaceScreenBase( "dummy_interface", "default" )
{
}

CDummyInterface::~CDummyInterface()
{
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x160AC441, CDummyInterface );
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_DUMMY, CICDummy );


