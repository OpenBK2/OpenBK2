#pragma once

#include "Main/CommandsHistory.hpp"
#include "Transceiver.h"

struct ICommandsHistory;
struct IAILogic;

class CSinglePlayerTransceiver : public ITransceiver
{
	OBJECT_NOCOPY_METHODS( CSinglePlayerTransceiver );

	ZDATA
	// история команд
	CPtr<ICommandsHistory> pCmdsHistory;
	// общий номер сегмента - для истории команд
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	//
	// int32_t and not long: this is serialized, and long is four bytes on
	// Windows and eight on Linux, so the two wrote different chunk sizes into
	// the same replay. Four is what every existing recording holds, and every
	// other use of the counter is already an int, including
	// GetCurrentCommonSegment's return type.
	int32_t nCommonSegment = {};
	bool bCommandsFromHistory = {};
	CPtr<IAILogic> pAI;
	int nAdjustedGameSpeed = {};
	int nGameSpeed = {};
	NTimer::STime nStartCountingTime = {};
	int nFrames = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pCmdsHistory );
		f.Add( 3, &nCommonSegment );
		f.Add( 4, &bCommandsFromHistory );
		f.Add( 5, &pAI );
		f.Add( 6, &nAdjustedGameSpeed );
		f.Add( 7, &nGameSpeed );
		f.Add( 8, &nStartCountingTime );
		f.Add( 9, &nFrames );
		return 0;
	}

	void AdjustGameSpeed( const int nDelta );

public:
	CSinglePlayerTransceiver();
	CSinglePlayerTransceiver( ICommandsHistory *_pCommandHistory, IAILogic *_pAI );

	~CSinglePlayerTransceiver() override {}

	void StartMission( const NDb::SMapInfo *_pMap, IAILogic *pAI ) override;
	// perform segments for AI
	void DoSegments() override;

	void SendCommand( struct IAILogicCommandB2 *pCommand ) override;

	// client commands
	void CommandClientTogglePause() override;
	void CommandClientSpeed( int nChange ) override;

	void CommandClientDropPlayer( const std::wstring &szPlayerNick ) override {}

	void CommandTimeOut( bool bSet ) override {}

	ICheckSumLog *GetCheckSumLogger() override
	{
		return pCmdsHistory;
	}

	const NDb::SMapInfo *GetMap() const override;
};
