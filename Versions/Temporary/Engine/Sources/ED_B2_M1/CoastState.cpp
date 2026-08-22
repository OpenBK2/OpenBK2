#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "SceneB2/Scene.h"

#include "MapInfoEditor.h"
#include "CoastState.h"


#include "libdb/ResourceManager.h"
#include "Stats_B2_M1/TerraAIObserver.h"

#include <cstdint>

#include <zconf.h>

void CCoastState::GetWaterPos( const NDb::SMapInfo *pMapInfo, const vector<NDb::SVSOPoint> &rPoints, CVec3 *pWaterPos )
{
	if ( pMapInfo && pWaterPos )
	{
		CTRect<float> mapRect( 0.0f, 0.0f, pMapInfo->nNumPatchesX * AI_TILE_SIZE * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILE_SIZE * AI_TILES_IN_PATCH );
		for ( int nPointIndex = 0; nPointIndex < rPoints.size(); ++nPointIndex )
		{
			CVec3 vNormalPoint = rPoints[nPointIndex].vPos + rPoints[nPointIndex].vNorm * rPoints[nPointIndex].fWidth;
			if ( mapRect.IsInside( vNormalPoint.x, vNormalPoint.y ) )
			{
				pWaterPos->x = vNormalPoint.x;
				pWaterPos->y = vNormalPoint.y;
				pWaterPos->z = 0.0f;
				break;
			}
		}
	}
}


bool CCoastState::CanEdit()
{
	return ( ( GetMapInfoEditor()->pMapInfo != 0 ) && ( GetMapInfoEditor()->GetViewManipulator() != 0 ) && ( GetParentState()->GetEditParameters() != 0 ) );
}


bool CCoastState::CanEditPoints( CVSOManager::SVSOSelection::ESelectionType eSelectionType )

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


void CCoastState::PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList )
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	if ( CanEdit() )
	{
		if ( pPickVSOIDList )
		{
			pPickVSOIDList->clear();
			list<CVec3> boundingPolygon;
			CVSOManager::GetBoundingPolygon( &boundingPolygon, GetMapInfoEditor()->VSOCollector.coast.points, CVSOManager::PT_NORMALE, 1.0f );
			if ( ClassifyPolygon( boundingPolygon, rvPos ) != CP_OUTSIDE )
			{
				pPickVSOIDList->push_back( GetMapInfoEditor()->VSOCollector.coast.nVSOID );
			}
		}
	}
	//
	DebugTrace( "CCoastState::PickVSO(): %g", NHPTimer::GetTimePassed( &time ) );
}


bool CCoastState::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			return ( objectSet.szObjectTypeName == "CoastDesc" );
		}
	}
	return false;
}


NDb::SVSOInstance* CCoastState::GetVSO( int nVSOID, int *pnVSOIndex )
{
	if ( CanEdit() )
	{
		if ( GetMapInfoEditor()->VSOCollector.coast.nVSOID == nVSOID )
		{
			return const_cast<NDb::SVSOInstance*>( &( GetMapInfoEditor()->VSOCollector.coast ) );
		}
	}
	return 0;
}


void CCoastState::UpdateVSO( int nVSOID, EUpdateType eEpdateType, CVSOManager::SVSOSelection::ESelectionType eSelectionType, unsigned nFlags )
{
	if ( CanEdit() )
	{
		//DebugTrace( "CCoastState::UpdateVSO( %d, %d, %d, %d ): %d", nVSOID, eEpdateType, eSelectionType, nFlags, bVSOChanged );
		if ( eEpdateType == UT_START )
		{
			//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
			if ( GetMapInfoEditor()->VSOCollector.coast.nVSOID == nVSOID )
			{
				oldVSOInstance = GetMapInfoEditor()->VSOCollector.coast;
			}
		}
		else if ( eEpdateType == UT_FINISH )
		{
			if ( bVSOChanged == true )
			{
				//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
				if ( GetMapInfoEditor()->VSOCollector.coast.nVSOID == nVSOID )
				{
					if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
					{							
						if ( UpdateVSOInBase( pMapInfoController, GetMapInfoEditor()->VSOCollector.coast ) )
						{
							pMapInfoController->AddChangeVSOOperation( GetVSOType(), oldVSOInstance, GetMapInfoEditor()->VSOCollector.coast );
							pMapInfoController->Redo( false, true, GetMapInfoEditor() );
							Singleton<IControllerContainer>()->Add( pMapInfoController );
							//
							if ( IEditorScene *pScene = EditorScene() )
							{
								if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
								{
									NHPTimer::STime time = 0;
									NHPTimer::GetTime( &time );
									//
									GetMapInfoEditor()->VSOCollector.bHasCoast = true;
									GetWaterPos( GetMapInfoEditor()->pMapInfo, GetMapInfoEditor()->VSOCollector.coast.points, &( GetMapInfoEditor()->VSOCollector.vCoastMidPoint ) );
									pTerraManager->UpdateWater();
									pTerraManager->UpdateRiversDepthes();
									//
									DebugTrace( "CCoastState::UpdateVSO(), SetSeaPlacement(): %g", NHPTimer::GetTimePassed( &time ) );
									//
									GetMapInfoEditor()->SetModified( true );
								}
							}
						}
					}
				}
			}
			bVSOChanged = false;
		}
		else
		{
			bVSOChanged = true;
		}
	}
}


