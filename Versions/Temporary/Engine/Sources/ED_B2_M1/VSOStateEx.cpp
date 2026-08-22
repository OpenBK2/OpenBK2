#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "SceneB2/Scene.h"

#include "MapInfoEditor.h"
#include "VSOStateEx.h"

#include "libdb/ResourceManager.h"
#include "Stats_B2_M1/TerraAIObserver.h"

#include <cstdint>

#include <zconf.h>

bool CVSOStateEx::CanEdit()
{
	return ( ( GetMapInfoEditor()->pMapInfo != 0 ) && ( GetMapInfoEditor()->GetViewManipulator() != 0 ) && ( GetParentState()->GetEditParameters() != 0 ) );
}


void CVSOStateEx::PickVSO( const CVec3 &rvPos, CVSOIDList *pPickVSOIDList )
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
				CVSOManager::GetBoundingPolygon( &boundingPolygon, ( *pVSOList )[nVSOIndex].points, CVSOManager::PT_BOTH, 1.0f );
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


bool CVSOStateEx::CanInsertVSO()
{
	if ( CanEdit() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				if ( const NDb::SVSODesc *pDescriptor = NDb::Get<NDb::SVSODesc>( objectSet.objectNameSet.begin()->first ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}


NDb::SVSOInstance* CVSOStateEx::GetVSO( int nVSOID, int *pnVSOIndex )
{
	if ( CanEdit() )
	{
		NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
		for ( int nVSOIndex = 0; nVSOIndex < pVSOList->size(); ++nVSOIndex )
		{
			if ( ( *pVSOList )[nVSOIndex].nVSOID == nVSOID )
			{
				if ( pnVSOIndex != 0 )
				{
					( *pnVSOIndex ) = nVSOIndex;
				}
				return ( &( ( *pVSOList )[nVSOIndex] ) );
			}
		}
	}
	return 0;
}


int CVSOStateEx::GetFreeVSOID()
{
	int nFreeVSOID = INVALID_NODE_ID;
	if ( CanEdit() )
	{
		NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
		// find free VSOID
		for ( int nVSOIndex = 0; nVSOIndex < pVSOList->size(); ++nVSOIndex )
		{
			if ( ( *pVSOList )[nVSOIndex].nVSOID > nFreeVSOID )
			{
				nFreeVSOID = ( *pVSOList )[nVSOIndex].nVSOID;
			}
		}
		++nFreeVSOID;
	}
	return nFreeVSOID;
}


void CVSOStateEx::UpdateVSO( int nVSOID, EUpdateType eEpdateType, CVSOManager::SVSOSelection::ESelectionType eSelectionType, unsigned nFlags )
{
	if ( CanEdit() )
	{
		//DebugTrace( "CVSOVSOState::UpdateVSO( %d, %d, %d, %d ): %d", nVSOID, eEpdateType, eSelectionType, nFlags, bVSOChanged );
		NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
		NDb::SVSOInstance *pVSOInstance = 0;
		if ( eEpdateType == UT_START )
		{
			bVSOChanged = false;
			ClearOldVSOInstance();
			pVSOInstance = GetVSO( nVSOID, 0 );
			if ( pVSOInstance != 0 )
			{
				oldVSOInstance = ( *pVSOInstance );
			}
		}
		else if ( ( eEpdateType == UT_FINISH ) || ( eEpdateType == UT_CANCEL ) || ( eEpdateType == UT_CONTINUE_MOVE ) )
		{
			int nVSOIndex = INVALID_NODE_ID;
			pVSOInstance = GetVSO( nVSOID, &nVSOIndex );
			if ( pVSOInstance != 0 )
			{
				if ( ( eEpdateType == UT_FINISH ) && bVSOChanged )
				{
					if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
					{							
						if ( UpdateVSOInBase( pMapInfoController, nVSOIndex, *pVSOInstance ) )
						{
							pMapInfoController->AddChangeVSOOperation( GetVSOType(), oldVSOInstance, *pVSOInstance );
							pMapInfoController->Redo( false, true, GetMapInfoEditor() );
							Singleton<IControllerContainer>()->Add( pMapInfoController );
						}
					}
					bVSOChanged = false;
					ClearOldVSOInstance();
					GetMapInfoEditor()->SetModified( true );
				}
				else if ( eEpdateType == UT_CANCEL )
				{
					bVSOChanged = false;
					ClearOldVSOInstance();
					if ( !IsRealtimeUpdate() )
					{
						return;
					}
				}
				else if ( eEpdateType == UT_CONTINUE_MOVE )
				{
					bVSOChanged = true;
					if ( !IsRealtimeUpdate() )
					{
						return;
					}
				}
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				//
				UpdateVSOInTerrain( nVSOID );
				//
				DebugTrace( "CVSOStateEx::UpdateVSO(), UpdateVSOInTerrain(): %g", NHPTimer::GetTimePassed( &time ) );
			}
		}
		else
		{
			bVSOChanged = true;
		}
	}
	EditorScene()->GetTerraManager()->GetAIObserver()->FinalizeUpdates();
}


int CVSOStateEx::InsertVSO( const vector<CVec3> &rControlPointList )
{
	int nFreeVSOID = INVALID_NODE_ID;
	if ( CanEdit() && CanInsertVSO() )
	{
		SObjectSet objectSet;
		if ( Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<uint32_t>( &objectSet ) ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if ( objectSet.szObjectTypeName == GetVSOTypeName() )
			{
				NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
				nFreeVSOID = GetFreeVSOID();
				//
				NMapInfoEditor::CVSOInstanceList::iterator posNewVSO = pVSOList->insert( pVSOList->end(), NDb::SVSOInstance() );
				//
				NHPTimer::STime time = 0;
				NHPTimer::GetTime( &time );
				//
				posNewVSO->pDescriptor = NDb::Get<NDb::SVSODesc>( objectSet.objectNameSet.begin()->first );
				//
				DebugTrace( "CVSOStateEx::InsertVSO(), NDb::GetResourcel(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				posNewVSO->nVSOID = nFreeVSOID;
				posNewVSO->controlPoints = rControlPointList;
				//
				UpdateVisualVSO( &( *posNewVSO ), true );
				//
				DebugTrace( "CVSOStateEx::InsertVSO(), CVSOManager::Update(): %g", NHPTimer::GetTimePassed( &time ) );
				//
				bool bVSOInserted = false;
				if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
				{							
					if ( InsertVSOToBase( pMapInfoController, NODE_ADD_INDEX, *posNewVSO ) )
					{
						pMapInfoController->AddInsertVSOOperation( GetVSOType(), ( *posNewVSO ) );
						pMapInfoController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pMapInfoController );
						//
						NHPTimer::GetTime( &time );
						//
						InsertVSOInTerrain( *posNewVSO );
						//
						DebugTrace( "CVSOStateEx::InsertVSO(), AddVSO(): %g", NHPTimer::GetTimePassed( &time ) );
						//
						GetMapInfoEditor()->SetModified( true );
						bVSOInserted = true;
					}
				}
				if ( !bVSOInserted )
				{
					pVSOList->erase( posNewVSO );
					nFreeVSOID = INVALID_NODE_ID;
				}
			}
		}
	}
	// EditorScene()->GetTerrain()->GetAIObserver()->FinalizeUpdates(); - no need - this is called from UpdateVisualVSO()
	return nFreeVSOID;
}


void CVSOStateEx::RemoveVSO( int nVSOID )
{
	if ( CanEdit() )
	{
		NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
		int nVSOIndex = INVALID_NODE_ID;
		NDb::SVSOInstance *pVSOInstance = GetVSO( nVSOID, &nVSOIndex );
		if ( pVSOInstance != 0 )
		{
			if ( CPtr<CMapInfoController> pMapInfoController = GetMapInfoEditor()->CreateController() )
			{							
				if ( RemoveVSOFromBase( pMapInfoController, nVSOIndex ) )
				{
					pMapInfoController->AddRemoveVSOOperation( GetVSOType(), *pVSOInstance );
					pMapInfoController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pMapInfoController );
					//
					pVSOList->erase( pVSOList->begin() + nVSOIndex );
					//
					NHPTimer::STime time = 0;
					NHPTimer::GetTime( &time );
					//
					RemoveVSOFromTerrain( nVSOID );
					//
					DebugTrace( "CVSOStateEx::RemoveVSO(), RemoveVSO(): %g", NHPTimer::GetTimePassed( &time ) );
					//
					GetMapInfoEditor()->SetModified( true );
				}
			}
		}
	}
	EditorScene()->GetTerraManager()->GetAIObserver()->FinalizeUpdates();
}


bool CVSOStateEx::InsertVSOToBase( CObjectBaseController *pObjectController, int nVSOIndex, const NDb::SVSOInstance &rVSO )
{
	if ( CanEdit() )
	{
		if ( pObjectController != 0 )
		{
			NHPTimer::STime time = 0;
			NHPTimer::GetTime( &time );
			//
			string szVSODescName = rVSO.pDescriptor->GetDBID().ToString();
			//
			NMapInfoEditor::CVSOInstanceList *pVSOList = GetVSOList();
			const int nNewVSOIndex = ( nVSOIndex == NODE_ADD_INDEX ) ? ( pVSOList->size() - 1 ) : nVSOIndex;
			const string szNewVSOLabel =	StrFmt( "%s.[%d].", GetVSOName().c_str(), nNewVSOIndex );
			if ( !szVSODescName.empty() )
			{
				bool bResult = true;
				IManipulator *pManipulator = GetMapInfoEditor()->GetViewManipulator();
				//Add
				bResult = bResult && pObjectController->AddInsertOperation( GetVSOName(), nVSOIndex, pManipulator );
				//Set Descriptor name
				bResult = bResult && pObjectController->AddChangeOperation( szNewVSOLabel + "Descriptor", string( StrFmt( "%s:%s", GetVSOTypeName().c_str(), szVSODescName.c_str() ) ), pManipulator );
				//Set ID
				bResult = bResult && pObjectController->AddChangeOperation( szNewVSOLabel + "VSOID", rVSO.nVSOID, pManipulator );
				// Add ControlPoints
				if ( bResult )
				{
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
				}
				// Add ponts
				if ( bResult )
				{
					for ( int nPointIndex = 0; nPointIndex < rVSO.points.size(); ++nPointIndex )
					{
						const string szPointLabel =  szNewVSOLabel + StrFmt( "points.[%d].", nPointIndex );
						//Add
						bResult = bResult && pObjectController->AddInsertOperation( szNewVSOLabel + "points", NODE_ADD_INDEX, pManipulator );
						//Set Pos
						bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szPointLabel +"Pos", rVSO.points[nPointIndex].vPos, pManipulator );
						//Set Norm
						bResult = bResult && pObjectController->AddChangeVec3Operation<CVec3, float>( szPointLabel +"Norm", rVSO.points[nPointIndex].vNorm, pManipulator );
						//Set Radius
						bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "Radius", CVSOManager::DEFAULT_LEFT_BANK_HEIGHT, pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( szPointLabel + "Reserved", CVSOManager::DEFAULT_RIGHT_BANK_HEIGHT, pManipulator );
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
				}
				DebugTrace( "CVSOStateEx::InsertVSOToBase(): %g", NHPTimer::GetTimePassed( &time ) );
				return bResult;
			}
		}
	}
	return false;
}


bool CVSOStateEx::RemoveVSOFromBase( CObjectBaseController *pObjectController, int nVSOIndex )
{
	if ( CanEdit() )
	{
		if ( pObjectController != 0 )
		{
			NHPTimer::STime time = 0;
			NHPTimer::GetTime( &time );
			//Delete
			bool bResult = true;
			bResult = bResult && pObjectController->AddRemoveOperation( GetVSOName(), nVSOIndex, GetMapInfoEditor()->GetViewManipulator() );
			DebugTrace( "CVSOStateEx::RemoveVSOFromBase(): %g", NHPTimer::GetTimePassed( &time ) );
			return bResult;
		}
	}
	return false;
}


bool CVSOStateEx::UpdateVSOInBase( CObjectBaseController *pObjectController, int nVSOIndex, const NDb::SVSOInstance &rVSO )
{
	if ( CanEdit() )
	{
		if ( pObjectController != 0 )
		{
			if ( RemoveVSOFromBase( pObjectController, nVSOIndex ) )
			{
				return InsertVSOToBase( pObjectController, nVSOIndex, rVSO );
			}
		}
	}
	return false;
}


// basement storage  


