#include "stdafx.h"

#include "AICmdsAutoMagic.h"
#include "AILogicCommandInternal.h"
#include "CommonCommands.h"

#include "GameX_export.h"

#include <fmt/format.h>

//*******************************************************************
//*												CAICmdsAutomagic													*
//*******************************************************************

CAICmdsAutomagic::CAICmdsAutomagic()
{
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CRegisterGroupCommand ) ) );
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CUnregisterGroupCommand ) ) );
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CB2GroupCommand ) ) );
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CUnitCommand ) ) );
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CControlSumCheckCommand ) ) );
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CControlSumHistoryCommand ) ) ); // Test?
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CDropPlayerCommand ) ) );
	// Network type IDs are separate from save/replay registration. Append to preserve existing wire IDs.
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CTransferUnitsCommand ) ) );

	NI_ASSERT( byte2msg.size() < 256, fmt::format( "Too many net messages ({})", byte2msg.size() ) );

	for ( int i = 0; i < byte2msg.size(); ++i )
	{
		msg2byte[byte2msg[i]] = i;
	}
}

int CAICmdsAutomagic::GetCommandID( CObjectBase *p )
{
	if ( !p )
	{
		return -1;
	}
	const int nTypeID = NObjectFactory::GetObjectTypeID( typeid( *p ) );
	const auto it = msg2byte.find( nTypeID );
	// Assertions are disabled in release builds. Never turn an unknown command into wire ID zero.
	return it != msg2byte.end() ? it->second : -1;
}

CObjectBase *CAICmdsAutomagic::MakeCommand( int nID )
{
	if ( nID < 0 || nID >= byte2msg.size() )
	{
		return nullptr;
	}
	return NObjectFactory::MakeObject( byte2msg[nID] );
}

IAICmdsAutoMagic *CreateAICmdsAutoMagic()
{
	return new CAICmdsAutomagic();
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x300A7AC1, CAICmdsAutomagic )
