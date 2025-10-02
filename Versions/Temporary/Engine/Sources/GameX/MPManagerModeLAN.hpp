#pragma once

#include "MPManagerMode.h"
#include "../Client/LANClient.h"

class CMPManagerModeLAN : public CMPManagerMode
{
	OBJECT_NOCOPY_METHODS( CMPManagerModeLAN );

	enum EGameState
	{
		EGS_NOT_IN_GAME,
		EGS_HOST,
		EGS_JOINING,
		EGS_CLIENT,
		EGS_GAME_STARTED,
	};

	struct SGameSettings
	{
		ZDATA
		CDBPtr<NDb::SMultiplayerMap> pMPMap;
		int nGameType;
		int nTechLevel;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pMPMap); f.Add(3,&nGameType); f.Add(4,&nTechLevel); return 0; }
		SGameSettings() : nGameType(0), nTechLevel(0) {}
	};

	CPtr<CLANClient> pLANClient;

	WORD wGameUniqueID;
	EGameState eCurrentState;

	//{ Messages
	bool OnChatMessage( SMPUIChatMessage *pMsg );
	//}

	//{ Packets
	bool OnNetNewClient( class CNetNewClient *pPacket );
	bool OnConnectGameFailed( class CConnectGameFailed *pPacket );
	bool OnGameClientWasKicked( class CGameClientWasKicked *pPacket );

	bool OnLANPassword( class CLANPasswordPacket *pPacket );
	bool OnRequestLANPassword( class CRequestLANPasswordPacket *pPacket );
	//}

	virtual const int GetOwnClientID() { return pLANClient->GetOwnClientID(); }
	virtual const bool IsInGameRoom() const { return ( eCurrentState == EGS_HOST || eCurrentState == EGS_CLIENT ); }
	virtual const bool IsGameRunning() const { return ( eCurrentState == EGS_GAME_STARTED ); }
	virtual const bool IsGameHost() const { return ( eCurrentState == EGS_HOST ); }

	virtual void UpdateGameList();
	virtual void TryToCreateGame();
	virtual void TryToJoinGame( const SNetGameInfo &game );
	virtual void StartGame();
	virtual void EndGame();
	virtual void OnLeaveGame();
	virtual void OnGameRoomClientAdded();
	virtual void OnGameRoomClientRemoved();
	virtual void OnSetMySlotNumber();
	virtual void OnGameSpecificInfo();
	virtual void KickPlayerFromSlot( const int nSlot );

	void StartNewLANDriver();
	void CompileAndSendLANServerInfo();
	void ExtractGameInfo( SNetGameInfo *pDst, NNet::IDriver::SGameInfo *pSrc );

public:
	CMPManagerModeLAN();

	bool Segment();

	virtual const ENetMode GetMode() const { return ENM_LAN; }
	virtual void SetLanTester( CLANTester *_pLANTester ) { pLanTester = _pLANTester ; }
};

