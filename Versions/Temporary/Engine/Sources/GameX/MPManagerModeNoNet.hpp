#pragma once
#include "MPManagerMode.h"

class CMPManagerModeNoNet : public CMPManagerMode
{
	OBJECT_BASIC_METHODS( CMPManagerModeNoNet );

	const int GetOwnClientID() override
	{
		return -1;
	}

	const bool IsInGameRoom() const override
	{
		return false;
	}

	const bool IsGameRunning() const override
	{
		return false;
	}

	const bool IsGameHost() const override
	{
		return false;
	}

	void UpdateGameList() override {}

	void OnLeaveGame() override {}

	void OnGameRoomClientAdded() override {}

	void OnGameRoomClientRemoved() override {}

	void OnSetMySlotNumber() override {}

	void OnGameSpecificInfo() override {}

	void KickPlayerFromSlot( const int nSlot ) override {}

public:
	const ENetMode GetMode() const override
	{
		return ENM_NONE;
	}

	void SetLanTester( class CLANTester *_pLANTester ) override
	{
		NI_ASSERT( 0, "Wrong call" )
	}
};
