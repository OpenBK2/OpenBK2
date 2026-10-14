#pragma once

//#include "../Server_Client_Common/PacketProcessor.h"
#include "MPInterfaceData.h"
#include "MPManagerMode.h"
#include "Net/NetAddress.h"
#include "Net/NetDriver.h"
#include "Server_Client_Common/GameInfo.h"

struct IServerClient;

namespace NDb
{
struct SMultiplayerMap;
}

IMPToUIManager *CreateMPManager();

class CMPManager : public IMPToUIManager, public CMPUIMessageProcessor
{
	OBJECT_NOCOPY_METHODS( CMPManager );

	CObj<CMPManagerMode> pCurrentNetMode;

	bool OnUnknownMessage( SMPUIMessage *pMsg );
	bool OnNoNetMessage( SMPUIMessage *pMsg );
	bool OnNivalNetMessage( SMPUIMessage *pMsg );
	bool OnLANNetMessage( SMPUIMessage *pMsg );

	bool OnBackFromGameListMessage( SMPUIMessage *pMsg );

public:
	CMPManager();
	~CMPManager() override;

	void SetLanTester( CLANTester *_pLANTester )
	{
		pCurrentNetMode->SetLanTester( _pLANTester );
	}

	//{ IMPToUIManager
	void AddUIMessage( SMPUIMessage *pMsg ) override;
	void AddUIMessage( EMPUIMessageType eMessageType ) override;
	SMPUIMessage *GetUIMessage() override;
	SMPUIMessage *PeekUIMessage() override;
	void MPUISegment() override;
	//}

	//{ CPacketProcessor
	bool Segment();

	//}
	bool SaveReplay( const std::string &szFileName ) override
	{
		return pCurrentNetMode->SaveReplay( szFileName );
	}

	//void UpdateInfoForReplay( SMultiplayerReplayInfo *pReplayInfo ) { pCurrentNetMode->UpdateInfoForReplay( pReplayInfo ); }
};
