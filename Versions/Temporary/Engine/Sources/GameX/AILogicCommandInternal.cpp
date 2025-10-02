#include "stdafx.h"

#include "AILogicCommandInternal.h"

#include "../AILogic/B2AI.h"

// ************************************************************************************************************************ //
// **
// ** register group
// **
// **
// **
// ************************************************************************************************************************ //

CRegisterGroupCommand::CRegisterGroupCommand( const vector<int> &vIDs, const int _nID )
: nID( _nID ), unitsIDs( vIDs )
{
}

void CRegisterGroupCommand::Execute()
{
	Singleton<IAILogic>()->RegisterGroup( unitsIDs, nID );
}

// ************************************************************************************************************************ //
// **
// ** unregister group
// **
// **
// **
// ************************************************************************************************************************ //

CUnregisterGroupCommand::CUnregisterGroupCommand( const int _nGroup )
: nGroup( _nGroup )
{
}

void CUnregisterGroupCommand::Execute()
{
	Singleton<IAILogic>()->UnregisterGroup( nGroup );
}

// ************************************************************************************************************************ //
// **
// ** group command
// **
// **
// **
// ************************************************************************************************************************ //

CB2GroupCommand::CB2GroupCommand( const SAIUnitCmd *pCommand, const WORD _wGroup, bool _bPlaceInQueue)
: command( *pCommand ), wGroup( _wGroup ), bPlaceInQueue( _bPlaceInQueue )
{
}

void CB2GroupCommand::Execute()
{
	Singleton<IAILogic>()->GroupCommand( &command, wGroup, bPlaceInQueue );
}

// ************************************************************************************************************************ //
// **
// ** unit command
// **
// **
// **
// ************************************************************************************************************************ //

CUnitCommand::CUnitCommand( const struct SAIUnitCmd *pCommand, const WORD _wID, const int _nPlayer )
: command( *pCommand ), wID( _wID ), nPlayer( _nPlayer )
{
}

void CUnitCommand::Execute()
{
	Singleton<IAILogic>()->UnitCommand( &command, wID, nPlayer );
}

REGISTER_SAVELOAD_CLASS( 0x300A73C0, CRegisterGroupCommand )
REGISTER_SAVELOAD_CLASS( 0x300A73C1, CUnregisterGroupCommand )
REGISTER_SAVELOAD_CLASS( 0x300A73C2, CB2GroupCommand )
REGISTER_SAVELOAD_CLASS( 0x300A73C3, CUnitCommand )

