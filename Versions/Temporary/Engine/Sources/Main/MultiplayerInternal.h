#include "Multiplayer.h"

namespace NNet
{
	interface IDriver;
}
class CMultiplayerCommand;

namespace NDb
{
	struct SNetGameConsts;
};

interface IAICmdsAutoMagic;
class CMultiplayerInternal : public IMultiplayer
{
	OBJECT_BASIC_METHODS( CMultiplayerInternal );

	ZDATA
	CPtr<NNet::IDriver> pNetDriver;
	CDBPtr<NDb::SNetGameConsts> pConsts;
	CMemoryStream pktOutgoing, pktIncoming;
	int nIncomingPktClientID;
	CPtr<IAICmdsAutoMagic> pCmdsSerialize;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pNetDriver); f.Add(3,&pConsts); f.Add(4,&pktOutgoing); f.Add(5,&pCmdsSerialize); return 0; }
	//
	bool CheckConnection();
	CMultiplayerCommand* ProcessPacket( const int nClientID, CMemoryStream *pPkt );
	int GetPort() const;
public:
	CMultiplayerInternal( const NDb::SNetGameConsts *_pConsts = 0, IAICmdsAutoMagic *pCmds = 0 );
	void CreateAsHost();
	bool CreateAsClient( const char *pszIPAddress );

	virtual void SendAICommand( CObjectBase *pCommand );
	virtual void SendDirect( int nPlayer, CObjectBase *pCommand );
	virtual CMultiplayerCommand* GetCommand();
	virtual void FinishSegment();
	virtual int GetSelfPlayerNum();
	virtual EConnectionState GetState();
	float GetPing( int nPlayer );

	virtual void SendRecv();

	// CRAP{ only for alpha
	interface NNet::IDriver* GetNetDriver() const { return pNetDriver; }
	// CRAP}
};


