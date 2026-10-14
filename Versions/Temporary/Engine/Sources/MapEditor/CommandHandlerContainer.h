#pragma once

#include "MapEditorLib/Interface_CommandHandler.h"

#include <cstdint>

class CCommandHandlerContainer : public ICommandHandlerContainer
{
	OBJECT_NOCOPY_METHODS( CCommandHandlerContainer );

	//
	struct SCommandRange
	{
		// CRAP{ HASH_SET
		typedef std::unordered_map<unsigned, int> CCommandIDSet; // важно наличие
		// CRAP} HASH_SET
		CCommandIDSet commandIDSet;

		SCommandRange() {}

		SCommandRange( const SCommandRange &rCommandRange )
			: commandIDSet( rCommandRange.commandIDSet )
		{
		}

		SCommandRange &operator=( const SCommandRange &rCommandRange )
		{
			if ( &rCommandRange != this )
			{
				commandIDSet = rCommandRange.commandIDSet;
			}
			return *this;
		}
	};

	//
	typedef std::unordered_map<unsigned, SCommandRange> CCommandHandlerIDToCommandIDMap;
	typedef std::unordered_map<unsigned, unsigned> CCommandIDToCommandHandlerIDMap;
	typedef std::unordered_map<unsigned, ICommandHandler *> CCommandHandlerMap;
	//
	CCommandHandlerIDToCommandIDMap commandHandlerIDToCommandIDMap;
	CCommandIDToCommandHandlerIDMap commandIDToCommandHandlerIDMap;
	CCommandHandlerMap commandHandlerMap;

public:
	// ICommandHandlerContainer
	void Register( unsigned nType, unsigned nFirstCommandID, unsigned nLastCommandID ) override;
	void UnRegister( unsigned nType ) override;
	void Set( unsigned nType, ICommandHandler *pCommandHandler ) override;
	void Remove( unsigned nType, ICommandHandler *pCommandHandler ) override;
	void Remove( unsigned nType ) override;
	ICommandHandler *Get( unsigned nType ) override;
	bool HandleCommand( unsigned nType, unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nType, unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) override;
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) override;
};
