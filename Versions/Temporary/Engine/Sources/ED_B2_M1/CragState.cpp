#include "stdafx.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../sceneb2/scene.h"
#include "ResourceDefines.h"

#include "MapInfoEditor.h"
#include "CragState.h"

#include <zconf.h>

const string CCragState::VSO_NAME = "Crags";
const string CCragState::VSO_TYPE_NAME = "CragDesc";


/**
bool CCragState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				if ( const NDb::SCragDesc *pDescriptor = NDb::Get<NDb::SCragDesc>( objectSet.objectNameSet.begin()->first ) )
				{
					if ( ( pDescriptor->pRidgeMaterial != 0 ) &&
							 ( pDescriptor->pFootMaterial != 0 ) )
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

NMapInfoEditor::CVSOInstanceList* CCragState::GetVSOList()
{ 
	//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
	return &( GetMapInfoEditor()->VSOCollector.cragList );
}


void CCragState::PrepareInsertVSO()
{
	pSelectedCragDesc = 0;
	const UINT nObjectTypeID = NDb::SCragDesc::typeID;
	SObjectSet objectSet;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
	{
		if ( objectSet.szObjectTypeName == VSO_TYPE_NAME )
		{
			pSelectedCragDesc = NDb::Get<NDb::SCragDesc>( objectSet.objectNameSet.begin()->first );
		}
	}
}


bool CCragState::CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType )

{
	const NDb::SCragDesc* pCragDesc = 0;
	if ( NDb::SVSOInstance *pSelectedVSO = GetVSO( GetSelectedVSOID(), 0 ) )
	{
		pCragDesc = checked_cast<const NDb::SCragDesc*>( &( *pSelectedVSO->pDescriptor ) );
	}
	else
	{
		pCragDesc = pSelectedCragDesc;
	}
	switch( eSelectionType )
	{
		case CVSOManager::SVSOSelection::ST_CONTROL:
			return true;
		case CVSOManager::SVSOSelection::ST_CENTER:
			return true;
		case CVSOManager::SVSOSelection::ST_NORMALE:
			if ( pCragDesc != 0 )
			{
				return !pCragDesc->bLeftSided;
			}
			return false;
		case CVSOManager::SVSOSelection::ST_OPNORMALE:
			if ( pCragDesc != 0 )
			{
				return pCragDesc->bLeftSided;
			}
			return false;
		default:
			return false;
	}
}


void CCragState::PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList )
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
				const NDb::SCragDesc *pCragDesc = checked_cast<const NDb::SCragDesc*>( &( *( ( *pVSOList )[nVSOIndex].pDescriptor ) ) );
				CVSOManager::GetBoundingPolygon( &boundingPolygon, ( *pVSOList )[nVSOIndex].points, pCragDesc->bLeftSided ? CVSOManager::PT_OPNORMALE : CVSOManager::PT_NORMALE, 1.0f );
				if ( ClassifyPolygon( boundingPolygon, rvPos ) != CP_OUTSIDE )
				{
					pPickVSOIDList->push_back( ( *pVSOList )[nVSOIndex].nVSOID );
				}
			}
		}
	}
	//
	DebugTrace( "CVSOStateEx::PickVSO(): %g", NHPTimer::GetTimePassed( &time ) );
}


void CCragState::InsertVSOInTerrain( const NDb::SVSOInstance &rVSO )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->AddCrag( &rVSO );
			GetMapInfoEditor()->heightContainer.InsertVSO( rVSO );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
	}
}


void CCragState::RemoveVSOFromTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->RemoveCrag( nVSOID );
			GetMapInfoEditor()->heightContainer.ErasePolygon( nVSOID );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
		}
	}
}


void CCragState::UpdateVSOInTerrain( int nVSOID )
{
	if ( IEditorScene *pScene = EditorScene() )
	{
		if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
		{
			pTerraManager->UpdateCrag( nVSOID );
			if ( NDb::SVSOInstance *pVSO = GetVSO( nVSOID, 0 ) )
			{
				GetMapInfoEditor()->heightContainer.InsertVSO( *pVSO );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAPINFO_EDITOR, ID_TOOLS_UPDATE_VSO, 0 );
			}
		}
	}
}


// basement storage  



