#pragma once

#include "AILogic/AILogicCommand.h"
#include "Stats_B2_M1/AIUnitCmd.h"

#include <cstdint>

class CRegisterGroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CRegisterGroupCommand );
	//
	ZDATA
		std::vector<int> unitsIDs;					// IDs of all obejcts in group
	uint16_t nID;														// ID of the group
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&unitsIDs); f.Add(3,&nID); return 0; }
public:
	CRegisterGroupCommand() { }
	CRegisterGroupCommand( const std::vector<int> &vIDs, const int nID  );
	//	CRegisterGroupCommand( CObjectBase **pUnitsBuffer, const int nLen, const uint16_t wID, IAILogic *pAILogic );
	//
	void Execute();
	//
	bool NeedToBeStored() const { return true; }
#ifndef _FINALRELEASE
	virtual std::string GetDebugInfo() const
	{
		std::string szDebug;
		szDebug = StrFmt( "GroupID = %i, UnitIDs: ", nID );
		for ( int i = 0; i < unitsIDs.size(); ++i )
			szDebug += StrFmt( "%i, ", unitsIDs[i] );
		return szDebug;
	}
#endif

};

class CUnregisterGroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CUnregisterGroupCommand );
	//
	ZDATA
		int nGroup;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nGroup); return 0; }
public:
	CUnregisterGroupCommand() { }
	CUnregisterGroupCommand( const int nGroup );
	//
	void Execute();
	//
	bool NeedToBeStored() const { return true; }
#ifndef _FINALRELEASE
	virtual std::string GetDebugInfo() const
	{
		return StrFmt( "UnregisterGroup %i", nGroup );
	}
#endif

};

class CB2GroupCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CB2GroupCommand );
	//
	ZDATA
		SAIUnitCmd command;									// command itself
	uint16_t wGroup;												// group ID, this command for
	bool bPlaceInQueue;									// do we need place this command in the group's queue
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&command); f.Add(3,&wGroup); f.Add(4,&bPlaceInQueue); return 0; }
public:
	CB2GroupCommand() { }
	CB2GroupCommand( const SAIUnitCmd *pCommand, const uint16_t wGroup, bool bPlaceInQueue );
	//
	void Execute();
	//
	bool NeedToBeStored() const { return true; }
	SAIUnitCmd *GetCommand() { return &command; }
#ifndef _FINALRELEASE
	virtual std::string GetDebugInfo() const
	{
		return StrFmt( "GroupCommand CmdID %i, GroupID %i, (%f,%f)", int(command.nCmdType), wGroup, command.vPos.x, command.vPos.y  );
	}
#endif

};

class CUnitCommand : public IAILogicCommandB2
{
	OBJECT_BASIC_METHODS( CUnitCommand );
	//
	ZDATA
		SAIUnitCmd command;									// command itself
	uint16_t wID;														// group ID - result of this command :)
	int nPlayer;												// player number
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&command); f.Add(3,&wID); f.Add(4,&nPlayer); return 0; }
public:
	CUnitCommand() { }
	CUnitCommand( const struct SAIUnitCmd *pCommand, const uint16_t wID, const int nPlayer );
	//
	void Execute();
	//
	bool NeedToBeStored() const { return true; }
	SAIUnitCmd *GetCommand() { return &command; }
#ifndef _FINALRELEASE
	virtual std::string GetDebugInfo() const
	{
		return StrFmt( "UnitCommand CmdID %i, Player %i, (%f,%f)", int(command.nCmdType), nPlayer, command.vPos.x, command.vPos.y  );
	}
#endif
};