int CCoastState::InsertVSO( const vector<CVec3> &rControlPointList )
{
	int nNewVSOID = 0;
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if ( objectSet.szObjectTypeName == "CoastDesc" )
			{
				const unsigned nObjectTypeID = NDb::SCoastDesc::typeID;
				//
				//NDb::SMapInfo *pMapInfo = const_cast<NDb::SMapInfo*>( GetMapInfoEditor()->pMapInfo );
				//
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				//
				GetMapInfoEditor()->VSOCollector.coast.pDescriptor = NDb::Get<NDb::SVSODesc>( objectSet.objectNameSet.begin()->first );
				const NDb::SCoastDesc* pCoastDesc = checked_cast<const NDb::SCoastDesc*>( &( *( GetMapInfoEditor()->VSOCollector.coast.pDescriptor  ) ) );
				//
				DebugTrace( "CCoastState::InsertVSO(), NDb::GetResourcel(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				GetMapInfoEditor()->VSOCollector.coast.nVSOID = nNewVSOID;
				GetMapInfoEditor()->VSOCollector.coast.controlPoints = rControlPointList;
				//
				UpdateVisualVSO( &GetMapInfoEditor()->VSOCollector.coast, true );
				//
				DebugTrace( "CCoastState::InsertVSO(), CVSOManager::Update(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
				{							
					if ( InsertVSOToBase( pMapInfoController, GetMapInfoEditor()->VSOCollector.coast ) )
					{
						pMapInfoController->AddInsertVSOOperation( GetVSOType(), GetMapInfoEditor()->VSOCollector.coast );
						pMapInfoController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pMapInfoController );
						//
						if ( IEditorScene *pScene = EditorScene() )
						{
							if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
							{
								NHPTimer::STime time = 0;
								NHPTimer::GetTime( &time );
								//
								GetMapInfoEditor()->VSOCollector.bHasCoast = true;
								GetWaterPos( GetMapInfoEditor()->pMapInfo, GetMapInfoEditor()->VSOCollector.coast.points, &( GetMapInfoEditor()->VSOCollector.vCoastMidPoint ) );
								pTerraManager->UpdateWater();
								pTerraManager->UpdateRiversDepthes();
								//
								DebugTrace( "CCoastState::InsertVSO(), AddCoast(): %g", NHPTimer::GetTimePassed( &time ) );
								//
								GetMapInfoEditor()->SetModified( true );
								//
								return nNewVSOID;
							}
						}
					}
				}
				GetMapInfoEditor()->VSOCollector.coast.controlPoints.clear();
				GetMapInfoEditor()->VSOCollector.coast.points.clear();
				GetMapInfoEditor()->VSOCollector.coast.pDescriptor = 0;
				GetMapInfoEditor()->VSOCollector.coast.nVSOID = INVALID_NODE_ID;
				return INVALID_NODE_ID;
			}
		}
	}
	return INVALID_NODE_ID;
}


void CCoastState::RemoveVSO( int nVSOID )
{
	if ( CanEdit() )
	{
		if ( GetMapInfoEditor()->VSOCollector.coast.nVSOID == nVSOID )
		{
			if ( IEditorScene *pScene = EditorScene() )
			{
				if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
				{
					if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
					{							
						if ( RemoveVSOFromBase( pMapInfoController ) )
						{
							pMapInfoController->AddRemoveVSOOperation( GetVSOType(), GetMapInfoEditor()->VSOCollector.coast );
							pMapInfoController->Redo( false, true, GetMapInfoEditor() );
							Singleton<IControllerContainer>()->Add( pMapInfoController );
							//
							GetMapInfoEditor()->VSOCollector.coast.controlPoints.clear();
							GetMapInfoEditor()->VSOCollector.coast.points.clear();
							GetMapInfoEditor()->VSOCollector.coast.pDescriptor = 0;
							GetMapInfoEditor()->VSOCollector.coast.nVSOID = INVALID_NODE_ID;
							GetMapInfoEditor()->VSOCollector.bHasCoast = false;
							GetMapInfoEditor()->VSOCollector.vCoastMidPoint = VNULL3;
							//
							pTerraManager->UpdateWater();
							pTerraManager->UpdateRiversDepthes();
							//
							NHPTimer::STime time = 0;
							NHPTimer::GetTime( &time );
							//
							DebugTrace( "CCoastState::RemoveVSO(), RemoveCoast(): %g", NHPTimer::GetTimePassed( &time ) );
							//
							GetMapInfoEditor()->SetModified( true );
						}
					}
				}
			}
		}
	}
	EditorScene()->GetTerraManager()->GetAIObserver()->FinalizeUpdates();
}


