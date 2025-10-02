#include "stdafx.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"

#include "MapInfoEditor.h"
#include "LakeState.h"

#include "../libdb/ResourceManager.h"

#include <zconf.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


const string CLakeState::VSO_NAME = "Lakes";
const string CLakeState::VSO_TYPE_NAME = "LakeDesc";


/**
bool CLakeState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
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


