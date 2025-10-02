#include "StdAfx.h"

#include "CommandHandlerContainer.h"
#include "..\MapEditorLib\Tools_HashSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CCommandHandlerContainer::Register( UINT nType, UINT nFirstCommandID, UINT nLastCommandID )
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
			UINT nCommandID = nFirstCommandID;
			nFirstCommandID = nLastCommandID;
			nLastCommandID = nCommandID;
		}
		for ( UINT nCommandID = nFirstCommandID; nCommandID <= nLastCommandID; ++nCommandID )
		{
			InsertHashSetElement( &( posCommandHandlerIDToCommandID->second.commandIDSet ), nCommandID );
			commandIDToCommandHandlerIDMap[nCommandID] = nType;
		}
	}
}


void CCommandHandlerContainer::UnRegister( UINT nType )
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


void CCommandHandlerContainer::Set( UINT nType, ICommandHandler *pCommandHandler )
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


void CCommandHandlerContainer::Remove( UINT nType, ICommandHandler *pCommandHandler )
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


void CCommandHandlerContainer::Remove( UINT nType )
{ 
	CCommandHandlerMap::iterator posCommandHandler = commandHandlerMap.find( nType );
	if ( posCommandHandler != commandHandlerMap.end() )
	{
		commandHandlerMap.erase( posCommandHandler );
	}
}


ICommandHandler* CCommandHandlerContainer::Get( UINT nType )
{
	CCommandHandlerMap::iterator posCommandHandler = commandHandlerMap.find( nType );
	if ( posCommandHandler != commandHandlerMap.end() )
	{
		return posCommandHandler->second;
	}
	return 0;
}


bool CCommandHandlerContainer::HandleCommand( UINT nType, UINT nCommandID, DWORD dwData )
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


bool CCommandHandlerContainer::UpdateCommand( UINT nType, UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	ICommandHandler *pCommandHandler = Get( nType );
	if ( pCommandHandler )
	{
		return pCommandHandler->UpdateCommand( nCommandID, pbEnable, pbCheck );
	}
	return false;
}


bool CCommandHandlerContainer::HandleCommand( UINT nCommandID, DWORD dwData )
{
	CCommandIDToCommandHandlerIDMap::iterator posCommandIDToCommandHandlerID = commandIDToCommandHandlerIDMap.find( nCommandID );
	if ( posCommandIDToCommandHandlerID != commandIDToCommandHandlerIDMap.end() )
	{
		return HandleCommand( posCommandIDToCommandHandlerID->second, nCommandID, dwData );
	}
	return false;
}


bool CCommandHandlerContainer::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	CCommandIDToCommandHandlerIDMap::iterator posCommandIDToCommandHandlerID = commandIDToCommandHandlerIDMap.find( nCommandID );
	if ( posCommandIDToCommandHandlerID != commandIDToCommandHandlerIDMap.end() )
	{
		return UpdateCommand( posCommandIDToCommandHandlerID->second, nCommandID, pbEnable, pbCheck );
	}
	return false;
}

// basement storage  


