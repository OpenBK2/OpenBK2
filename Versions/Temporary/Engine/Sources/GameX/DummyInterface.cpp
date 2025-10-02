#include "StdAfx.h"

#include "DummyInterface.h"
#include "GameXClassIDs.h"


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

REGISTER_SAVELOAD_CLASS( 0x160AC441, CDummyInterface );
REGISTER_SAVELOAD_CLASS( ML_COMMAND_DUMMY, CICDummy );

