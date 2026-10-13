#pragma once
#include "MPManagerMode.h"

class CMPManagerModeNoNet : public CMPManagerMode
{
	OBJECT_BASIC_METHODS( CMPManagerModeNoNet );

	virtual const int GetOwnClientID() { return -1; }
	virtual const bool IsInGameRoom() const { return false; }
	virtual const bool IsGameRunning() const { return false; }
	virtual const bool IsGameHost() const { return false; }
	virtual void UpdateGameList() {}
	virtual void OnLeaveGame() {}
	virtual void OnGameRoomClientAdded() {}
	virtual void OnGameRoomClientRemoved() {}
	virtual void OnSetMySlotNumber() {}
	virtual void OnGameSpecificInfo() {}
	virtual void KickPlayerFromSlot( const int nSlot ) {}
public:
	virtual const ENetMode GetMode() const { return ENM_NONE; }
	virtual void SetLanTester( class CLANTester *_pLANTester ) { NI_ASSERT( 0, "Wrong call" ) }
};
