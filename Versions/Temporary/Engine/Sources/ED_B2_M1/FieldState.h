#if !defined( __FIELD_STATE__ )
#define __FIELD_STATE__
#pragma once

#include "PolygonState.h"
#include "DBField.h"
#include "HeightPattern.h"
#include "..\MapEditorLib\WV_Types.h"
#include "MapInfoEditorData.h"
#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "CommandHandlerDefines.h"

//Mapinfo terrain field state edit parameters
#define MITFEP_MOVE_TYPE		0x00000001
#define MITFEP_FIELD_COUNT	0x00000002
#define MITFEP_FIELD_INDEX	0x00000004
#define MITFEP_RANDOMIZE		0x00000008
#define MITFEP_MIN_LENGTH		0x00000010
#define MITFEP_WIDTH				0x00000020
#define MITFEP_DISTURBANCE	0x00000040
#define MITFEP_FILL_TERRAIN	0x00000080
#define MITFEP_FILL_OBJECTS	0x00000100
#define MITFEP_FILL_HEIGHTS	0x00000200
#define MITFEP_UPDATE_MAP		0x00000400
#define MITFEP_ALL					0xFFFFFFFF


class CFieldState : public CPolygonState
{
	// Friend классы
	friend class CMultiInputState;
	friend class CMapInfoState;
	friend class CFieldWindow;

	// константы
	static const char FIELD_TYPE_NAME[];

public:
	// Структура данных, с помощью которой конфигурационный диалог общается со стейтом
	struct SEditParameters
	{
		typedef vector<string> CFieldList;

		UINT nFlags;
		//
		EMoveType eMoveType;
		CFieldList fieldList;
		int nFieldIndex;
		float fMinLength;
		float fWidth;
		float fDisturbance;
		bool bRandomize;
		bool bFillTerrain;
		bool bFillObjects;
		bool bFillHeights;
		bool bUpdateMap;
		//
		SEditParameters()
			:	nFlags( MITFEP_ALL ),
				eMoveType( MT_SINGLE ),
				nFieldIndex( 0 ),
				fMinLength( 10.0f ),
				fWidth( 0.5f ),
				fDisturbance( 0.2f ),
				bRandomize( true ),
				bFillTerrain( true ),
				bFillObjects( true ),
				bFillHeights( true ),
				bUpdateMap( false ) {}
		SEditParameters( const SEditParameters &rEditParameters )
			:	nFlags( rEditParameters.nFlags ),
				eMoveType( rEditParameters.eMoveType ),
				fieldList( rEditParameters.fieldList ),
				nFieldIndex( rEditParameters.nFieldIndex ),
				fMinLength( rEditParameters.fMinLength ),
				fWidth( rEditParameters.fWidth ),
				fDisturbance( rEditParameters.fDisturbance ),
				bRandomize( rEditParameters.bRandomize ),
				bFillTerrain( rEditParameters.bFillTerrain ),
				bFillObjects( rEditParameters.bFillObjects ),
				bFillHeights( rEditParameters.bFillHeights ),
				bUpdateMap( rEditParameters.bUpdateMap ) {}
		SEditParameters& operator=( const SEditParameters &rEditParameters )
		{
			if( &rEditParameters != this )
			{
				nFlags = rEditParameters.nFlags;
				eMoveType = rEditParameters.eMoveType;
				fieldList = rEditParameters.fieldList;
				nFieldIndex = rEditParameters.nFieldIndex;
				fMinLength = rEditParameters.fMinLength;
				fWidth = rEditParameters.fWidth;
				fDisturbance = rEditParameters.fDisturbance;
				bRandomize = rEditParameters.bRandomize;
				bFillTerrain = rEditParameters.bFillTerrain;
				bFillObjects = rEditParameters.bFillObjects;
				bFillHeights = rEditParameters.bFillHeights;
				bUpdateMap = rEditParameters.bUpdateMap;
			}
			return *this;
		}

