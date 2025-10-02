#if !defined( __ADV_CLIPBOARD_STATE__ )
#define __ADV_CLIPBOARD_STATE__
#pragma once
/**

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\sceneb2\scene.h"
#include "../libdb/resourcemanager.h"
#include "PolygonState.h"
#include "AdvClipboardWindow.h"
#include "MapClip.h"

//
//
// ADVANCED CLIPBOARD PASTE MARKER
//
//

struct SClipboardObjectInfo;
struct SClipboardEntrenchment;
struct SACBMarker : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( SACBMarker );

public:
	vector<int> sceneIDs;
	//
	SACBMarker() {}
	virtual ~SACBMarker()
	{
		Clear();
	}
	//
	void Clear()
	{
		CPtr<IEditorScene> pScene = EditorScene();
		if ( !pScene )
			return;
		for ( int i = 0; i < sceneIDs.size(); ++i )
		{
			int nSceneID = sceneIDs[i];
			pScene->RemoveObject( nSceneID ); 
		}
		sceneIDs.clear();
	}
	//
	void CreateClipMarker( const CMapClip &rMapClip, const SAdvClipboardPasteSettings &pasteSettings, const CVec3 &vPasteCenter )
	{
		// создают полупрозрачные модели объектов для вставки
		//
		CPtr<IEditorScene > pScene = EditorScene();
		if ( !pScene )
			return;

		if ( rMapClip.GetObjNum() == 0 )
			return;

		sceneIDs.resize( rMapClip.GetObjNum() );
		for ( int i = 0; i < sceneIDs.size(); ++i )
			sceneIDs[i] = -1;

		for ( int i = 0; i < rMapClip.GetObjNum(); ++i )
		{
			const SClipboardObjectInfo &oi = (*rMapClip.GetObj(i));

			if ( oi.szRPGStatsTypeName == "MechUnitRPGStats" && pasteSettings.bPasteUnitsSquads )
			{
			}
			else if ( oi.szRPGStatsTypeName == "SquadRPGStats" && pasteSettings.bPasteUnitsSquads )
			{
			}
			else if ( oi.szRPGStatsTypeName == "BuildingRPGStats" && pasteSettings.bPasteBuildings )
			{
			}
			else if ( oi.szRPGStatsTypeName == "ObjectRPGStats" && pasteSettings.bPasteObjectsFencesEntrenchmentes )
			{
			}
			else if ( oi.szRPGStatsTypeName == "FenceRPGStats" && pasteSettings.bPasteObjectsFencesEntrenchmentes )
			{
			}
			else
			{
				continue;
			}

			CVec3 vObjectScenePosition = oi.vPosition + vPasteCenter;
			CQuat qObjectSceneRotation = CQuat( oi.fDirection, V3_AXIS_Z );

			const NDb::SModel *pModel = GetModel( oi );

			if ( !pModel )
				continue;

			int nObjectSceneID = pScene->AddObject(INVALID_NODE_ID, pModel, vObjectScenePosition, qObjectSceneRotation, CVec3( 1, 1, 1 ), OBJ_ANIM_MODE_DEFAULT, 0 );

			sceneIDs[i] = nObjectSceneID;

			//::OutputDebugString( StrFmt( "create: %s %d\n", oi.szName.c_str(), nObjectSceneID ) );
		}

		list<int> tmp;
		for ( int i = 0; i < sceneIDs.size(); ++i )
		{
			if ( sceneIDs[i] != -1 )
				tmp.push_back( sceneIDs[i] );
		}
		pScene->SetFadedObjects( tmp );
	}
	//
	void MoveMarker(  const CMapClip &rMapClip, const CVec3 &vNewCenterPos )
	{
		CPtr<IEditorScene > pScene = EditorScene();
		if ( !pScene )
			return;

		NI_ASSERT( rMapClip.GetObjNum() == sceneIDs.size(), "SACBMarker::MoveMarker(): rMapClip.GetObjNum() != sceneIDs.size()" );
		if ( rMapClip.GetObjNum() != sceneIDs.size() )
			return;

		for ( int i = 0; i < rMapClip.GetObjNum(); ++i )
		{
			const SClipboardObjectInfo *pOi = rMapClip.GetObj(i);
			if ( !pOi )
				continue;

			const SClipboardObjectInfo &oi = (*pOi);

			CVec3 vObjectScenePosition = oi.vPosition + vNewCenterPos;
			CQuat qObjectSceneRotation = CQuat( oi.fDirection, V3_AXIS_Z );

			if ( i < sceneIDs.size() )
			{
				int nSceneID = sceneIDs[i];
				pScene->MoveObject( nSceneID, vObjectScenePosition, qObjectSceneRotation );
				//::OutputDebugString( StrFmt( "move: %s %d\n", oi.szName.c_str(), nSceneID ) );
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	const NDb::SModel* GetModel( const SClipboardObjectInfo &oi )
	{
		CPtr<IResourceManager> pResourceManager = Singleton<IResourceManager>();
		if ( !pResourceManager )
			return 0;

		if ( oi.szRPGStatsTypeName == "FenceRPGStats" )
		{
			const NDb::SFenceRPGStats *pRPGStats = dynamic_cast<const NDb::SFenceRPGStats*>
				( 
				NDb::GetResource( oi.nRPGStatsTypeID, oi.nRPGStatsID ) 
				);
			if ( !pRPGStats )
				return 0;
			if ( !pRPGStats->centerSegments.visObjes.empty() )
			{
				return ::GetModel( pRPGStats->centerSegments.visObjes[0], NDb::SEASON_SUMMER );
			}
			else
				return 0;
		}
		else
		{
			const NDb::SHPObjectRPGStats *pHPObjectRPGStats = dynamic_cast<const NDb::SHPObjectRPGStats*>
				( 
				NDb::GetResource( oi.nRPGStatsTypeID, oi.nRPGStatsID ) 
				);
			if ( !pHPObjectRPGStats )
				return 0;
			return ::GetModel( pHPObjectRPGStats->pvisualObject, NDb::SEASON_SUMMER );
		}
		return 0;
	}
};

//
//
//		ADVANCED CLIPBOARD STATE
//
//

struct SClipboardObjectInfo;
struct SClipboardBridge;
struct SClipboardTerraSpot;
namespace NMapInfoEditor
{
	struct SMapInfo;
}
class CAdvClipboardState : public CPolygonState, public ICommandHandler
{
	CMapInfoEditor *pMapInfoEditor;
	//
	CMapClip gMapClip;
	CPolygonState::CControlPointList cpList;
	bool bCanEdit;
	CVec3 vCurrentPasteCenter;
	SAdvClipboardPasteSettings pasteSettings;
	//
	CPtr<CObjectBaseController> pObjectController;
	//
	NMapInfoEditor::SObjectInfoCollector* GetObjectInfoCollector();
	//
	void ClearData();
	void CopySelection();
	void PasteSelection();
	//
	void PasteObject( const SClipboardObjectInfo &oi );
	bool IsObjectInSelectionRegion( const CVec3 &p );
	CVec3 GetSelectionCenter();
	//
	bool bPasteMode;
	bool SaveMapClipToDB();
	bool ReadMapClipFromDB();
	bool CopyMapObjects( const char *pszTypeName );
	bool CopyTerraHeights();
	bool PasteTerraHeigts();
	bool CopyVSOs( const string &szType, const vector< NDb::SVSOInstance > &mapVsoObjects );
	bool PasteVSO( const string &szType, const NDb::SVSOInstance &vsoInstance );
	bool CopyTerrain();
	bool PasteTerrain();
	bool CopyHeightsCrags();
	bool CopyTerrainTilesSpots();
	bool CopyRiversLakesIslands();
	bool CopyBridgeRoads();
	bool CopyObjectsFencesEntrenchmentes();
	bool CopyBuildings();
	bool CopyUnitsSquads();
	bool PasteHeightsCrags();
	bool PasteVSOs( const char *pszTypeName );
	bool PasteMapObjects( const char *pszTypeName );
	bool PasteTerrainTilesSpots();
	bool PasteRiversLakesIslands();
	bool PasteBridgeRoads();
	bool PasteObjectsFencesEntrenchmentes();
	bool PasteBuildings();
	bool PasteUnitsSquads();
	//
	bool InsertVSOToBaseRoad( int nVSOIndex, const NDb::SVSOInstance &rVSO );
	int InsertRoadVSO( int nRoadDescID, const vector<CVec3> &rControlPointList, const vector<NDb::SVSOPoint> &rPoints );
	void UpdateVisualVSORoad( NDb::SVSOInstance *pVSO, bool bBothEdges );
	//
	bool InsertVSOToBaseRiver( int nVSOIndex, const NDb::SVSOInstance &rVSO );
	int InsertRiverVSO( int nRoadDescID, const vector<CVec3> &rControlPointList, const vector<NDb::SVSOPoint> &rPoints );
	void UpdateVisualVSORiver( NDb::SVSOInstance *pVSO, bool bBothEdges );
	//
	bool InsertVSOToBaseCrag( int nVSOIndex, const NDb::SVSOInstance &rVSO );
	int InsertCragVSO( int nCragDescID, const vector<CVec3> &rControlPointList, const vector<NDb::SVSOPoint> &rPoints );
	void UpdateVisualVSOCrag( NDb::SVSOInstance *pVSO, bool bBothEdges );
	//
	bool InsertVSOToBaseLake( int nVSOIndex, const NDb::SVSOInstance &rVSO );
	int InsertLakeVSO( int nRoadDescID, const vector<CVec3> &rControlPointList, const vector<NDb::SVSOPoint> &rPoints );
	void UpdateVisualVSOLake( NDb::SVSOInstance *pVSO, bool bBothEdges );
	//
	void DrawPasteRegion( CPaintDC *pPaintDC );
	SACBMarker acbMarker;
	void DrawVSOObjects( CPaintDC *pPaintDC );
	void DrawHeights( CPaintDC *pPaintDC );
	void DrawEntrenchmentMarkers( CPaintDC *pPaintDC );
	void DrawBridgeMarkers( CPaintDC *pPaintDC ); 
	void DrawTerraSpotMarkers( CPaintDC *pPaintDC );
	//
	bool CopyEntrenchments();
	bool PasteEntrenchments();
	bool PasteEntrenchment( const SClipboardEntrenchment &trench );
	bool CopyBridges();
	bool PasteBridges();
	bool PasteBridge( const SClipboardBridge &bridge );
	bool CopySpots();
	bool PasteSpots();
	bool PasteSpot( const SClipboardTerraSpot &spot );

public:
	CAdvClipboardState( CMapInfoEditor *pMapInfoEditor = 0 );
	virtual ~CAdvClipboardState() {}

	// IInputStateInterface
	virtual void Enter();
	virtual void Leave();
	virtual void Draw( CPaintDC *pPaintDC );
	void OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnLButtonDblClk( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint );
	void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	void OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint );

	// CPolygonState
	virtual bool CanEdit() { return bCanEdit && !bPasteMode; }
	virtual bool CanInsertPolygon() { return true; }
	virtual bool IsClosedPolygon() { return true; }
	virtual bool IsDrawSceneDrawTool() { return false; }
	virtual EMoveType GetMoveType() { return CPolygonState::MT_MULTI; }
	virtual void GetBounds( int *pnMinCount, int *pnMaxCount );
	virtual CControlPointList* GetControlPoints( int nPolygonID ) { return &cpList; }
	virtual bool PrepareControlPoints( CControlPointList *pControlPointList ) { return true; }
	virtual void PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList ) {}
	virtual void UpdatePolygon( int nPolygonID, EUpdateType eEpdateType );
	virtual UINT InsertPolygon( const CControlPointList &rControlPointList ) { return INVALID_NODE_ID; }
	virtual void RemovePolygon( int nPolygonID );

	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
};

/**/
#endif // #if !defined( __ADV_CLIPBOARD_STATE__ )
