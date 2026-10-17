#pragma once

#include "AILogic/AILogicCommand.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include "GameX_export.h"

#include <cstdint>
#include <vector>

#include <fmt/format.h>

// Ownership changes carry the selection itself so later selection/group changes cannot affect the donation.
class GAMEX_EXPORT CTransferUnitsCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CTransferUnitsCommand );
	std::vector<int> unitIDs;
	int nSourcePlayer = -1;
	int nTargetPlayer = -1;

public:
	CTransferUnitsCommand() {}

	CTransferUnitsCommand( const std::vector<int> &ids, int nSource, int nTarget );

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 2, &unitIDs );
		saver.Add( 3, &nSourcePlayer );
		saver.Add( 4, &nTargetPlayer );
		return 0;
	}

	void Execute() override;

	bool NeedToBeStored() const override
	{
		return true;
	}

#ifndef _FINALRELEASE
	std::string GetDebugInfo() const override
	{
		return fmt::format(
				"TransferUnits Player {} to {}, {} selected objects", nSourcePlayer, nTargetPlayer, unitIDs.size() );
	}
#endif
};

class CRegisterGroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CRegisterGroupCommand );
	//
	ZDATA
	std::vector<int> unitsIDs; // IDs of all obejcts in group
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	uint16_t nID = {}; // ID of the group

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &unitsIDs );
		f.Add( 3, &nID );
		return 0;
	}

public:
	CRegisterGroupCommand() {}

	CRegisterGroupCommand( const std::vector<int> &vIDs, const int nID );
	//	CRegisterGroupCommand( CObjectBase **pUnitsBuffer, const int nLen, const uint16_t wID, IAILogic *pAILogic );
	//
	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return true;
	}
#ifndef _FINALRELEASE
	std::string GetDebugInfo() const override
	{
		std::string szDebug;
		szDebug = fmt::format( "GroupID = {}, UnitIDs: ", nID );
		for ( int i = 0; i < unitsIDs.size(); ++i )
		{
			szDebug += fmt::format( "{}, ", unitsIDs[i] );
		}
		return szDebug;
	}
#endif
};

class CUnregisterGroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CUnregisterGroupCommand );
	//
	ZDATA
	int nGroup = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &nGroup );
		return 0;
	}

public:
	CUnregisterGroupCommand() {}

	CUnregisterGroupCommand( const int nGroup );
	//
	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return true;
	}
#ifndef _FINALRELEASE
	std::string GetDebugInfo() const override
	{
		return fmt::format( "UnregisterGroup {}", nGroup );
	}
#endif
};

class CB2GroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CB2GroupCommand );
	//
	ZDATA
	SAIUnitCmd command;      // command itself
	uint16_t wGroup = {};    // group ID, this command for
	bool bPlaceInQueue = {}; // do we need place this command in the group's queue

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &command );
		f.Add( 3, &wGroup );
		f.Add( 4, &bPlaceInQueue );
		return 0;
	}

public:
	CB2GroupCommand() {}

	CB2GroupCommand( const SAIUnitCmd *pCommand, const uint16_t wGroup, bool bPlaceInQueue );
	//
	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return true;
	}

	SAIUnitCmd *GetCommand()
	{
		return &command;
	}
#ifndef _FINALRELEASE
	std::string GetDebugInfo() const override
	{
		return fmt::format( "GroupCommand CmdID {}, GroupID {}, ({:f},{:f})",
		                    int( command.nCmdType ),
		                    wGroup,
		                    command.vPos.x,
		                    command.vPos.y );
	}
#endif
};

class CUnitCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CUnitCommand );
	//
	ZDATA
	SAIUnitCmd command; // command itself
	uint16_t wID = {};  // group ID - result of this command :)
	int nPlayer = {};   // player number

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &command );
		f.Add( 3, &wID );
		f.Add( 4, &nPlayer );
		return 0;
	}

public:
	CUnitCommand() {}

	CUnitCommand( const struct SAIUnitCmd *pCommand, const uint16_t wID, const int nPlayer );
	//
	void Execute() override;

	//
	bool NeedToBeStored() const override
	{
		return true;
	}

	SAIUnitCmd *GetCommand()
	{
		return &command;
	}
#ifndef _FINALRELEASE
	std::string GetDebugInfo() const override
	{
		return fmt::format( "UnitCommand CmdID {}, Player {}, ({:f},{:f})",
		                    int( command.nCmdType ),
		                    nPlayer,
		                    command.vPos.x,
		                    command.vPos.y );
	}
#endif
};
