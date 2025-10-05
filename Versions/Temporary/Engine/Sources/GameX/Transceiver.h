#pragma once

namespace NDb
{
	struct SNetGameConsts;
	struct SMapInfo;
};
namespace NNet
{
	struct IDriver;
}

struct IAILogic;
struct ICheckSumLog;
struct IAICmdsAutoMagic;
struct ITransceiver : public CObjectBase
{
	virtual void StartMission( const NDb::SMapInfo *pMap, IAILogic *pAI ) = 0;
	// perform segments for AI
	virtual void DoSegments() = 0;

	// обработать pCommand, пришедшую на текущем сегменте
	virtual void SendCommand( struct IAILogicCommandB2 *pCommand ) = 0;

	// client commands
	virtual void CommandClientTogglePause() = 0;
	virtual void CommandClientSpeed( int nChange ) = 0;
	virtual void CommandClientDropPlayer( const std::wstring &szPlayerNick ) = 0;
	virtual void CommandTimeOut( bool bSet ) = 0;

	virtual ICheckSumLog *GetCheckSumLogger() = 0;
	virtual	const NDb::SMapInfo *GetMap() const = 0;
};

struct SReplayInfo
{
	bool bDoReplay;
	std::string szReplayName;
	SReplayInfo() : bDoReplay(false) {}
	SReplayInfo( const std::string &_sz ) : bDoReplay(true), szReplayName(_sz) {}
};
ITransceiver *CreateSinglePlayerTransceiver( const SReplayInfo &replay, IAILogic *pAI );


