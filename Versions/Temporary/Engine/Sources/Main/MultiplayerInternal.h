#pragma once
#include "Multiplayer.h"

namespace NNet
{
struct IDriver;
}
class CMultiplayerCommand;

namespace NDb
{
struct SNetGameConsts;
};

struct IAICmdsAutoMagic;

class CMultiplayerInternal : public IMultiplayer
{
	OBJECT_BASIC_METHODS( CMultiplayerInternal );

	ZDATA
	CPtr<NNet::IDriver> pNetDriver;
	CDBPtr<NDb::SNetGameConsts> pConsts;
	CMemoryStream pktOutgoing, pktIncoming;
	int nIncomingPktClientID;
	CPtr<IAICmdsAutoMagic> pCmdsSerialize;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pNetDriver );
		f.Add( 3, &pConsts );
		f.Add( 4, &pktOutgoing );
		f.Add( 5, &pCmdsSerialize );
		return 0;
	}

	//
	bool CheckConnection();
	CMultiplayerCommand *ProcessPacket( const int nClientID, CMemoryStream *pPkt );
	int GetPort() const;

public:
	CMultiplayerInternal( const NDb::SNetGameConsts *_pConsts = 0, IAICmdsAutoMagic *pCmds = 0 );
	void CreateAsHost();
	bool CreateAsClient( const char *pszIPAddress );

	void SendAICommand( CObjectBase *pCommand ) override;
	void SendDirect( int nPlayer, CObjectBase *pCommand ) override;
	CMultiplayerCommand *GetCommand() override;
	void FinishSegment() override;
	int GetSelfPlayerNum() override;
	EConnectionState GetState() override;
	float GetPing( int nPlayer ) override;

	void SendRecv() override;

	// CRAP{ only for alpha
	struct NNet::IDriver *GetNetDriver() const
	{
		return pNetDriver;
	}

	// CRAP}
};
