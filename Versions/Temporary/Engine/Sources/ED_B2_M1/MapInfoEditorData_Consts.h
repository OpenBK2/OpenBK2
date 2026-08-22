#pragma once

#include "libdb/Manipulator.h"
#include "B2_M1_Terrain/DBVSO.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "MapInfoController.h"
#include "CoastState.h"

#include <cstdint>

#define POSITION_CHANGED		0x01
#define DIRECTION_CHANGED		0x02
#define LINK_CHANGED				0x04
#define PLAYER_CHANGED			0x08
#define FRAME_INDEX_CHANGED	0x10


namespace NMapInfoEditor
{
	extern const uint32_t BRUSH_COLOR;
	extern const float BRUSH_RADIUS;
	extern const int BRUSH_PARTS;
	//
	extern const uint32_t PLACEMENT_COLOR;
	extern const float PLACEMENT_RADIUS0;
	extern const float PLACEMENT_RADIUS1;
	extern const float PLACEMENT_DIRECTION_RADIUS;
	extern const int PLACEMENT_PARTS;

	extern const uint32_t SCENE_OBJECT_SELECTION_COLOR;
	extern const float SCENE_OBJECT_SELECTION_RADIUS0;
	extern const float SCENE_OBJECT_SELECTION_RADIUS1;
	extern const float SCENE_OBJECT_DIRECTION_RADIUS;
	extern const int SCENE_OBJECT_SELECTION_PARTS;
	//
	extern const uint32_t OBJECT_SELECTION_COLOR;
	extern const float OBJECT_SELECTION_RADIUS0;
	extern const float OBJECT_SELECTION_RADIUS1;
	extern const float OBJECT_DIRECTION_RADIUS;
	extern const int OBJECT_SELECTION_PARTS;
	//
	extern const uint32_t OBJECT_LINK_COLOR;
	extern const float OBJECT_LINK_RADIUS0;
	extern const float OBJECT_LINK_RADIUS1;
	extern const int OBJECT_LINK_PARTS;
	//
	extern const uint32_t MAIN_OBJECT_SELECTION_COLOR;
	extern const float MAIN_OBJECT_SELECTION_RADIUS0;
	extern const float MAIN_OBJECT_SELECTION_RADIUS1;
	extern const float MAIN_OBJECT_DIRECTION_RADIUS;
	extern const int MAIN_OBJECT_SELECTION_PARTS;
	//
	extern const uint32_t SELECTION_COLOR;
	extern const float SELECTION_RADIUS0;
	extern const float SELECTION_RADIUS1;
	extern const float DIRECTION_RADIUS;
	extern const float SELECTION_POINT_RADIUS;
	extern const int SELECTION_PARTS;
	extern const int SELECTION_POINT_PARTS;
	//
	extern const float HEIGHT_DELIMITER;
	//
	extern const uint32_t DRAW_SELECTION_CIRCLE0;
	extern const uint32_t DRAW_SELECTION_CIRCLE1;
	extern const uint32_t DRAW_SELECTION_POINT_CIRCLE;
	extern const uint32_t DRAW_DIRECTION;
	extern const uint32_t DRAW_DIRECTION_POINT;
	extern const uint32_t DRAW_ALL;
	//
	extern const uint32_t PARCEL_COLOR_UNKNOWN;
	extern const uint32_t PARCEL_COLOR_DEFENCE;
	extern const uint32_t PARCEL_COLOR_REINFORCE;
	extern const float PARCEL_REINFORCE_RAD;
	extern const int PARCEL_PARTS;
	extern const int PARCEL_POINT_PARTS;
	//
	extern const float SCENE_FADE_COEFFICIENT;
	extern const float SCENE_PASTE_OPACITY;
	extern const float SCENE_NORMAL_OPACITY;

	typedef vector<CVec3> CSelectionSquare;
	typedef list<int> CSceneIDList;
	typedef list<int> CIndicesList;
	typedef hash_map<unsigned, int> CObjectInfoIDSet;
	typedef vector<unsigned> CLinkIDList;
	typedef hash_map<unsigned, unsigned> CLinkIDMap;

