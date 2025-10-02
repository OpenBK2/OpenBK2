#if !defined(__COMMAND_HANDLER__CONTINER__)
#define __COMMAND_HANDLER__CONTINER__
#pragma once

#include "../MapEditorLib/Interface_CommandHandler.h"

class  CCommandHandlerContainer : public ICommandHandlerContainer
{
	OBJECT_NOCOPY_METHODS( CCommandHandlerContainer );
	//
	struct SCommandRange
	{
		// CRAP{ HASH_SET
		typedef hash_map<UINT, int> CCommandIDSet; // важно наличие
		// CRAP} HASH_SET
		CCommandIDSet commandIDSet;

		SCommandRange() {}
		SCommandRange( const SCommandRange &rCommandRange ) : commandIDSet( rCommandRange.commandIDSet ) {}
		SCommandRange& operator=( const SCommandRange &rCommandRange )
		{
			if ( &rCommandRange != this )
			{
				commandIDSet = rCommandRange.commandIDSet;
			}
			return *this;
		}	
	};
	//
	typedef hash_map<UINT, SCommandRange> CCommandHandlerIDToCommandIDMap;
	typedef hash_map<UINT, UINT> CCommandIDToCommandHandlerIDMap;
	typedef hash_map<UINT, ICommandHandler*> CCommandHandlerMap;
	//
	CCommandHandlerIDToCommandIDMap commandHandlerIDToCommandIDMap;
	CCommandIDToCommandHandlerIDMap commandIDToCommandHandlerIDMap;
	CCommandHandlerMap commandHandlerMap;
	
public:
	// ICommandHandlerContainer
	void Register( UINT nType, UINT nFirstCommandID, UINT nLastCommandID );
	void UnRegister( UINT nType );
	void Set( UINT nType, ICommandHandler *pCommandHandler );
	void Remove( UINT nType, ICommandHandler *pCommandHandler );
	void Remove( UINT nType );
	ICommandHandler* Get( UINT nType );
	bool HandleCommand( UINT nType, UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nType, UINT nCommandID, bool *pbEnable, bool *pbCheck );
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};

#endif // !defined(__COMMAND_HANDLER__CONTINER__)


