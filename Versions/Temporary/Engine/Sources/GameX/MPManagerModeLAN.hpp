#pragma once

#include "Client/LANClient.h"
#include "MPManagerMode.h"

#include <cstdint>

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

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pMPMap );
			f.Add( 3, &nGameType );
			f.Add( 4, &nTechLevel );
			return 0;
		}

		SGameSettings()
			: nGameType( 0 )
			, nTechLevel( 0 )
		{
		}
	};

	CPtr<CLANClient> pLANClient;

	uint16_t wGameUniqueID;
	EGameState eCurrentState;
	// Whether the failure to create pLANClient (UDP port taken, no network) has
	// been shown to the player yet. Segment reports it once; see the constructor.
	bool bDriverFailureReported;
	// Why pLANClient could not be created, from CLANClient::Create, shown under
	// the generic error text.
	std::string szDriverError;

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

	const int GetOwnClientID() override
	{
		return pLANClient->GetOwnClientID();
	}

	const bool IsInGameRoom() const override
	{
		return ( eCurrentState == EGS_HOST || eCurrentState == EGS_CLIENT );
	}

	const bool IsGameRunning() const override
	{
		return ( eCurrentState == EGS_GAME_STARTED );
	}

	const bool IsGameHost() const override
	{
		return ( eCurrentState == EGS_HOST );
	}

	void UpdateGameList() override;
	void TryToCreateGame() override;
	void TryToJoinGame( const SNetGameInfo &game ) override;
	void StartGame() override;
	void EndGame() override;

	void OnGameAborted() override
	{
		eCurrentState = EGS_NOT_IN_GAME;
	}

	void OnLeaveGame() override;
	void OnGameRoomClientAdded() override;
	void OnGameRoomClientRemoved() override;
	void OnSetMySlotNumber() override;
	void OnGameSpecificInfo() override;
	void KickPlayerFromSlot( const int nSlot ) override;

	void StartNewLANDriver();
	void ReportDriverFailure();
	void CompileAndSendLANServerInfo();
	void ExtractGameInfo( SNetGameInfo *pDst, NNet::IDriver::SGameInfo *pSrc );

public:
	CMPManagerModeLAN();

	bool Segment() override;

	const ENetMode GetMode() const override
	{
		return ENM_LAN;
	}

	void SetLanTester( CLANTester *_pLANTester ) override
	{
		pLanTester = _pLANTester;
	}
};
