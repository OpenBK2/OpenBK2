#pragma once

#include "PaletteCommands.h"
#include "Stats_B2_M1/DBMapInfo.h"

#include <vector>

// What the AI general points palette holds, and how the editor asks for it.
// Both used to live in AIGeneralWindow.h beside the MFC window; they are here
// so the wx palette can share them rather than restate them.

struct SAIGeneralPointsWindowData
{
	struct SAIPlayerInfo
	{
		struct SAIParcel : public NDb::SAIGeneralParcel
		{
			int nCurrentPoint;
			SAIParcel()
			{
				NDb::SAIGeneralParcel();
				Clear();
			}
			void Clear()
			{
				nCurrentPoint = -1;
			}
		};

		std::vector<int> mobileScriptIDs;
		std::vector<SAIParcel> parcels;
		int nCurrentID;
		int nCurrentParcel;
		//
		SAIPlayerInfo()
		{
			Clear();
		}
		void Clear()
		{
			mobileScriptIDs.clear();
			parcels.clear();
			nCurrentID = 0;
			nCurrentParcel = 0;
		}
	};
	//
	enum EAIGenPointsLastAction
	{
		AIGP_UNKNOWN,
		AIGP_NO_ACTIONS,
		AIGP_ID_ADD,
		AIGP_ID_DEL,
		AIGP_ID_JUMP,
		AIGP_PARCEL_ADD,
		AIGP_PARCEL_DEL,
		AIGP_PARCEL_JUMP,
		AIGP_PARCEL_EDIT,
		AIGP_PLAYER_JUMP
	};

	std::vector<SAIPlayerInfo> players;
	int nCurrentPlayer;
	EAIGenPointsLastAction eLastAction;

	SAIGeneralPointsWindowData()
	{
		Clear();
		nCurrentPlayer = 0;
	}
	void Clear()
	{
		players.clear();
	}
	//
	int CurrentPlayer() { return nCurrentPlayer; }
	int CurrentParcel() { return players[CurrentPlayer()].nCurrentParcel; }
	int CurrentID() { return players[CurrentPlayer()].nCurrentID; }
	int CurrentPoint() { return players[CurrentPlayer()].parcels[CurrentParcel()].nCurrentPoint; }
};


// CMapInfoState drives this palette with the two window commands every palette
// takes, so the dispatch is CPaletteCommands' and only the reading and writing
// of controls is written per implementation.
typedef CPaletteCommands<SAIGeneralPointsWindowData> CAIGeneralPointsCommands;