	enum EObjectInfoType
	{
		OIT_SIMPLE_OBJECT		= 0,
		OIT_BRIDGE					= 1,
		OIT_ENTRENCHMENT		= 2,
		OIT_SPOT						= 3,
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SControllerChangeInfo
	{
		int nIndex;
		unsigned nFlags;

		SControllerChangeInfo() : nIndex( INVALID_NODE_ID ), nFlags( 0 ) {}
		SControllerChangeInfo( int _nIndex ) : nIndex( _nIndex ), nFlags( 0 ) {}
		SControllerChangeInfo( const SControllerChangeInfo &rControllerChangeInfo )
			: nIndex( rControllerChangeInfo.nIndex ),
				nFlags( rControllerChangeInfo.nFlags ) {}
		SControllerChangeInfo& operator=( const SControllerChangeInfo &rControllerChangeInfo )
		{
			if( &rControllerChangeInfo != this )
			{
				nIndex = rControllerChangeInfo.nIndex;
				nFlags = rControllerChangeInfo.nFlags;
			}
			return *this;
		}	
	};
	typedef list<NMapInfoEditor::SControllerChangeInfo> CControllerChangeInfoList;
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SObjectLoadInfo
	{
		int nObjectIndex;
		unsigned nLinkID;
		unsigned nLinkWith;
		bool bSearchIndices;
		//
		bool bAdditionalDataFilled;
		string szRPGStatsTypeName;
		CDBID rpgStatsDBID;
		CVec3 vPosition;
		float fDirection;
		unsigned nFrameIndex;
		unsigned nPlayer;
		float fHP;
		//
		SObjectLoadInfo()
			:	nObjectIndex( INVALID_NODE_ID ),
				nLinkID( INVALID_NODE_ID ),
				nLinkWith( INVALID_NODE_ID ),
				bSearchIndices( true ),
				//
				bAdditionalDataFilled( false ),
				vPosition ( VNULL3 ),
				fDirection( 0.0f ),
				nFrameIndex( -1 ),
				nPlayer( 0 ),
				fHP( 1.0f ) {}
		virtual ~SObjectLoadInfo() {}	
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SObjectCreateInfo
	{
		CVec3 vPosition;
		float fDirection;
		//
		string szRPGStatsTypeName;
		CDBID rpgStatsDBID;
		//
		unsigned nFrameIndex;
		unsigned nPlayer;
		float fHP;
		//
		bool bFitToGrid;
		bool bRotateTo90Degree;
		//
		SObjectCreateInfo()
			: vPosition( VNULL3 ),
				fDirection( 0.0f ),
				//
				nFrameIndex( -1 ),
				nPlayer( 0 ), 
				fHP( 1.0f ),
				//
				bFitToGrid( false ),
				bRotateTo90Degree( false ) {}

		virtual ~SObjectCreateInfo() {}	
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SObjectEditInfo
	{
		bool bFitToGrid;
		bool bRotateTo90Degree;
		//
		virtual ~SObjectEditInfo() {}	
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef vector<NDb::SVSOInstance> CVSOInstanceList;
	struct SVSOCollector
	{
		CVSOInstanceList roadList;
		CVSOInstanceList riverList;
		CVSOInstanceList cragList;
		CVSOInstanceList lakeList;
		bool bHasCoast;
		CVec3 vCoastMidPoint;
		//
		NDb::SVSOInstance coast;
		//
		void Load( const NDb::SMapInfo *pMapInfo )
		{
			if ( pMapInfo != 0 )
			{
				roadList = pMapInfo->roads;	
				riverList = pMapInfo->rivers;	
				cragList = pMapInfo->crags;	
				lakeList = pMapInfo->lakes;	
				coast = pMapInfo->coast;
				bHasCoast = pMapInfo->bHasCoast;
				vCoastMidPoint = pMapInfo->vCoastMidPoint;
			}
		}
		//
		CVSOInstanceList* GetVSOList( CMapInfoController::SVSOUndoData::EVSOType eVSOType )
		{
			switch ( eVSOType )
			{
				case CMapInfoController::SVSOUndoData::VSO_ROAD:
					return &roadList;
				case CMapInfoController::SVSOUndoData::VSO_RIVER:
					return &riverList;
				case CMapInfoController::SVSOUndoData::VSO_CRAG:
					return &cragList;
				case CMapInfoController::SVSOUndoData::VSO_LAKE:
					return &lakeList;
			}
			return 0;
		}
		//
		bool InsertVSO( CMapInfoController::SVSOUndoData::EVSOType eVSOType, const NDb::SVSOInstance &rNewVSO, const NDb::SMapInfo *pMapInfo )
		{
			if ( CVSOInstanceList *pVSOInstanceList = GetVSOList( eVSOType ) )
			{
				pVSOInstanceList->insert( pVSOInstanceList->end(), rNewVSO );
				return true;
			}
			else if ( eVSOType == CMapInfoController::SVSOUndoData::VSO_COAST )
			{
				coast = rNewVSO;
				CCoastState::GetWaterPos( pMapInfo, coast.points, &( vCoastMidPoint ) );
				return true;
			}
			return false;
		}
		//
		bool UpdateVSO( CMapInfoController::SVSOUndoData::EVSOType eVSOType, const NDb::SVSOInstance &rNewVSO, const NDb::SMapInfo *pMapInfo )
		{
			if ( CVSOInstanceList *pVSOInstanceList = GetVSOList( eVSOType ) )
			{
				for ( int nVSOIndex = 0; nVSOIndex < pVSOInstanceList->size(); ++nVSOIndex )
				{
					if ( ( *pVSOInstanceList )[nVSOIndex].nVSOID == rNewVSO.nVSOID )
					{
						( *pVSOInstanceList )[nVSOIndex] = rNewVSO;
						return true;
					}
				}
			}
			else if ( eVSOType == CMapInfoController::SVSOUndoData::VSO_COAST )
			{
				coast = rNewVSO;
				CCoastState::GetWaterPos( pMapInfo, coast.points, &( vCoastMidPoint ) );
				return true;
			}
			return false;
		}
		//
		bool RemoveVSO( CMapInfoController::SVSOUndoData::EVSOType eVSOType, int nVSOID )
		{	
			if ( CVSOInstanceList *pVSOInstanceList = GetVSOList( eVSOType ) )
			{
				for ( int nVSOIndex = 0; nVSOIndex < pVSOInstanceList->size(); ++nVSOIndex )
				{
					if ( ( *pVSOInstanceList )[nVSOIndex].nVSOID == nVSOID )
					{
						pVSOInstanceList->erase( pVSOInstanceList->begin() + nVSOIndex );
						return true;
					}
				}
			}
			else if ( eVSOType == CMapInfoController::SVSOUndoData::VSO_COAST )
			{
				coast.controlPoints.clear();
				coast.points.clear();
				coast.pDescriptor = 0;
				coast.nVSOID = INVALID_NODE_ID;
				bHasCoast = false;
				vCoastMidPoint = VNULL3;
				return true;
			}
			return false;
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool CheckLinkCapability( const string &rszObjectRPGStatsTypeName, const CDBID &rObjectRPGStatsDBID, unsigned nFrameIndex,
														const string &rszLinkToObjectRPGStatsTypeName, const CDBID &rLinkToObjectRPGStatsID, unsigned nLinkToFrameIndex );
};


