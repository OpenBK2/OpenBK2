#include "stdafx.h"

#include "AICmdsAutoMagic.h"
#include "CommonCommands.h"
#include "AILogicCommandInternal.h"

#include "GameX_export.h"

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
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CControlSumHistoryCommand ) ) );		// Test?
	byte2msg.push_back( NObjectFactory::GetObjectTypeID( typeid( CDropPlayerCommand ) ) );

	NI_ASSERT( byte2msg.size() < 256, StrFmt( "Too many net messages (%d)", byte2msg.size() ) );

	for ( int i = 0; i < byte2msg.size(); ++i )
		msg2byte[byte2msg[i]] = i;
}

int CAICmdsAutomagic::GetCommandID( CObjectBase *p )
{
	int nTypeID = NObjectFactory::GetObjectTypeID( typeid( *p ) );
	NI_ASSERT( msg2byte.find( nTypeID ) != msg2byte.end(), StrFmt( "AI command %s not found", typeid(*p).name() ) );
	return msg2byte[ nTypeID ];
}

CObjectBase *CAICmdsAutomagic::MakeCommand( int nID )
{
	NI_ASSERT( nID >= 0 && nID < msg2byte.size(), StrFmt( "Net message with byte code %d not found", nID ) );
	return NObjectFactory::MakeObject( byte2msg[ nID ] );
}

IAICmdsAutoMagic *CreateAICmdsAutoMagic()
{
	return new CAICmdsAutomagic();
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x300A7AC1, CAICmdsAutomagic )

