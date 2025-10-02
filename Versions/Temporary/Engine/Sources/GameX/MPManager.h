#pragma once

//#include "../Server_Client_Common/PacketProcessor.h"
#include "../Server_Client_Common/GameInfo.h"
#include "MPInterfaceData.h"
#include "../Net/NetDriver.h"
#include "../Net/NetAddress.h"
#include "MPManagerMode.h"


interface IServerClient;
namespace NDb
{
	struct SMultiplayerMap;
}

IMPToUIManager *CreateMPManager();

class CMPManager : public IMPToUIManager, public CMPUIMessageProcessor
{
	OBJECT_NOCOPY_METHODS(CMPManager);

	CObj<CMPManagerMode> pCurrentNetMode;

	bool OnUnknownMessage( SMPUIMessage *pMsg );
	bool OnNoNetMessage( SMPUIMessage *pMsg );
	bool OnNivalNetMessage( SMPUIMessage *pMsg );
	bool OnLANNetMessage( SMPUIMessage *pMsg );

	bool OnBackFromGameListMessage( SMPUIMessage *pMsg );

public:
	CMPManager();
	~CMPManager();


	void SetLanTester( CLANTester *_pLANTester ) { pCurrentNetMode->SetLanTester( _pLANTester ); }

	//{ IMPToUIManager
	virtual void AddUIMessage( SMPUIMessage *pMsg );
	virtual void AddUIMessage( EMPUIMessageType eMessageType );
	virtual SMPUIMessage* GetUIMessage();
	virtual SMPUIMessage* PeekUIMessage();
	virtual void MPUISegment();
	//}

	//{ CPacketProcessor
	bool Segment();
	//}
	bool SaveReplay( const string &szFileName ) { return pCurrentNetMode->SaveReplay( szFileName ); }
	//void UpdateInfoForReplay( SMultiplayerReplayInfo *pReplayInfo ) { pCurrentNetMode->UpdateInfoForReplay( pReplayInfo ); }
};

