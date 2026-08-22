#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"

#include "MapInfoEditor.h"
#include "LakeState.h"

#include "libdb/ResourceManager.h"

#include <cstdint>

#include <zconf.h>

const string CLakeState::VSO_NAME = "Lakes";
const string CLakeState::VSO_TYPE_NAME = "LakeDesc";


/**
bool CLakeState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				if ( const NDb::SLakeDesc *pDescriptor = NDb::Get<NDb::SLakeDesc>( objectSet.objectNameSet.begin()->first ) )
				{
					if ( pDescriptor->pWaterParams != 0 ) 
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}
/**/


NMapInfoEditor::CVSOInstanceList* CLakeState::GetVSOList()
{ 
	//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
	return &( GetMapInfoEditor()->VSOCollector.lakeList );
}


bool CLakeState::CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType )

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
			return false;
		default:
			return false;
	}
}


void CLakeState::PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList )
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	if ( CanEdit() )
	{
		if ( pPickVSOIDList )
		{
			pPickVSOIDList->clear();
			NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
			for ( int nVSOIndex = 0; nVSOIndex < pVSOList->size(); ++nVSOIndex )
			{
				list<CVec3> boundingPolygon;
				vector<NDb::SVSOPoint> *pVSOPointList = &( ( *pVSOList )[nVSOIndex].points );
				for ( int nPointIndex = 0; nPointIndex < pVSOPointList->size(); ++nPointIndex )
				{
					boundingPolygon.push_back( ( *pVSOPointList )[nPointIndex].vPos );
				}
				if ( ClassifyPolygon( boundingPolygon, rvPos ) != CP_OUTSIDE )
				{
					pPickVSOIDList->push_back( ( *pVSOList )[nVSOIndex].nVSOID );
				}
			}
		}
	}
	//
	DebugTrace( "CLakeState::PickVSO(): %g", NHPTimer::GetTimePassed( &time ) );
}


void CLakeState::InsertVSOInTerrain( const NDb::SVSOInstance &rVSO )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->UpdateWater();
			pTerraManager->UpdateRiversDepthes();
		}
	}
}


void CLakeState::RemoveVSOFromTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->UpdateWater();
			pTerraManager->UpdateRiversDepthes();
		}
	}
}


void CLakeState::UpdateVSOInTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->UpdateWater();
			pTerraManager->UpdateRiversDepthes();
		}
	}
}

// basement storage  