bool CCoastState::InsertVSOToBase( class CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO )
{
	if ( CanEdit() )
	{
		NHPTimer::STime time = 0;
		NHPTimer::GetTime( &time );
		//
		string szVSODescName = rVSO.pDescriptor->GetDBID().ToString();
		//
		const string szNewVSOLabel =	StrFmt( "Coast." );
		if ( !szVSODescName.empty() )
		{
			bool bResult = true;
			if ( pObjectController )
			{
				IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
				//Set Descriptor name
				bResult = bResult && pObjectController->AddChangeOperation( szNewVSOLabel + "Descriptor", string( "CoastDesc:" ) + szVSODescName, pManipulator );
				//Set ID
				bResult = bResult && pObjectController->AddChangeOperation( szNewVSOLabel + "VSOID", rVSO.nVSOID, pManipulator );
				// Add ControlPoints
				for ( int nPointIndex = 0; nPointIndex < rVSO.controlPoints.size(); ++nPointIndex )
				{
					const string szControlPointLabel = szNewVSOLabel + StrFmt( "ControlPoints.[%d]", nPointIndex );
					//Add
					bResult = bResult && pObjectController->AddInsertOperation( szNewVSOLabel + "ControlPoints", NODE_ADD_INDEX, pManipulator );
					//Set
					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szControlPointLabel, rVSO.controlPoints[nPointIndex], pManipulator );
					if ( !bResult )
					{
						break;
					}
				}
				// Add ponts
				for ( int nPointIndex = 0; nPointIndex < rVSO.points.size(); ++nPointIndex )
				{
					const string szPointLabel =  szNewVSOLabel + StrFmt( "points.[%d].", nPointIndex );
					//Add
					bResult = bResult && pObjectController->AddInsertOperation( szNewVSOLabel + "points", NODE_ADD_INDEX, pManipulator );
					//Set Pos
					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szPointLabel + "Pos", rVSO.points[nPointIndex].vPos, pManipulator );
					//Set Norm
					bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szPointLabel + "Norm", rVSO.points[nPointIndex].vNorm, pManipulator );
					//Set Radius
					bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "Radius", 0.0f, pManipulator );
					//Set Width
					bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "Width", rVSO.points[nPointIndex].fWidth, pManipulator );
					//Set Opacity
					bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "Opacity", rVSO.points[nPointIndex].fOpacity, pManipulator );
					//Set Control Point Attribute
					bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "KeyPoint", rVSO.points[nPointIndex].bKeyPoint, pManipulator );
					if ( !bResult )
					{
						break;
					}
				}
				// позиция моря
				CVec3 vWaterPoint = VNULL3;
				GetWaterPos( GetMapInfoEditor()->pMapInfo, rVSO.points, &vWaterPoint );
				bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( "CoastMidPoint", vWaterPoint, pManipulator );
				// устанавливаем наличие побережья
				bResult = bResult && pObjectController->AddChangeValueOperation<bool>( "HasCoast", true, pManipulator );
				//
				DebugTrace( "CCoastState::InsertVSOToBase(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				return bResult;
			}
		}
	}
	return false;
}


bool CCoastState::RemoveVSOFromBase( class CObjectBaseController *pObjectController )
{
	if ( CanEdit() )
	{
		NHPTimer::STime time = 0;
		NHPTimer::GetTime( &time );
		//
		bool bResult = true;
		if ( pObjectController )
		{
			IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
			//
			const string szVSOLabel =	StrFmt( "Coast." );
			//Delete
			bResult = bResult && pObjectController->AddChangeValueOperation<CVariant>( szVSOLabel + "Descriptor", CVariant(), pManipulator );
			bResult = bResult && pObjectController->AddChangeValueOperation<int>( szVSOLabel + "VSOID", INVALID_NODE_ID, pManipulator );
			bResult = bResult && pObjectController->AddRemoveOperation( szVSOLabel + "ControlPoints", NODE_REMOVEALL_INDEX, pManipulator );
			bResult = bResult && pObjectController->AddRemoveOperation( szVSOLabel + "points", NODE_REMOVEALL_INDEX, pManipulator );
			bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( "CoastMidPoint", VNULL3, pManipulator );
			bResult = bResult && pObjectController->AddChangeValueOperation<bool>( "HasCoast", false, pManipulator );
			//
			DebugTrace( "CCoastState::RemoveVSOFromBase(): %g", NHPTimer::GetTimePassed( &time ) );
			//
			return bResult;
		}
	}
	return false;
}


bool CCoastState::UpdateVSOInBase( CObjectBaseController *pObjectController, const NDb::SVSOInstance &rVSO )
{
	if ( CanEdit() )
	{
		if ( pObjectController != 0 )
		{
			if ( RemoveVSOFromBase( pObjectController ) )
			{
				return InsertVSOToBase( pObjectController, rVSO );
			}
		}
	}
	return false;
}

// basement storage  




