#pragma once

#include "PaletteCommands.h"
#include "Stats_B2_M1/RPGStats.h"

#include <string>
#include <vector>

// What the reinforcement points palette holds, and how the editor asks for it.
// Both used to live in ReinfPointsWindow.h beside the MFC window; they are here
// so the wx palette can share them rather than restate them.

struct SReinfPointsWindowData
{
	struct STypedTemplate
	{
		NDb::EReinforcementType reinfType;
		std::string szTemplate;
	};
	//
	int nPlayerIndex;
	int nPlayerCount;
	//
	struct SReinfPoint
	{
		CVec2 vPosition;
		CVec2 vAviationPosition;
		NDb::EReinforcementType eType;
		bool bIsDefault;
		int nNumPoints;
		std::string szDeployTemplate;
		std::vector<STypedTemplate> typedTemplates;
		//
		SReinfPoint() :
			vPosition( VNULL2 ),
			eType( NDb::EReinforcementType(-1) ),
			bIsDefault( false ),
			nNumPoints( 0 ),
			szDeployTemplate( "" )
		{
		}
	};
	std::vector<SReinfPoint> reinfPoints;
	int nSelectedPoint;
	bool bAviationPointSelected;
	//
	enum EReinfWndLastAction
	{
		RWA_UNKNOWN,
		RWA_NO_ACTIONS,
		RWA_POINT_ADD,
		RWA_POINT_DEL,
		RWA_POINT_EDIT_DEPLOY,
		RWA_POINT_EDIT_TYPED,
		RWA_POINT_JUMP,
		RWA_POINT_SEL_CHANGE,
		RWA_PLAYER_CHANGE
	};
	EReinfWndLastAction eLastAction;
	//
	SReinfPointsWindowData() :
		nPlayerIndex(-1),
		nPlayerCount( 0 ),
		nSelectedPoint(-1),
		bAviationPointSelected( false ),
		eLastAction(RWA_NO_ACTIONS)
	{
	}
	//
	void Clear()
	{
		nPlayerIndex = -1;
		nPlayerCount = 0;
		reinfPoints.clear();
		nSelectedPoint = -1;
		bAviationPointSelected = false;
		eLastAction = RWA_NO_ACTIONS;
	}
};


// CMapInfoState drives this palette with the two window commands every palette
// takes, so the dispatch is CPaletteCommands' and only the reading and writing
// of controls is written per implementation.
typedef CPaletteCommands<SReinfPointsWindowData> CReinfPointsCommands;
