#include "StdAfx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\mapeditorlib\commoneditormethods.h"
#include "..\sceneb2\scene.h"
#include "..\main\gametimer.h"
#include "simpleobjectinfodata.h"
#include <float.h>
#include "ResourceDefines.h"
#include "../libdb/ResourceManager.h"
#include "EditorMethods.h"
#include "MapObjectMultiState.h"
#include "FenceState.h"
#include "MapInfoEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::ClearData()
{
	designTool.Clear();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::InsertFence()
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		CWaitCursor wcur;

		CPtr<IEditorScene> pScene = EditorScene();
		CPtr<IManipulator> pManipulator = GetMapInfoEditor()->GetViewManipulator();
		CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController();

		if ( ( !pScene ) || ( !pManipulator ) || ( !pObjectController ) )
		{
			return;
		}
		ClearScene();
		if ( selectedFenceInfo.pFenceRPGStats == 0 )
		{
			return;
		}
		vector<CFenceDesignTool::SFenceSectionInfo> segInfo;
		designTool.GetSectionsInfo( &segInfo, selectedFenceInfo.pFenceRPGStats->ePlacementType ); 
		if ( segInfo.empty() )
		{
			return;
		}
		if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
		{
			pEditParameters->nFlags = MIMOSEP_PLAYER_INDEX;
			GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			//
			for ( vector<CFenceDesignTool::SFenceSectionInfo>::iterator itTE = segInfo.begin(); itTE!= segInfo.end(); ++itTE )
			{
				CFenceDesignTool::SFenceSectionInfo *pSectionInfo = &(*itTE);
				pSectionInfo->vPosition.z = 0.0f;
				//
				// записываем секцию забора в базу и добавляем во внутренние структуры CMapInfoEditor
				NMapInfoEditor::SObjectCreateInfo objectCreateInfo;
				objectCreateInfo.vPosition = pSectionInfo->vPosition;
				objectCreateInfo.fDirection = pSectionInfo->fDirection;
				objectCreateInfo.szRPGStatsTypeName = selectedFenceInfo.szRPGStatsTypeName;
				objectCreateInfo.rpgStatsDBID = selectedFenceInfo.rpgStatsDBID;
				objectCreateInfo.nFrameIndex = 0;
				objectCreateInfo.nPlayer = pEditParameters->nPlayerIndex;
				objectCreateInfo.fHP = 1.0f;
				objectCreateInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
				objectCreateInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
				//
				UINT nSimpleObjectInfoID = INVALID_NODE_ID;
				if ( NMapInfoEditor::SSimpleObjectInfo *pSimpleObjectInfo = GetMapInfoEditor()->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSimpleObjectInfo*>( 0 ), &nSimpleObjectInfoID ) )
				{
					if ( !pSimpleObjectInfo->Create( &objectCreateInfo, pScene, pObjectController, pManipulator ) )
					{
						pObjectController->Undo( true, false, GetMapInfoEditor() );
						break;
					}
				}
			}
			//
			pObjectController->Redo( false, true, GetMapInfoEditor() );
			Singleton<IControllerContainer>()->Add( pObjectController );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	if ( CMapObjectState::HandleCommand( nCommandID, dwData ) )
	{
		return true;
	}
	//
	switch( nCommandID ) 
	{
	case ID_TOOLS_FIT_TO_GRID:
		{ // изменена настройка редактора FitToAIGrid
			designTool.Clear();
			ClearData();
			ClearScene();
			RefreshSelectedFenceInfo();
			designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		return true;
	default:
		return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CFenceState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CFenceState::UpdateCommand(), pbCheck == 0" );
	//
	if ( CMapObjectState::UpdateCommand( nCommandID, pbEnable, pbCheck ) )
	{
		return true;
	}
	//
	switch( nCommandID ) 
	{
	case ID_TOOLS_FIT_TO_GRID:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::RefreshSelectedFenceInfo()
{
	CWaitCursor waitCursor;
	selectedFenceInfo = SSelectedFenceInfo();
	//
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		bool bRes = ( objectSet.szObjectTypeName == "FenceRPGStats" ) && ( !objectSet.objectNameSet.empty() );
		if ( !bRes ) 
		{
			return;
		}
		selectedFenceInfo.szRPGStatsTypeName = objectSet.szObjectTypeName;
		selectedFenceInfo.rpgStatsDBID = objectSet.objectNameSet.begin()->first;
		selectedFenceInfo.pFenceRPGStats = dynamic_cast<const NDb::SFenceRPGStats*>( NDb::GetObject( selectedFenceInfo.rpgStatsDBID ) );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::InsertObjectEnter()
{
	sceneDrawTool.Clear();
	ClearData();
	ClearScene();
	//
	RefreshSelectedFenceInfo();
	designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_FENCE_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::InsertObjectLeave()
{
	sceneDrawTool.Clear();
	ClearData();
	ClearScene();
	//
	GetObjectInfoCollector()->ClearSelection();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_FENCE_STATE );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::InsertObjectDraw( CPaintDC *pPaintDC )
{
	if ( !designTool.bRay )
	{
		CVec3 vPlacementPosition = pStoreInputState->lastEventInfo.vTerrainPos;
		vPlacementPosition.z = GetTerrainHeight( vPlacementPosition.x, vPlacementPosition.y );
		CVec3 vPlacementSelectorPosition = vPlacementPosition;
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS0, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
		sceneDrawTool.DrawCircle( vPlacementSelectorPosition, NMapInfoEditor::PLACEMENT_RADIUS1, NMapInfoEditor::PLACEMENT_PARTS, NMapInfoEditor::PLACEMENT_COLOR, false );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::FillScene( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		ClearScene();
		//
		if ( selectedFenceInfo.pFenceRPGStats == 0 )
		{
			return;
		}
		//
		if ( IEditorScene *pScene = EditorScene() )
		{
			vector<CFenceDesignTool::SFenceSectionInfo> sectionsInfo;
			designTool.GetSectionsInfo( &sectionsInfo, selectedFenceInfo.pFenceRPGStats->ePlacementType );
			//
			if ( !sectionsInfo.empty() )
			{
				for ( vector<CFenceDesignTool::SFenceSectionInfo>::iterator it = sectionsInfo.begin(); it != sectionsInfo.end(); ++it )
				{
					CFenceDesignTool::SFenceSectionInfo *pSectionInfo = it;
					pSectionInfo->vPosition.z = GetTerrainHeight( pSectionInfo->vPosition.x, pSectionInfo->vPosition.y );

					DWORD dwNormal = Vec3ToDWORD( V3_AXIS_Z );
					CQuat qRotation = CQuat( pSectionInfo->fDirection, V3_AXIS_Z );
					if ( SSimpleObjectInfo::NeedMakeOrientation( selectedFenceInfo.szRPGStatsTypeName, selectedFenceInfo.rpgStatsDBID ) )
					{
						dwNormal = EditorScene()->GetNormal( CVec2( pSectionInfo->vPosition.x, pSectionInfo->vPosition.y ) );
						MakeOrientation( &qRotation, DWORDToVec3( dwNormal ) );
					}
					//
					const UINT nObjectSceneID = GetMapInfoEditor()->objectInfoCollector.sceneIDCollector.LockID();
					if ( CPtr<SAINewUnitUpdate> pUpdate = new SAINewUnitUpdate() )
					{
						pUpdate->eUpdateType = ACTION_NOTIFY_NEW_ST_OBJ;
						pUpdate->info.nObjUniqueID = nObjectSceneID;
						pUpdate->info.pStats = NDb::Get<NDb::SHPObjectRPGStats>( selectedFenceInfo.rpgStatsDBID );
						pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
						pUpdate->info.bNewFormat = true;
						pUpdate->info.vPlacement = pSectionInfo->vPosition;
						pUpdate->info.rotation = qRotation;
						//
						pUpdate->info.center = CVec2( pSectionInfo->vPosition.x, pSectionInfo->vPosition.y );
						pUpdate->info.z = pSectionInfo->vPosition.z;
						pUpdate->info.dir = Vis2AIRad( pSectionInfo->fDirection );
						pUpdate->info.dwNormal = dwNormal;
						//
						pUpdate->info.fSpeed = 0.0f;
						pUpdate->info.cSoil = 0;
						pUpdate->info.fResize = 1.0f;
						pUpdate->info.fHitPoints = selectedFenceInfo.pFenceRPGStats->fMaxHP;
						pUpdate->info.fFuel = 1.0f;
						pUpdate->info.eDipl = EDI_FRIEND;
						pUpdate->info.nPlayer = pEditParameters->nPlayerIndex;
						pUpdate->info.nFrameIndex = 0;
						pUpdate->info.nExpLevel = 0;
						//
						GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
					}
					tmpSceneObjectsIDList.push_back( nObjectSceneID );
				}
			}
			pScene->SetFadedObjects( tmpSceneObjectsIDList, NMapInfoEditor::SCENE_FADE_COEFFICIENT );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		IEditorScene *pScene = EditorScene();
		if ( !pScene )
		{
			return false;
		}
		designTool.ProcessMovePoint( rTerrainPos ); 
		FillScene( nFlags, rTerrainPos );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::InsertObjectLButtonDown( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( !designTool.bRay )
	{
		designTool.ProcessLClickPoint( rTerrainPos ); 
		designTool.ProcessMovePoint( rTerrainPos );
		FillScene( nFlags, rTerrainPos );
	}
	else
	{
		InsertFence();
		CVec3 vLastPos = designTool.vLastPos;
		//
		ClearData();
		ClearScene();
		//
		RefreshSelectedFenceInfo();
		designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
		//
		designTool.ProcessLClickPoint( vLastPos ); 
		designTool.ProcessMovePoint( vLastPos );
		FillScene( nFlags, rTerrainPos );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( designTool.bRay )
	{
		ClearData();
		ClearScene();
		//
		RefreshSelectedFenceInfo();
		designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		return false;
	}
	else
	{
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceState::InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		if ( nChar == VK_ESCAPE )
		{
			if ( designTool.bRay )
			{
				ClearData();
				ClearScene();
				//
				RefreshSelectedFenceInfo();
				designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
			else
			{
				return true;
			}
		}
		//
		if ( nChar == VK_RETURN )
		{
			if ( designTool.bRay )
			{
				InsertFence();
				CVec3 vLastPos = designTool.vLastPos;
				//
				ClearData();
				ClearScene();
				//
				RefreshSelectedFenceInfo();
				designTool.SetUp( GetMapInfoEditor()->editorSettings.bFitToGrid, selectedFenceInfo.pFenceRPGStats );
				//
				designTool.ProcessLClickPoint( vLastPos ); 
				designTool.ProcessMovePoint( vLastPos );
				FillScene( nFlags, rTerrainPos );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceState::ClearScene()
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( !tmpSceneObjectsIDList.empty() )
		{
			for ( NMapInfoEditor::CSceneIDList::const_iterator itSceneID = tmpSceneObjectsIDList.begin(); itSceneID != tmpSceneObjectsIDList.end(); ++itSceneID )
			{
				if ( CPtr<SAIDissapearObjUpdate> pUpdate = new SAIDissapearObjUpdate() )
				{
					pUpdate->eUpdateType = ACTION_NOTIFY_DISSAPEAR_OBJ;
					pUpdate->nUpdateTime = Singleton<IGameTimer>()->GetGameTime();
					pUpdate->nDissapearObjID = *itSceneID;
					pUpdate->bShowEffects = false;
					//
					GetMapInfoEditor()->objectInfoCollector.pEditorUpdatableWorld->ProcessEditorUpdate( pUpdate );
				}
			}
			tmpSceneObjectsIDList.clear();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//				FENCE DESIGN TOOL
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CVec3 AlignToAIGridCellCenter( const CVec3 &p )
{
	const float F_AI_TILE_SIZE = Vis2AI( float(VIS_TILE_SIZE / AI_TILES_IN_VIS_TILE) );
	const float F_AI_TILE_SIZE_2 = F_AI_TILE_SIZE / 2.0f;

	CVec3 r = p;

	r /= F_AI_TILE_SIZE;
	r.x = int( r.x );
	r.y = int( r.y );
	r *= F_AI_TILE_SIZE;
	r.x += F_AI_TILE_SIZE_2;
	r.y += F_AI_TILE_SIZE_2;

	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CVec3 CFenceDesignTool::GetFencePlace( const CVec3 &p )
{
	if ( !bFitToAIGrid )
		return p;

	return AlignToAIGridCellCenter( p );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceDesignTool::SetUp( bool _bFitToAIGrid, const NDb::SFenceRPGStats *pStats )
{
	if ( !pStats )
		return;
	//
	bEnabled = true;
	bComplete = false;
	bRay = false;
	vRayOrigin = VNULL3;
	fRayDir = 0;
	nNumSections = 0;
	bFitToAIGrid = _bFitToAIGrid;
	//
	fSectionLen = AI_TILE_SIZE * AI_TILES_IN_VIS_TILE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceDesignTool::Clear()
{
	bComplete = false;
	bRay = false;
	vRayOrigin = VNULL3;
	fRayDir = 0;
	nNumSections = 0;
	bFitToAIGrid = false;
	fSectionLen = 0;
	bEnabled = false;
	vLastPos = VNULL3;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceDesignTool::ProcessLClickPoint( const CVec3 &p )
{
	if ( !bEnabled || bComplete )
	{
		return false;
	}
	if ( !bRay )
	{
		bRay = true;
		vRayOrigin = GetFencePlace(p);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceDesignTool::ProcessRClickPoint( const CVec3 &p )
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceDesignTool::ProcessEscape()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceDesignTool::ProcessMovePoint( const CVec3 &p )
{
	if ( !bEnabled || bComplete || !bRay )
	{
		return false;
	}
	//
	CVec3 pp = GetFencePlace( p );
	CVec3 dir = p - vRayOrigin;
	float fLen = fabs( dir );
	if ( fLen <= FLT_EPSILON )
	{
		dir = CVec3( 1.0f, 0.0f, 0.0f );
		fLen = 1.0f;
	}
	//
	if ( bFitToAIGrid )
	{
		Normalize( &dir );
		float fExactDir = atan2( dir.y, dir.x );
		int nA = fExactDir / FP_PI4;
		switch ( nA )
		{
			case 0:
				fExactDir = 0;
				break;
			case 1:
			case 2:
				fExactDir = FP_PI2;
				break;
			case 3:
				fExactDir = FP_PI;
			case -3:
				fExactDir = -FP_PI;
				break;
			case -2:
			case -1:
				fExactDir = -FP_PI2;
				break;
		}
		float fADiff = ADiff(fExactDir,fRayDir);
		fRayDir = fExactDir;
		nNumSections = ( fabs( fLen * cos( fADiff ) ) ) / fSectionLen;
		if ( nNumSections == 0 )
		{
			nNumSections = 1;
		}
	}
	else
	{
		Normalize( &dir );
		fRayDir = atan2( dir.y, dir.x );
		nNumSections = fLen / fSectionLen;
		if ( nNumSections == 0 )
		{
			nNumSections = 1;
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceDesignTool::Complete()
{
	if ( !bEnabled )
	{
		return;
	}

	if ( nNumSections == 0 )
	{
		nNumSections = 1; // одиночный кусок забора
	}
	bComplete = true;		
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFenceDesignTool::IsComplete()
{
	if ( !bEnabled )
		return false;

	return bComplete;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFenceDesignTool::GetSectionsInfo( vector<SFenceSectionInfo> *pSectionInfo, 
																			  NDb::SFenceRPGStats::EFencePlacementMode ePlacementMode )
{
	if ( ( !bEnabled ) || ( !bRay ) )
	{
		return;
	}
	pSectionInfo->clear();
	CQuat q( fRayDir, V3_AXIS_Z );
	CVec3 dir = V3_AXIS_X;
	q.Rotate( &dir, dir );

	CVec3 cp = vRayOrigin;
	for ( int i = 0; i < nNumSections; ++i )
	{
		if ( bFitToAIGrid )
		{
			cp = AlignToAIGridCellCenter( cp );
		}
		//
		SFenceSectionInfo segInfo;
		// fRayDir - угол отсчитывается против часовой стрелки от оси OX
		// fDirection - AI угол - нужно отсчитывать от OY
		segInfo.fDirection = fRayDir - FP_PI2; 
		//
		if ( ePlacementMode == NDb::SFenceRPGStats::FENCE_PLACE_STAGGERED )
		{
			segInfo.vPosition = cp;
			segInfo.vPosition.z = 0;
		}
		else if ( ePlacementMode == NDb::SFenceRPGStats::FENCE_PLACE_ON_TERRAIN )
		{
			segInfo.vPosition = cp;
			segInfo.vPosition.z = 0;
		}
		pSectionInfo->push_back( segInfo );
		//
		cp += ( dir * fSectionLen );
	}
	vLastPos = cp;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
