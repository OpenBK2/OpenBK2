#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDb
{
	struct SNetGameConsts;
	struct SMapInfo;
};
namespace NNet
{
	interface IDriver;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IAILogic;
interface ICheckSumLog;
interface IAICmdsAutoMagic;
interface ITransceiver : public CObjectBase
{
	virtual void StartMission( const NDb::SMapInfo *pMap, IAILogic *pAI ) = 0;
	// perform segments for AI
	virtual void DoSegments() = 0;

	// обработать pCommand, пришедшую на текущем сегменте
	virtual void SendCommand( interface IAILogicCommandB2 *pCommand ) = 0;

	// client commands
	virtual void CommandClientTogglePause() = 0;
	virtual void CommandClientSpeed( int nChange ) = 0;
	virtual void CommandClientDropPlayer( const wstring &szPlayerNick ) = 0;
	virtual void CommandTimeOut( bool bSet ) = 0;

	virtual ICheckSumLog *GetCheckSumLogger() = 0;
	virtual	const NDb::SMapInfo *GetMap() const = 0;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SReplayInfo
{
	bool bDoReplay;
	string szReplayName;
	SReplayInfo() : bDoReplay(false) {}
	SReplayInfo( const string &_sz ) : bDoReplay(true), szReplayName(_sz) {}
};
ITransceiver *CreateSinglePlayerTransceiver( const SReplayInfo &replay, IAILogic *pAI );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
