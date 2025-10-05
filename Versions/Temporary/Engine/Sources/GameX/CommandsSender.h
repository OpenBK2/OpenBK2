#pragma once

#include "Stats_B2_M1/AIUnitCmd.h"

struct ITransceiver;
class CCommandsSender : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CCommandsSender );

	ZDATA
		bool bHistoryPlaying;
		CPtr<ITransceiver> pTransciver;
		std::vector<int> lastGroup;
		int nLastGroupID;
		SAIUnitCmd lastCommand;
		NTimer::STime lastCommandTime;
		bool bGroupChanged;
		bool bLastCommandSkipped;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bHistoryPlaying); f.Add(3,&pTransciver); f.Add(4,&lastGroup); f.Add(5,&nLastGroupID); f.Add(6,&lastCommand); f.Add(7,&lastCommandTime); f.Add(8,&bGroupChanged); f.Add(9,&bLastCommandSkipped); return 0; }
public:
	CCommandsSender() : bHistoryPlaying(false), nLastGroupID( -1 ), lastCommandTime( 0 ), bGroupChanged( false ), bLastCommandSkipped( false ) {}
	CCommandsSender( ITransceiver *_pTransciver );

	void ResetGroup() { lastGroup.clear(); bGroupChanged = true; }
	// register group of units to AI
	virtual int CommandRegisterGroup( const std::vector<int> &vIDs );
	// unregister group
	virtual void CommandUnregisterGroup( const WORD wGroup );
	// send command to group of units
	virtual void CommandGroupCommand( const SAIUnitCmd *pCommand, const WORD wGroup, bool bPlaceInQueue, const int nCommandSaveID );
	// set single command to call planes, reinforcements, etc. returns group number, which was created
	virtual int CommandUnitCommand( const SAIUnitCmd *pCommand );
	// Actor-less command
	virtual void CommandGeneralCommand( const SAIUnitCmd *pCommand );

	bool LastCommandSkipped() const { return bLastCommandSkipped; }
	//
	void SendCommand( struct IAILogicCommandB2 *pCmd );
};


