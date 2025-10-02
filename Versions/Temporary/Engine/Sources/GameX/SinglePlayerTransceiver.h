#pragma once

#include "Transceiver.h"
#include "../Main/CommandsHistory.hpp"

interface ICommandsHistory;
interface IAILogic;

class CSinglePlayerTransceiver : public ITransceiver
{
	OBJECT_NOCOPY_METHODS( CSinglePlayerTransceiver );

	ZDATA
		// история команд
		CPtr<ICommandsHistory> pCmdsHistory;
	// общий номер сегмента - для истории команд
	long nCommonSegment;
	bool bCommandsFromHistory;
	CPtr<IAILogic> pAI;
	int nAdjustedGameSpeed;
	int nGameSpeed;
	NTimer::STime nStartCountingTime;
	int nFrames;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pCmdsHistory); f.Add(3,&nCommonSegment); f.Add(4,&bCommandsFromHistory); f.Add(5,&pAI); f.Add(6,&nAdjustedGameSpeed); f.Add(7,&nGameSpeed); f.Add(8,&nStartCountingTime); f.Add(9,&nFrames); return 0; }
	void AdjustGameSpeed( const int nDelta );
public:
	CSinglePlayerTransceiver();
	CSinglePlayerTransceiver( ICommandsHistory *_pCommandHistory, IAILogic *_pAI );
	~CSinglePlayerTransceiver() {}

	void StartMission( const NDb::SMapInfo *_pMap, IAILogic *pAI );
	// perform segments for AI
	void DoSegments();

	void SendCommand( interface IAILogicCommandB2 *pCommand );

	// client commands
	void CommandClientTogglePause();
	void CommandClientSpeed( int nChange );
	void CommandClientDropPlayer( const wstring &szPlayerNick ) {}
	void CommandTimeOut( bool bSet ) {}

	ICheckSumLog *GetCheckSumLogger() { return pCmdsHistory; }
	const NDb::SMapInfo *GetMap() const;
};


