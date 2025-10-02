#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "sceneb2/scene.h"
#include "mapeditorlib/commoneditormethods.h"
#include "ResourceDefines.h"

#include "MapInfoEditor.h"
#include "VSOMultiState.h"
#include "RoadState.h"

#include "EditorMethods.h"

#include "libdb/ResourceManager.h"

#include <zconf.h>

const string CRoadState::VSO_NAME = "Roads";
const string CRoadState::VSO_TYPE_NAME = "RoadDesc";


bool CRoadState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				if ( const NDb::SRoadDesc *pDescriptor = NDb::Get<NDb::SRoadDesc>( objectSet.objectNameSet.begin()->first ) )
				{
					/**
					if ( ( pDescriptor->leftBorder.pMaterial != 0 ) &&
							 ( pDescriptor->rightBorder.pMaterial != 0 ) )
					/**/
					{
						for( int nMaterialIndex = 0; nMaterialIndex < pDescriptor->center.materials.size(); ++nMaterialIndex )
						{
							if ( pDescriptor->center.materials[nMaterialIndex] == 0 )
							{
								return false;
							}
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}


NMapInfoEditor::CVSOInstanceList* CRoadState::GetVSOList()
{ 
	//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
	return &( GetMapInfoEditor()->VSOCollector.roadList );
}


void CRoadState::UpdateVisualVSO( NDb::SVSOInstance *pVSO, bool bBothEdges )
{
	if ( CanEdit() )
	{
		CVSOMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		if ( pVSO )
		{
			float fDefaultWidth = 0.0f;
			SObjectSet objectSet;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
			{
				if ( objectSet.szObjectTypeName == VSO_TYPE_NAME )
				{
					CPtr<IManipulator> pVSODescManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first );
					if ( pVSODescManipulator )
					{
						CManipulatorManager::GetValue( &fDefaultWidth, pVSODescManipulator, "DefaultWidth" );
					}
				}
			}
			//
			pEditParameters->nFlags = MIVSOSEP_POINT_NUMBER | MIVSOSEP_WIDTH | MIVSOSEP_OPACITY | MIVSOSEP_HEIGHT;
			GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			//
			if ( ( pEditParameters->fWidth < ( AI_TILE_SIZE / 2.0f ) ) ||
					 ( pEditParameters->fWidth > ( AI_TILE_SIZE * 2.0f * 16.0f * 2.0f ) ) )
			{
				pEditParameters->fWidth = fDefaultWidth / 2.0f;
			}
			//
			int nMinCount = INVALID_NODE_ID;
			int nMaxCount = INVALID_NODE_ID;
			GetControlPointBounds( &nMinCount, &nMaxCount );
			//UniquePolygon<vector<CVec3>, CVec3>( &( pVSOtoAddVSOInstance.controlPoints ), CVSOState::DEFAULT_POINT_RADIUS );
			if ( ( ( nMinCount == INVALID_NODE_ID ) || ( pVSO->controlPoints.size() >= nMinCount ) ) && 
					( pVSO->controlPoints.size() > 1 ) )
			{
				CVSOManager::Update( pVSO,
														 true,
														 false,
														 GetDefaultStep(),
														 pEditParameters->fWidth,
														 pEditParameters->fHeight,
														 pEditParameters->fOpacity,
														 true,
														 false,
														 true,
														 IsClose(),
														 IsComplete() );
			}
			else
			{
				pVSO->points.clear();
			}
		}
	}
}


bool CRoadState::CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType )
{
	switch( eSelectionType )
	{
		case CVSOManager::SVSOSelection::ST_CONTROL:
			return true;
		case CVSOManager::SVSOSelection::ST_CENTER:
			return false;
		case CVSOManager::SVSOSelection::ST_NORMALE:
			return true;
		case CVSOManager::SVSOSelection::ST_OPNORMALE:
			return true;
		default:
			return false;
	}
}


void CRoadState::InsertVSOInTerrain( const NDb::SVSOInstance &rVSO )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->AddRoad( &rVSO );
		}
	}
}


void CRoadState::RemoveVSOFromTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->RemoveRoad( nVSOID );
		}
	}
}


void CRoadState::UpdateVSOInTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			if ( const NDb::SVSOInstance* pVSO = GetVSO( nVSOID, 0 ) )
			{
				pTerraManager->RemoveRoad( nVSOID );
				pTerraManager->AddRoad( pVSO );
			}
		}
	}
}


// basement storage  