		int operator&( IXmlSaver &xs );
	};
	//
private:
	class CMapInfoEditor *pMapInfoEditor;
	CControlPointList controlPointList;

	SEditParameters* GetEditParameters();
	void UpdateEditParameters( UINT nFlags );

	typedef list<CVec2> CFieldPolygon;
	typedef vector<int> CXPosList;
	typedef hash_map<LPARAM, float> CFieldDistanceMap;
	typedef NWV::CWeightVector<int, NWV::SClientRandom> CTileSetWeightVector;
	typedef	vector<CTileSetWeightVector> CTileSetWeightVectorList;
	typedef NWV::CWeightVector<CDBPtr<NDb::SHPObjectRPGStats>, NWV::SClientRandom> CObjectSetWeightVector;
	typedef	vector<CObjectSetWeightVector> CObjectSetWeightVectorList;

	static void CreateTileSetWeightVectorList( CTileSetWeightVectorList *pTileSetWeightVectorList, const NDb::SField &rField );
	static void CreateObjectSetWeightVectorList( CObjectSetWeightVectorList *pObjectSetWeightVectorList, const NDb::SField &rField );
	static int GetPolygonLine( int nYPos, float fSide, const CFieldPolygon &rPolygon, const CTPoint<int> &rXBounds, CXPosList *pXPosList );
	static bool FillTileSet( CArray2D<BYTE> *pTile2DArray,
													 CTPoint<int> *pStartTile,
													 const NDb::SField &rField,
													 const CFieldPolygon &rPolygon,
													 const CTPoint<int> terrainSize,
													 float fTileSize,
													 CFieldDistanceMap *pDistances );
	static bool FillProfilePattern( SHeightPattern *pHeightPattern,
																	const NDb::SField &rField,
																	const CFieldPolygon &rPolygon,
																	const CTPoint<int> terrainSize,
																	float fTileSize,
																	CFieldDistanceMap *pDistances );
	static bool FillObjectSet( CMapInfoEditor *pMapInfoEditor,
														 CObjectBaseController *pObjectController,
														 const NDb::SField &rField,
														 const CFieldPolygon &rPolygon,
														 const CTPoint<int> terrainSize,
														 float fTileSize,
														 CFieldDistanceMap *pDistances,
														 CArray2D<BYTE> *pTileMap );

public:
	//IInputState interface
	void Enter();
	void Leave();

	// CPolygonState
	bool SkipEnterAfterInsert() { return false; }
	bool CanEdit();
	bool CanInsertPolygon() { return true; }
	bool IsClosedPolygon() { return true; }
	bool IsDrawSceneDrawTool() { return true; }
	EMoveType GetMoveType();
	void GetBounds( int *pnMinCount, int *pnMaxCount );
	CControlPointList* GetControlPoints( int nPolygonID ) { return &controlPointList; }
	bool PrepareControlPoints( CControlPointList *pControlPointList );
	void PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList ) {}
	void UpdatePolygon( int nPolygonID, EUpdateType eEpdateType );
	UINT InsertPolygon( const CControlPointList &rControlPointList ) { controlPointList = rControlPointList; return 0; }
	void RemovePolygon( int nPolygonID ) {}

	//
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	//
	// CFieldState
	CFieldState() : pMapInfoEditor( 0 )
	{
		NI_ASSERT( pMapInfoEditor != 0, "CFieldState(): Invalid parameter: pMapInfoEditor == 0" );
		Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_FIELD_STATE, this );
	}
	CFieldState( CMapInfoEditor *_pMapInfoEditor ) : pMapInfoEditor( _pMapInfoEditor )
	{
		NI_ASSERT( pMapInfoEditor != 0, "CFieldState(): Invalid parameter: pMapInfoEditor == 0" );
		Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_TERRAIN_FIELD_STATE, this );
	}
	~CFieldState()
	{
		Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_TERRAIN_FIELD_STATE );
	}
};

#endif // #if !defined( __FIELD_STATE__ )

