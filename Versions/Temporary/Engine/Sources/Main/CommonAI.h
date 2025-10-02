#pragma once

struct ICommonAI : public CObjectBase
{
	enum { tidTypeID = 0x30121380 };
	
	virtual void Segment() = 0;
	virtual bool IsSuspended() const = 0;
	virtual void Suspend() = 0;
	virtual void Resume() = 0;

	virtual void NetGameStarted() = 0;
	virtual bool IsNetGameStarted() const = 0;
	virtual void NoWin() = 0;
	virtual bool IsNoWin() const = 0;

	virtual void SetMyDiplomacyInfo( const int nParty, const int nNumber ) = 0;
	virtual void SetNPlayers( const int nPlayers ) = 0;
	virtual void SetNetGame( const bool bNetGame ) = 0;
	virtual void NeutralizePlayer( const int nPlayer ) = 0;

	virtual void ProcessCommand( CObjectBase *pCommand ) { }
};


