#include "StdAfx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\3dmotor\dbscene.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\sceneb2\scene.h"
#include "../libdb/resourcemanager.h"
#include "..\mapeditorlib\commoneditormethods.h"
#include "ResourceDefines.h"

#include "..\MapEditorLib\Interface_Logger.h"

#include "MapObjectMultiState.h"
#include "SpotState.h"
#include "..\Misc\Win32Random.h"
#include "EditorMethods.h"
#include "MapInfoEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool CSpotState::CanAddSpot()
{
	bool bResult = false;
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		bResult = ( objectSet.szObjectTypeName == "TerrainSpotDesc" ) &&
							( !objectSet.objectNameSet.empty() );
		if ( bResult )
		{
			bResult = false;
			if ( CPtr<IManipulator> pTerrainSpotDescManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
			{
				if ( CPtr<IManipulator> pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Material", pTerrainSpotDescManipulator, 0, 0, 0 ) )
				{
					if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, 0, 0 ) )
					{
						bResult = true;
					}
				}
			}
		}
		if ( !bResult )
		{
			NLog::GetLogger()->Log( LT_ERROR, StrFmt( "Spot have no material: %s%c%d\n", objectSet.szObjectTypeName.c_str(), TYPE_SEPARATOR_CHAR, objectSet.objectNameSet.begin()->first ) );
		}
	}
	return bResult;
}


bool CSpotState::InsertObjectMouseMove( UINT nFlags, const CVec3 &rTerrainPos )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	return false;
}


bool CSpotState::InsertObjectLButtonUp( UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		IEditorScene *pScene = EditorScene();
		ICamera *pCamera = Camera();
		IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pManipulator == 0 ) )
		{
			return false;
		}
		//
		if ( !CanAddSpot() )
		{
			return false;
		}
		//
		SObjectSet objectSet;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) );
		if ( !objectSet.objectNameSet.empty() )
		{
			const float fDirection = ( pEditParameters->fDirection * FP_PI ) / 180.0f;
			pEditParameters->nFlags = MIMOSEP_PLAYER_INDEX;
			GetEditParameters( pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
			NMapInfoEditor::SSpotCreateInfo spotCreateInfo;
			spotCreateInfo.vPosition = rTerrainPos;
			spotCreateInfo.vPosition.z = 0.0f;
			spotCreateInfo.fDirection = fDirection;
			spotCreateInfo.szRPGStatsTypeName = objectSet.szObjectTypeName;
			spotCreateInfo.rpgStatsDBID = objectSet.objectNameSet.begin()->first;
			spotCreateInfo.nFrameIndex = INVALID_NODE_ID;
			spotCreateInfo.nPlayer = pEditParameters->nPlayerIndex;
			spotCreateInfo.bRotateTo90Degree = GetMapInfoEditor()->editorSettings.bRotateTo90Degree;
			spotCreateInfo.bFitToGrid = GetMapInfoEditor()->editorSettings.bFitToGrid;
			spotCreateInfo.spotSquare = terrainSpotInstance.points;
			UINT nSpotInfoID = INVALID_NODE_ID;
			if ( CPtr<CObjectBaseController> pObjectController = GetMapInfoEditor()->CreateController() )
			{
				if ( NMapInfoEditor::SSpotInfo *pSpotInfo = GetMapInfoEditor()->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSpotInfo*>( 0 ), &nSpotInfoID ) )
				{
					if ( pSpotInfo->Create( &spotCreateInfo, pScene, pObjectController, pManipulator ) )
					{
						pObjectController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pObjectController );
					}
					else
					{
						pObjectController->Undo( true, false, GetMapInfoEditor() );
					}
				}
			}
		}
		//
		if ( pEditParameters->eDirectionType == CMapObjectMultiState::SEditParameters::DT_RANDOM )
		{
			pEditParameters->nFlags = MIMOSEP_DIRECTION;
			pEditParameters->fDirection = NWin32Random::Random( 360 ) * 1.0f;
			SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	return false;
}


bool CSpotState::InsertObjectRButtonUp( UINT nFlags, const CVec3 &rTerrainPos )
{
	return true;
}


bool CSpotState::InsertObjectKeyDown( UINT nChar, UINT nFlags, const CVec3 &rTerrainPos )
{
	if ( nChar == VK_ESCAPE )
	{
		return true;
	}
	/**
	else
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	/**/
	return false;
}


void CSpotState::ModifySpotSquare( NDb::STerrainSpotInstance *pTerrainSpotInstance, const CVec3 &rvPosition, float fDirection )
{
	pTerrainSpotInstance->points.resize( 4 );
	const float fScaleCoeff = ( pTerrainSpotInstance->pDescriptor->fScaleCoeff > 0.0f ) ? pTerrainSpotInstance->pDescriptor->fScaleCoeff : 1.0f;
	const CVec2 vSpotSize( pTerrainSpotInstance->pDescriptor->pMaterial->pTexture->nWidth * pTerrainSpotInstance->pDescriptor->fUsedTexSizeX * 2.0f / fScaleCoeff,
												 pTerrainSpotInstance->pDescriptor->pMaterial->pTexture->nHeight * pTerrainSpotInstance->pDescriptor->fUsedTexSizeY * 2.0f / fScaleCoeff ); 
	// Заполняем геометрическую информацию:
	pTerrainSpotInstance->points[0] = CVec2( 0.0f, 0.0f );
	pTerrainSpotInstance->points[1] = CVec2( vSpotSize.x, 0.0f );
	pTerrainSpotInstance->points[2] = CVec2( vSpotSize.x, vSpotSize.y );
	pTerrainSpotInstance->points[3] = CVec2( 0.0f, vSpotSize.y );
	const CVec2 vSpotPos = CVec2( rvPosition.x, rvPosition.y ) - CVec2( vSpotSize.x / 2.0f, vSpotSize.y / 2.0f );
	MovePoints( &( pTerrainSpotInstance->points ), vSpotPos );
	RotatePoints( &( pTerrainSpotInstance->points ), fDirection, CVec2( rvPosition.x, rvPosition.y ) );
}


