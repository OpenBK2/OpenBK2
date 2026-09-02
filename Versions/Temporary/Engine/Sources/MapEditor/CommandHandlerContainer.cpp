#include "stdafx.h"

#include "CommandHandlerContainer.h"
#include "MapEditorLib/Tools_HashSet.h"

#include <cstdint>

void CCommandHandlerContainer::Register( unsigned nType, unsigned nFirstCommandID, unsigned nLastCommandID )
{
	CCommandHandlerIDToCommandIDMap::iterator posCommandHandlerIDToCommandID = commandHandlerIDToCommandIDMap.find( nType );
	if ( posCommandHandlerIDToCommandID == commandHandlerIDToCommandIDMap.end() )
	{
		commandHandlerIDToCommandIDMap[nType] = SCommandRange();
		posCommandHandlerIDToCommandID = commandHandlerIDToCommandIDMap.find( nType );
	}
	if ( posCommandHandlerIDToCommandID != commandHandlerIDToCommandIDMap.end() )
	{
		if ( nFirstCommandID > nLastCommandID )
		{
			unsigned nCommandID = nFirstCommandID;
			nFirstCommandID = nLastCommandID;
			nLastCommandID = nCommandID;
		}
		for ( unsigned nCommandID = nFirstCommandID; nCommandID <= nLastCommandID; ++nCommandID )
		{
			InsertHashSetElement( &( posCommandHandlerIDToCommandID->second.commandIDSet ), nCommandID );
			commandIDToCommandHandlerIDMap[nCommandID] = nType;
		}
	}
}


void CCommandHandlerContainer::UnRegister( unsigned nType )
{
	CCommandHandlerIDToCommandIDMap::iterator posCommandHandlerIDToCommandID = commandHandlerIDToCommandIDMap.find( nType );
	if ( posCommandHandlerIDToCommandID != commandHandlerIDToCommandIDMap.end() )
	{
		for ( SCommandRange::CCommandIDSet::const_iterator itCommandID = posCommandHandlerIDToCommandID->second.commandIDSet.begin(); itCommandID != posCommandHandlerIDToCommandID->second.commandIDSet.end(); ++itCommandID )
		{
			CCommandIDToCommandHandlerIDMap::iterator posCommandIDToCommandHandlerID = commandIDToCommandHandlerIDMap.find( itCommandID->first );
			if ( posCommandIDToCommandHandlerID != commandIDToCommandHandlerIDMap.end() )
			{
				commandIDToCommandHandlerIDMap.erase( posCommandIDToCommandHandlerID );
			}
		}
		commandHandlerIDToCommandIDMap.erase( posCommandHandlerIDToCommandID );
	}
}


void CCommandHandlerContainer::Set( unsigned nType, ICommandHandler *pCommandHandler )
{ 
	if ( pCommandHandler == 0 )
	{
		Remove( nType );
	}
	else
	{
		commandHandlerMap[nType] = pCommandHandler;
	}
}


void CCommandHandlerContainer::Remove( unsigned nType, ICommandHandler *pCommandHandler )
{
	CCommandHandlerMap::iterator posCommandHandler = commandHandlerMap.find( nType );
	if ( posCommandHandler != commandHandlerMap.end() )
	{
		if ( posCommandHandler->second == pCommandHandler )
		{
			commandHandlerMap.erase( posCommandHandler );
		}
	}
}


void CCommandHandlerContainer::Remove( unsigned nType )
{ 
	CCommandHandlerMap::iterator posCommandHandler = commandHandlerMap.find( nType );
	if ( posCommandHandler != commandHandlerMap.end() )
	{
		commandHandlerMap.erase( posCommandHandler );
	}
}


ICommandHandler* CCommandHandlerContainer::Get( unsigned nType )
{
	CCommandHandlerMap::iterator posCommandHandler = commandHandlerMap.find( nType );
	if ( posCommandHandler != commandHandlerMap.end() )
	{
		return posCommandHandler->second;
	}
	return 0;
}


bool CCommandHandlerContainer::HandleCommand( unsigned nType, unsigned nCommandID, uintptr_t dwData )
{
//	DebugTrace ( "CCommandHandlerContainer::HandleCommand ... nType = %d", nType );
	ICommandHandler *pCommandHandler = Get( nType );
	if ( pCommandHandler )
	{
		return pCommandHandler->HandleCommand( nCommandID, dwData );
	}
	else
	{
		return false;
	}
}


bool CCommandHandlerContainer::UpdateCommand( unsigned nType, unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	ICommandHandler *pCommandHandler = Get( nType );
	if ( pCommandHandler )
	{
		return pCommandHandler->UpdateCommand( nCommandID, pbEnable, pbCheck );
	}
	return false;
}


bool CCommandHandlerContainer::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	CCommandIDToCommandHandlerIDMap::iterator posCommandIDToCommandHandlerID = commandIDToCommandHandlerIDMap.find( nCommandID );
	if ( posCommandIDToCommandHandlerID != commandIDToCommandHandlerIDMap.end() )
	{
		return HandleCommand( posCommandIDToCommandHandlerID->second, nCommandID, dwData );
	}
	return false;
}


bool CCommandHandlerContainer::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	CCommandIDToCommandHandlerIDMap::iterator posCommandIDToCommandHandlerID = commandIDToCommandHandlerIDMap.find( nCommandID );
	if ( posCommandIDToCommandHandlerID != commandIDToCommandHandlerIDMap.end() )
	{
		return UpdateCommand( posCommandIDToCommandHandlerID->second, nCommandID, pbEnable, pbCheck );
	}
	return false;
}

// basement storage  


