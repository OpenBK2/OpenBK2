#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "ResourceDefines.h"

#include "MapInfoEditor.h"
#include "VSOMultiState.h"
#include "RiverState.h"

#include "EditorMethods.h"

#include "libdb/ResourceManager.h"

#include <cstdint>

#include <zconf.h>

const std::string CRiverState::VSO_NAME = "Rivers";
const std::string CRiverState::VSO_TYPE_NAME = "RiverDesc";


/**
bool CRiverState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				if ( const NDb::SRiverDesc *pDescriptor = NDb::Get<NDb::SRiverDesc>( objectSet.objectNameSet.begin()->first ) )
				{
					if ( ( pDescriptor->pBottomMaterial != 0 ) &&
							 ( pDescriptor->pPrecipiceMaterial != 0 ) &&
							 ( pDescriptor->pWaterMaterial != 0 ) )
					{
						for ( int nLayerIndex = 0; nLayerIndex < pDescriptor->waterLayers.size(); ++ nLayerIndex )
						{
							for( int nMaterialIndex = 0; nMaterialIndex < pDescriptor->waterLayers[nLayerIndex].materials.size(); ++nMaterialIndex )
							{
								if ( pDescriptor->waterLayers[nLayerIndex].materials[nMaterialIndex] == 0 )
								{
									return false;
								}
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
/**/

NMapInfoEditor::CVSOInstanceList* CRiverState::GetVSOList()
{ 
	//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
	return &( GetMapInfoEditor()->VSOCollector.riverList );
}


void CRiverState::UpdateVisualVSO( NDb::SVSOInstance *pVSO, bool bBothEdges )
{
	if ( CanEdit() )
	{
		CVSOMultiState::SEditParameters *pEditParameters = GetParentState()->GetEditParameters();
		if ( pVSO )
		{
			float fDefaultWidth = 0.0f;
			SObjectSet objectSet;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
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
			::GetEditParameters( pEditParameters, CHID_MAPINFO_VSO_WINDOW );
			//
			if ( ( fDefaultWidth >= AI_TILE_SIZE * 2.0f ) && ( fDefaultWidth <= AI_TILE_SIZE * 2.0f * 16.0f * 2.0f ) )
			{
				pEditParameters->fWidth = fDefaultWidth / 2.0f;
			}
			//
			int nMinCount = INVALID_NODE_ID;
			int nMaxCount = INVALID_NODE_ID;
			GetControlPointBounds( &nMinCount, &nMaxCount );
			//UniquePolygon<std::vector<CVec3>, CVec3>( &( pVSOtoAddVSOInstance.controlPoints ), CVSOState::DEFAULT_POINT_RADIUS );
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


bool CRiverState::CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType )
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


void CRiverState::InsertVSOInTerrain( const NDb::SVSOInstance &rVSO )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->AddRiver( &rVSO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
	}
}


void CRiverState::RemoveVSOFromTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->RemoveRiver( nVSOID );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
	}
}


void CRiverState::UpdateVSOInTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->UpdateRiver( nVSOID );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
	}
}


// basement storage  