void CSpotState::CreateSpotPointsList( list<CVec3> *pPointList, const NDb::STerrainSpotInstance &rTerrainSpotInstance )
{
	if ( pPointList )
	{
		if ( IEditorScene *pScene = EditorScene() )
		{
			pPointList->clear();
			for ( int nPointIndex = 0; nPointIndex < rTerrainSpotInstance.points.size(); ++nPointIndex )
			{
				CVec3 vPoint = CVec3( rTerrainSpotInstance.points[nPointIndex].x, rTerrainSpotInstance.points[nPointIndex].y, 0.0f );
				vPoint.z = GetTerrainHeight( vPoint.x, vPoint.y );
				pPointList->push_back( vPoint );
			}
		}
	}
}


void CSpotState::InsertObjectEnter()
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		IEditorScene *pScene = EditorScene();
		ICamera *pCamera = Camera();
		IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
		if ( ( pScene == 0 ) || ( pCamera == 0 ) || ( pManipulator == 0 ) )
		{
			return;
		}
		//
		if ( !CanAddSpot() )
		{
			return;
		}
		//
		CVec3 vPosition = pCamera->GetAnchor();
		Vis2AI( &vPosition );
		vPosition.z = GetTerrainHeight( vPosition.x, vPosition.y );
		//
		if ( pEditParameters->eDirectionType == CMapObjectMultiState::SEditParameters::DT_RANDOM )
		{
			pEditParameters->nFlags = MIMOSEP_DIRECTION;
			pEditParameters->fDirection = NWin32Random::Random( 360 ) * 1.0f;
			SetEditParameters( *pEditParameters, CHID_MAPINFO_MAPOBJECT_WINDOW );
		}
		const float fDirection = ( pEditParameters->fDirection * FP_PI ) / 180.0f;
		//
		ClearData();
		//
		SObjectSet objectSet;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) );
		if ( !objectSet.objectNameSet.empty() )
		{
			const NDb::STerrainSpotDesc *pTerrainSpotDesc = dynamic_cast<const NDb::STerrainSpotDesc*>( NDb::GetObject( objectSet.objectNameSet.begin()->first ) );
			if ( ( pTerrainSpotDesc->pMaterial != 0 ) && 
					( pTerrainSpotDesc->pMaterial->pTexture != 0 ) )
			{
				//NDb::SMapInfo* pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
				//
				terrainSpotInstance.pDescriptor = pTerrainSpotDesc;
				terrainSpotInstance.nSpotID = 0;
				/**
				list<CVec3> pointList;
				CreateSpotPointsList( &pointList, terrainSpotInstance );
				ModifySpotSquare( &terrainSpotInstance, vPosition, fDirection );
				/**/
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
		}
	}
}


void CSpotState::InsertObjectLeave()
{
	ClearData();
}


void CSpotState::InsertObjectDraw( class CPaintDC *pPaintDC )
{
	if ( CanEdit() )
	{
		CMapObjectMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		if ( IEditorScene *pScene = EditorScene() )
		{
			CVec3 vPosition = pStoreInputState->lastEventInfo.vTerrainPos;
			vPosition.z = GetTerrainHeight( vPosition.x, vPosition.y );
			float fDirection = ( ( pEditParameters->fDirection * FP_PI ) / 180.0f );
			if ( terrainSpotInstance.nSpotID != INVALID_NODE_ID )
			{
				terrainSpotInstance.points.resize( 4 );
				const float fScaleCoeff = ( terrainSpotInstance.pDescriptor->fScaleCoeff > 0.0f ) ? terrainSpotInstance.pDescriptor->fScaleCoeff : 1.0f;
				const CVec2 vSpotSize( terrainSpotInstance.pDescriptor->pMaterial->pTexture->nWidth * terrainSpotInstance.pDescriptor->fUsedTexSizeX * 2.0f / fScaleCoeff,
																terrainSpotInstance.pDescriptor->pMaterial->pTexture->nHeight * terrainSpotInstance.pDescriptor->fUsedTexSizeY * 2.0f / fScaleCoeff ); 
				//
				ModifySpotSquare( &terrainSpotInstance, vPosition, fDirection );
				list<CVec3> pointList;
				CreateSpotPointsList( &pointList, terrainSpotInstance );
				sceneDrawTool.DrawPolyline( pointList, NMapInfoEditor::PLACEMENT_COLOR, true, false );
			}
		}
	}
}

// basement storage  

/**
fPlacementDirection += FP_PI2;
if ( fPlacementDirection > FP_2PI )
{ 
	fPlacementDirection -= FP_2PI;
}
/**/

