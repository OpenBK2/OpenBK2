#include "StdAfx.h"

#include "MapInfoEditorData_Consts.h"
#include "..\Stats_B2_M1\Vis2AI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NMapInfoEditor
{
	const DWORD BRUSH_COLOR											= 0xFF00FF00;
	const float BRUSH_RADIUS										= ( AI_TILE_SIZE * 1.0f );
	const int BRUSH_PARTS												= 8;
	//
	const DWORD PLACEMENT_COLOR									= 0xFFFF4040;
	const float PLACEMENT_RADIUS0								= ( AI_TILE_SIZE * 1.8f );
	const float PLACEMENT_RADIUS1								= ( AI_TILE_SIZE * 2.0f );
	const float PLACEMENT_DIRECTION_RADIUS			= ( AI_TILE_SIZE * 4.0f );
	const int PLACEMENT_PARTS										= 8;
	//
	const DWORD SCENE_OBJECT_SELECTION_COLOR		= 0xFF00AF00;
	const float SCENE_OBJECT_SELECTION_RADIUS0	= ( AI_TILE_SIZE * 1.8f );
	const float SCENE_OBJECT_SELECTION_RADIUS1	= ( AI_TILE_SIZE * 2.0f );
	const float SCENE_OBJECT_DIRECTION_RADIUS		= ( AI_TILE_SIZE * 4.0f );
	const int SCENE_OBJECT_SELECTION_PARTS			= 8;
	//
	const DWORD OBJECT_SELECTION_COLOR					= 0xFF00AF00;
	const float OBJECT_SELECTION_RADIUS0				= ( AI_TILE_SIZE * 1.8f );
	const float OBJECT_SELECTION_RADIUS1				= ( AI_TILE_SIZE * 2.0f );
	const float OBJECT_DIRECTION_RADIUS					= ( AI_TILE_SIZE * 4.0f );
	const int OBJECT_SELECTION_PARTS						= 8;
	//
	const DWORD OBJECT_LINK_COLOR								= 0xFFFF8080;
	const float OBJECT_LINK_RADIUS0							= ( AI_TILE_SIZE * 0.5f );
	const float OBJECT_LINK_RADIUS1							= ( AI_TILE_SIZE * 0.8f );
	const int OBJECT_LINK_PARTS									= 8;
	//
	const DWORD MAIN_OBJECT_SELECTION_COLOR			= 0xFF00AF00;
	const float MAIN_OBJECT_SELECTION_RADIUS0		= ( AI_TILE_SIZE * 1.8f );
	const float MAIN_OBJECT_SELECTION_RADIUS1		= ( AI_TILE_SIZE * 2.0f );
	const float MAIN_OBJECT_DIRECTION_RADIUS		= ( AI_TILE_SIZE * 4.0f );
	const int MAIN_OBJECT_SELECTION_PARTS				= 8;
	//
	const DWORD SELECTION_COLOR									= 0xFF00FF00;
	const float SELECTION_RADIUS0								= ( AI_TILE_SIZE * 5.8f );
	const float SELECTION_RADIUS1								= ( AI_TILE_SIZE * 6.0f );
	const float DIRECTION_RADIUS								= ( AI_TILE_SIZE * 12.0f );
	const float SELECTION_POINT_RADIUS					= ( AI_TILE_SIZE * 1.0f );
	const int SELECTION_PARTS										= 8;
	const int SELECTION_POINT_PARTS							= 8;
	//
	const float HEIGHT_DELIMITER								= 1.0f;
	//
	const DWORD DRAW_SELECTION_CIRCLE0					= 0x00000001;
	const DWORD DRAW_SELECTION_CIRCLE1					= 0x00000002;
	const DWORD DRAW_SELECTION_POINT_CIRCLE			= 0x00000004;
	const DWORD DRAW_DIRECTION									= 0x00000008;
	const DWORD DRAW_DIRECTION_POINT						= 0x00000010;
	const DWORD DRAW_ALL												= 0xFFFFFFFF;
	//
	const DWORD PARCEL_COLOR_UNKNOWN						= 0xFFFF0000;
	const DWORD PARCEL_COLOR_DEFENCE						= 0xFF00FF00;
	const DWORD PARCEL_COLOR_REINFORCE					= 0xFFFFFF00;
	const float PARCEL_REINFORCE_RAD						= ( AI_TILE_SIZE * 6.0f );
	const int PARCEL_PARTS											= 16;
	const int PARCEL_POINT_PARTS								= 8;
	//
	const float SCENE_FADE_COEFFICIENT					= 0.7f;
	const float SCENE_PASTE_OPACITY							= 0.7f;
	const float SCENE_NORMAL_OPACITY						= 1.0f;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool CheckLinkCapability( const string &rszObjectRPGStatsTypeName, const CDBID &rObjectRPGStatsDBID, UINT nFrameIndex,
														const string &rszLinkToObjectRPGStatsTypeName, const CDBID &rLinkToObjectRPGStatsID, UINT nLinkToFrameIndex )
	{
		/**					
		case NDb::SBuildingRPGStats::typeID:
		case NDb::SMechUnitRPGStats::typeID:
		case NDb::SSquadRPGStats::typeID:
		case NDb::SMineRPGStats::typeID:
		case NDb::SObjectRPGStats::typeID:
		case NDb::STerraObjSetRPGStats::typeID:
		case NDb::SFenceRPGStats::typeID:
		case NDb::SInfantryRPGStats::typeID:
		case NDb::SEntrenchmentRPGStats::typeID:
		case NDb::SBridgeRPGStats::typeID:
		/**/
		if ( rszObjectRPGStatsTypeName == "SquadRPGStats" )
		{
			if ( rszLinkToObjectRPGStatsTypeName == "BuildingRPGStats" )
			{
				return true;
			}
			else if ( rszLinkToObjectRPGStatsTypeName == "MechUnitRPGStats" )
			{
				return true;
			}
			else if ( rszLinkToObjectRPGStatsTypeName == "EntrenchmentRPGStats" )
			{
				return true;
			}
			/**
			SquadRPGStats
			MineRPGStats
			ObjectRPGStats
			TerraObjSetRPGStats
			FenceRPGStats
			InfantryRPGStats
			BridgeRPGStats
			/**/
		}
		else if ( rszObjectRPGStatsTypeName == "MechUnitRPGStats" )
		{
			if ( rszLinkToObjectRPGStatsTypeName == "MechUnitRPGStats" )
			{
				return true;
			}
			/**
			EntrenchmentRPGStats
			BuildingRPGStats
			SquadRPGStats
			MineRPGStats
			ObjectRPGStats
			TerraObjSetRPGStats
			FenceRPGStats
			InfantryRPGStats
			BridgeRPGStats
			/**/
		}
		/**
		SMineRPGStats
		SBuildingRPGStats
		SObjectRPGStats
		STerraObjSetRPGStats
		SFenceRPGStats
		SInfantryRPGStats
		SEntrenchmentRPGStats
		SBridgeRPGStats
		/**/
		return false;
	}
};

// basement storage  

