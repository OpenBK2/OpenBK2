#pragma once

// internal header for ITransciever realisation

interface IAICmdsAutoMagic;

namespace NDb
{
	struct SNetGameConsts;
};
namespace NNet
{
	interface IDriver;
}

class CMultiplayerCommand : public CObjectBase
{
	OBJECT_BASIC_METHODS( CMultiplayerCommand );
public:
	enum EMultiplayerCommand
	{
		EMC_SEGMENT_FINISHED,
		EMC_PLAYER_REMOVED,
		EMC_AI_COMMAND,
		EMC_PLAYER_ADDED,
		//EMC_START_GAME,
		//EMC_PLAYER_LAG,
		//EMC_PAUSE,
		//EMC_GAME_SPEED,
		//EMC_TIMEOUT,
		//EMC_PLAYER_ALIVE,
	};

	EMultiplayerCommand eCommand;
	CPtr<CObjectBase> pAILogicCommand;
	int nPlayer;
	int nParam;

	CMultiplayerCommand() : nPlayer( -1 ) { }
	CMultiplayerCommand( const EMultiplayerCommand _eCommand, const int _nPlayer, 
											 const int _nParam, CObjectBase *_pAILogicCommand )
		: eCommand( _eCommand ), pAILogicCommand( _pAILogicCommand ), 
			nPlayer( _nPlayer ), nParam( _nParam ) { }
};

interface IMultiplayer : public CObjectBase
{
	enum EConnectionState
	{
		CONNECTING,
		ACTIVE,
		INACTIVE
	};
	virtual void SendAICommand( CObjectBase *pCommand ) = 0;
	virtual void SendDirect( int nPlayer, CObjectBase *pCommand ) = 0;
	virtual CMultiplayerCommand* GetCommand() = 0;
	virtual void FinishSegment() = 0;
	virtual int GetSelfPlayerNum() = 0;
	virtual EConnectionState GetState() = 0;
	virtual float GetPing( int nPlayer ) = 0;

	virtual void SendRecv() = 0;
};
IMultiplayer *CreateMultiplayerHost( IAICmdsAutoMagic *pCmds, const NDb::SNetGameConsts *pConsts );
IMultiplayer *CreateMultiplayerClient( IAICmdsAutoMagic *pCmds, const NDb::SNetGameConsts *pConsts, const char *pszIPAddress );

