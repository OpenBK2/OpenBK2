#include "stdafx.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/iconsset.h"
#include "../image/targa.h"
#include "../sceneb2/scene.h"
#include "../libdb/resourcemanager.h"
#include "simpleobjectinfodata.h"
#include "../mapeditorlib/commoneditormethods.h"
#include "ResourceDefines.h"
#include "../MapEditorLib/CommonExporterMethods.h"
#include "../MapEditorLib/Interface_Progress.h"
#include "../libdb/EditorDB.h"
#include "MapInfoEditor.h"
#include "FieldState.h"
#include "EditorMethods.h"
#include "../System/VFSOperations.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char CFieldState::FIELD_TYPE_NAME[] = "Field";

void CFieldState::UpdateEditParameters( UINT nFlags )
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		pEditParameters->nFlags = nFlags;
		if ( pEditParameters->nFlags & MITFEP_FIELD_COUNT )
		{
			string szMapTerraSetTypeName;
			string szMapTerraSetName;
			CManipulatorManager::GetParamsFromReference( "TerraSet", pMapInfoEditor->GetViewManipulator(), &szMapTerraSetTypeName, &szMapTerraSetName, 0 );
			//
			pEditParameters->fieldList.clear();
			if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( FIELD_TYPE_NAME ) )
			{
				for ( CPtr<IManipulatorIterator> it = pFolderManipulator->Iterate( false, ECT_NO_CACHE ); !it->IsEnd(); it->Next() )
				{
					string szName;
					if ( it->GetName( &szName ) && ( !szName.empty() ) && ( szName[szName.size() - 1] != PATH_SEPARATOR_CHAR ) )
					{
						if ( CPtr<IManipulator> pFieldManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( FIELD_TYPE_NAME, szName ) )
						{
							string szTerraSetTypeName;
							string szTerraSetName;
							CManipulatorManager::GetParamsFromReference( "TerraSet", pFieldManipulator, &szTerraSetTypeName, &szTerraSetName, 0 );
							//
							if ( ( szMapTerraSetTypeName == szTerraSetTypeName ) && ( szMapTerraSetName == szTerraSetName ) )
							{
								pEditParameters->fieldList.push_back( szName );
							}
						}
					}
				}
			}
		}
	}
}


void CFieldState::Enter()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
	}
	//
	CPolygonState::Enter();
}


void CFieldState::Leave()
{
	CPolygonState::Leave();
	//
	//UpdateEditParameters( MITFEP_ALL );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		::GetEditParameters( pEditParameters, CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
	}
}


bool CFieldState::CanEdit()
{
	return ( ( pMapInfoEditor->pMapInfo != 0 ) && ( pMapInfoEditor->GetViewManipulator() != 0 ) && GetEditParameters() != 0 );
}


void CFieldState::GetBounds( int *pnMinCount, int *pnMaxCount )
{
	NI_ASSERT( pnMinCount != 0, "CFieldState::GetBounds(): Invalid parameter: pnMinCount == 0" );
	NI_ASSERT( pnMaxCount != 0, "CFieldState::GetBounds(): Invalid parameter: pnMaxCount == 0" );
	( *pnMinCount ) = 3;
	( *pnMaxCount ) = INVALID_NODE_ID;
}


CPolygonState::EMoveType CFieldState::GetMoveType()
{
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		return pEditParameters->eMoveType;
	}
	else
	{
		return CPolygonState::MT_SINGLE;
	}
}

bool CFieldState::PrepareControlPoints( CControlPointList *pControlPointList )
{
	// подготовить полигон согласно установленным параметрам
	UniquePolygon<CControlPointList, CVec3>( pControlPointList, ( AI_TILE_SIZE * 2.0f ) );
	if ( SEditParameters *pEditParameters = GetEditParameters() )
	{
		if ( pEditParameters->bRandomize )
		{
			RandomizeEdges<CControlPointList, CVec3>( ( *pControlPointList ),
																								10,
																								pEditParameters->fWidth,
																								CTPoint<float>( 0.0f, pEditParameters->fDisturbance ),
																								pControlPointList,
																								pEditParameters->fMinLength * ( AI_TILE_SIZE * 2.0f ),
																								( 16.0f * 32.0f ) * ( AI_TILE_SIZE * 2.0f ),
																								true );
			UniquePolygon<CControlPointList, CVec3>( pControlPointList, ( AI_TILE_SIZE * 2.0f ) );
		}
	}
	return true;
}


void CFieldState::UpdatePolygon( int nPolygonID, EUpdateType eEpdateType )
{
	if ( eEpdateType == UT_FINISH )
	{
		// считываем бинарные данные
		if ( SEditParameters *pEditParameters = GetEditParameters() )
		{
			if ( ( pEditParameters->nFieldIndex >= 0 ) && ( pEditParameters->nFieldIndex < pEditParameters->fieldList.size() ) )
			{
				const string szFieldName = pEditParameters->fieldList[pEditParameters->nFieldIndex];
				if ( const NDb::SField *pField = NDb::Get<NDb::SField>( CDBID( szFieldName ) ) )
				{
					if ( CPtr<CMapInfoController> pMapInfoController = pMapInfoEditor->CreateController() )
					{
						NProgress::Create( true );
						CString strPM;
						strPM.LoadString( IDS_PM_PLACE_FIELD );
						NProgress::SetMessage( StrFmt( strPM, szFieldName.c_str() ) );
						NProgress::SetRange( 0, 6 );
						NProgress::IteratePosition();	// 1
						//
						CFieldDistanceMap visDistances;
						CFieldDistanceMap aiDistances;
						CFieldPolygon polygon;
						for ( CControlPointList::const_iterator itControlPoint = controlPointList.begin(); itControlPoint != controlPointList.end(); ++itControlPoint )
						{
							polygon.push_back( CVec2( itControlPoint->x, itControlPoint->y ) );
						}
						NProgress::IteratePosition();	// 2
						// заполняем землю
						if ( pEditParameters->bFillTerrain )
						{
							CArray2D<BYTE> tile2DArray;
							CTPoint<int> startTile;
							FillTileSet( &tile2DArray,
													&startTile,
													*pField,
													polygon,
													CTPoint<int>( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH,
																				pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ),
													AI_TILE_SIZE * 2.0f,
													&visDistances );
							//Trace2DByteArray( tile2DArray, "CFieldState::UpdatePolygon()" );
							CVec3 vDiffPos = VNULL3;
							CArray2D<BYTE> diff;
							if ( IEditorScene *pScene = EditorScene() )
							{
								if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
								{
									pTerraManager->GetTileTypeUpdateDifferences( &( vDiffPos.x ), &( vDiffPos.y ), &diff );
									pTerraManager->UpdateTileAreaType( startTile.x * AI_TILE_SIZE * 2.0f,
																										startTile.y * AI_TILE_SIZE * 2.0f,
																										tile2DArray,
																										NTerraBrush::TERRA_BRUSH_OVERRIDE );
									pTerraManager->GetTileTypeUpdateDifferences( &( vDiffPos.x ), &( vDiffPos.y ), &diff );
								}
							}
							if ( ( diff.GetSizeX() > 0 ) && ( diff.GetSizeY() > 0 ) )
							{ 
								pMapInfoController->AddChangeTileOperation( diff, vDiffPos );
							}
						}
						NProgress::IteratePosition();	// 3
						// заполняем высоты
						if ( pEditParameters->bFillHeights )
						{
							SHeightPattern heightPattern;
							FillProfilePattern( &heightPattern,
																	*pField,
																	polygon,
																	CTPoint<int>( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH,
																								pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH ),
																	AI_TILE_SIZE * 2.0f,
																	&visDistances );
							//Trace2DFloatArray( heightPattern.heights, "CFieldState::UpdatePolygon()" );
							CTPoint<int> diffPos = CTPoint<int>( 0, 0 );
							CArray2D<float> diff;
							if ( IEditorScene *pScene = EditorScene() )
							{
								if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
								{
									pTerraManager->GetTerraGeometryUpdateDifferences( &( diffPos.x ), &( diffPos.y ), &diff );
									pTerraManager->ModifyTerraGeometryByBrush( heightPattern.pos.x,
																														heightPattern.pos.y,
																														false,
																														heightPattern.heights,
																														NTerraBrush::TERRA_BRUSH_OVERRIDE );
									pTerraManager->GetTerraGeometryUpdateDifferences( &( diffPos.x ), &( diffPos.y ), &diff );
								}
							}
							if ( ( diff.GetSizeX() > 0 ) && ( diff.GetSizeY() > 0 ) )
							{ 
								pMapInfoController->AddChangeHeightOperation( diff, diffPos );
							}
						}
						NProgress::IteratePosition();	// 4
						// заполняем объекты
						if ( pEditParameters->bFillObjects )
						{
							CArray2D<BYTE> aiTileMap;
							aiTileMap.SetSizes( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * 2,
																	pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * 2 );
							aiTileMap.FillZero();
							FillObjectSet( pMapInfoEditor,
														pMapInfoController,
														*pField,
														polygon,
														CTPoint<int>( pMapInfoEditor->pMapInfo->nNumPatchesX * VIS_TILES_IN_PATCH * 2,
																					pMapInfoEditor->pMapInfo->nNumPatchesY * VIS_TILES_IN_PATCH * 2 ),
														AI_TILE_SIZE,
														&aiDistances,
														&aiTileMap );
						}
						NProgress::IteratePosition();	// 5
						pMapInfoController->Redo( false, true, pMapInfoEditor );
						Singleton<IControllerContainer>()->Add( pMapInfoController );
						NProgress::IteratePosition();	// 6
						//
						pMapInfoEditor->SetModified( true );
						Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
						NProgress::Destroy();
					}
				}
			}
		}
	}
}


bool CFieldState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	if ( !CPolygonState::HandleCommand( nCommandID, dwData ) )
	{
		switch( nCommandID )
		{
			case ID_GET_EDIT_PARAMETERS:
			{
				if ( SEditParameters *pEditParameters = GetEditParameters() )
				{
					pEditParameters->nFlags = dwData;
					::GetEditParameters( pEditParameters, CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
				}
				return true;
			}
			case ID_SET_EDIT_PARAMETERS:
			{
				if ( SEditParameters *pEditParameters = GetEditParameters() )
				{
					pEditParameters->nFlags = dwData;
					SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
				}
				return true;
			}
			case ID_UPDATE_EDIT_PARAMETERS:
			{
				if ( SEditParameters *pEditParameters = GetEditParameters() )
				{
					UpdateEditParameters( dwData );
					SetEditParameters( *pEditParameters, CHID_MAPINFO_TERRAIN_FIELD_WINDOW );
				}
				return true;
			}
			default:
				return false;
		}
		return false;
	}
	return true;
}


bool CFieldState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CFieldState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CFieldState::UpdateCommand(), pbCheck == 0" );
	//
	if ( !CPolygonState::UpdateCommand( nCommandID, pbEnable, pbCheck ) )
	{
		switch( nCommandID )
		{
			case ID_GET_EDIT_PARAMETERS:
			case ID_SET_EDIT_PARAMETERS:
			case ID_UPDATE_EDIT_PARAMETERS:
				( *pbEnable ) = true;
				( *pbCheck ) = false;
				return true;
			default:
				return false;
		}
		return false;
	}
	return true;
}


void CFieldState::CreateTileSetWeightVectorList( CTileSetWeightVectorList *pTileSetWeightVectorList, const NDb::SField &rField )
{
	NI_ASSERT( pTileSetWeightVectorList != 0, "CreateTileSetWeightVectorList(), invalid parameter: pTileSetWeightVectorList == 0" );
	if ( !rField.tileShells.empty() )
	{
		pTileSetWeightVectorList->resize( rField.tileShells.size() );
		for ( int nShellIndex = 0; nShellIndex < rField.tileShells.size(); ++nShellIndex )
		{
			for ( int nTileIndex = 0; nTileIndex < rField.tileShells[nShellIndex].tiles.size(); ++nTileIndex )
			{
				( *pTileSetWeightVectorList )[nShellIndex].push_back( rField.tileShells[nShellIndex].tiles[nTileIndex].nValue, rField.tileShells[nShellIndex].tiles[nTileIndex].nWeight );
			}
		}
	}
	else
	{
		pTileSetWeightVectorList->clear();
	}
}


void CFieldState::CreateObjectSetWeightVectorList( CObjectSetWeightVectorList *pObjectSetWeightVectorList, const NDb::SField &rField )
{
	NI_ASSERT( pObjectSetWeightVectorList != 0, "CreateObjectSetWeightVectorList(), invalid parameter: pObjectSetWeightVectorList == 0" );
	if ( !rField.objectShells.empty() )
	{
		pObjectSetWeightVectorList->resize( rField.objectShells.size() );
		for ( int nShellIndex = 0; nShellIndex < rField.objectShells.size(); ++nShellIndex )
		{
			for ( int nObjectIndex = 0; nObjectIndex < rField.objectShells[nShellIndex].objects.size(); ++nObjectIndex )
			{
				( *pObjectSetWeightVectorList )[nShellIndex].push_back( rField.objectShells[nShellIndex].objects[nObjectIndex].pValue, rField.objectShells[nShellIndex].objects[nObjectIndex].nWeight );
			}
		}
	}
	else
	{
		pObjectSetWeightVectorList->clear();
	}
}


//сервисная функция, определяет точки вхождения в полигон для слоя сетки
//возвращает количество элементов в pXPosList
//лист предствавляет из себя набор пар, - начало и конец отрезков входящих в полигон
//концы отрезков включаются в полигон, для включения точек
int CFieldState::GetPolygonLine( int nYPos, float fSide, const CFieldPolygon &rPolygon, const CTPoint<int> &rXBounds, CXPosList *pXPosList )
{
	NI_ASSERT( pXPosList != 0, "GetPolygonLine(), invalid parameter: pXPosList == 0" );
	pXPosList->clear();
	if ( rPolygon.empty() )
	{
		return 0;
	}

	CFieldPolygon::const_iterator currentPointIterator0 = rPolygon.begin();
	CFieldPolygon::const_iterator currentPointIterator1 = rPolygon.begin();

	++currentPointIterator1;
	if ( currentPointIterator1 == rPolygon.end() )
	{
		if ( ( currentPointIterator0->y / fSide ) == nYPos )
		{
			const int nXPos = currentPointIterator0->x / fSide;
			pXPosList->resize( 2, nXPos );
		}
		return pXPosList->size();
	}
	
	const float fY = ( nYPos * fSide ) + ( fSide / 2.0f );

	while ( currentPointIterator0 != rPolygon.end() )
	{
		if ( ( ( currentPointIterator1->y <= fY ) && ( fY < currentPointIterator0->y ) ) ||
				 ( ( currentPointIterator0->y <= fY ) && ( fY < currentPointIterator1->y ) ) )
		{
			if ( currentPointIterator0->x == currentPointIterator1->x )
			{
				pXPosList->push_back( currentPointIterator0->x / fSide );
			}
			else
			{
				const float fA = ( currentPointIterator0->y - currentPointIterator1->y ) / ( currentPointIterator0->x - currentPointIterator1->x );
				const float fB = currentPointIterator0->y - ( fA * currentPointIterator0->x );
				const float fX = ( fY - fB ) / fA;
				pXPosList->push_back( fX / fSide );
			}
		}
		++currentPointIterator0;
		++currentPointIterator1;
		if ( currentPointIterator1 == rPolygon.end() )
		{
			currentPointIterator1 = rPolygon.begin();
		}
	}
	sort( pXPosList->begin(), pXPosList->end() );
	int nOverMinCount = 0;
	int nOverMaxCount = 0;
	for ( CXPosList::iterator itXPos = pXPosList->begin(); itXPos < pXPosList->end(); ++itXPos )
	{
		if ( ( *itXPos ) < rXBounds.min )
		{
			++nOverMinCount;
		}
		if ( ( *itXPos ) >= rXBounds.max )
		{
			++nOverMaxCount;
		}
	}
	if ( nOverMinCount > 0 )
	{
		while ( nOverMinCount >= 2 )
		{
			CXPosList::iterator itXPosFirst = pXPosList->begin();
			CXPosList::iterator itXPosLast = pXPosList->begin();
			++itXPosLast;
			++itXPosLast;
			pXPosList->erase( itXPosFirst, itXPosLast );
			nOverMinCount -= 2;
		}
		if ( nOverMinCount == 1 )
		{
			CXPosList::iterator itXPos = pXPosList->begin();
			( *itXPos ) = rXBounds.x;
		}
	}
	if ( nOverMaxCount > 0 )
	{
		while ( nOverMaxCount >= 2 )
		{
			CXPosList::iterator itXPosFirst = pXPosList->end();
			CXPosList::iterator itXPosLast = pXPosList->end();
			--itXPosFirst;
			--itXPosFirst;
			pXPosList->erase( itXPosFirst, itXPosLast );
			nOverMaxCount -= 2;
		}
		if ( nOverMaxCount == 1 )
		{
			CXPosList::iterator itXPos = pXPosList->end();
			--itXPos;
			 ( *itXPos ) = rXBounds.max - 1;
		}
	}
	return pXPosList->size();
}


bool CFieldState::FillTileSet( CArray2D<BYTE> *pTile2DArray,
															 CTPoint<int> *pStartTile,
															 const NDb::SField &rField,
															 const CFieldPolygon &rPolygon,
															 const CTPoint<int> terrainSize,
															 float fTileSize,
															 CFieldDistanceMap *pDistances )
{
	NI_ASSERT( pTile2DArray != 0, "FillTileSet(): Invalid parameter pTile2DArray == 0" );
	NI_ASSERT( pStartTile != 0, "FillTileSet(): Invalid parameter pStartTile == 0" );
	//
	if ( rField.tileShells.empty() )
	{
		return true;
	}
	//
	CTRect<int> boundingRect( 0, 0, terrainSize.x, terrainSize.y );
	CTRect<float> boundingBox = GetPolygonBoundingBox( rPolygon );
	CTRect<int> indices( ( boundingBox.minx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.miny + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxy + ( fTileSize / 2.0f ) ) / fTileSize );
	//проверяем
	if ( ValidateRect( boundingRect, &indices ) < 0 )
	{
		return false;
	}
	//
	pStartTile->x = indices.minx;
	pStartTile->y = indices.miny;
	pTile2DArray->SetSizes( indices.maxx - indices.minx, indices.maxy - indices.miny );
	pTile2DArray->FillEvery( NO_AFFECTED_TILE );
	//
	// создаем список тайлов с весами для каждой оболочки
	CTileSetWeightVectorList tileSetWeightVectorList;
	CreateTileSetWeightVectorList( &tileSetWeightVectorList, rField );
	//пробегаем по тайлам
	for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
	{
		CXPosList xposList;
		int nCount = GetPolygonLine( nYIndex, fTileSize, rPolygon, CTPoint<int>( indices.minx, indices.maxx ), &xposList );
		string szIndices;
		for ( int i = 0; i < xposList.size(); ++i )
		{
			szIndices += StrFmt( "%d ", xposList[i] );
		}
		DebugTrace( "%d: %s", xposList.size(), szIndices.c_str() );
		// проверяем на четность
		if ( ( nCount > 0 ) && ( ( nCount & 0x1 ) == 0 ) )
		{
			CXPosList::const_iterator startXPosIterator = xposList.begin();
			CXPosList::const_iterator finishXPosIterator = xposList.begin();
			++finishXPosIterator;
			while ( true )
			{
				for ( int nXIndex = ( *startXPosIterator ); nXIndex <= ( *finishXPosIterator ); ++nXIndex )
				{
					float fDistance = 0.0f;
					if ( pDistances )
					{
						const LPARAM lParam = MAKELPARAM( nXIndex, nYIndex );
						CFieldDistanceMap::const_iterator distanceIterator = pDistances->find( lParam );
						if ( distanceIterator != pDistances->end() )
						{
							fDistance = distanceIterator->second;
						}
						else
						{
							const CVec2	vTileCenter( ( nXIndex * fTileSize ) + ( fTileSize / 2.0f ),
																			 ( nYIndex * fTileSize ) + ( fTileSize / 2.0f ) );
							fDistance = PolygonDistance( rPolygon, vTileCenter, true );
							( *pDistances )[lParam] = fDistance;
						}
					}
					else
					{
						const CVec2	vTileCenter( ( nXIndex * fTileSize ) + ( fTileSize / 2.0f ),
																		 ( nYIndex * fTileSize ) + ( fTileSize / 2.0f ) );
						fDistance = PolygonDistance( rPolygon, vTileCenter, true );
					}
					
					if ( fDistance >= 0 )
					{
						fDistance /= fTileSize;
						//определяем слой
						float fWidth = 0.0f;
						for ( int nShellIndex = 0; nShellIndex < rField.tileShells.size(); ++nShellIndex )
						{
							fWidth += rField.tileShells[nShellIndex].fWidth;
							if ( fWidth > fDistance )
							{
								if ( !tileSetWeightVectorList[nShellIndex].empty() )
								{
									( *pTile2DArray )[nYIndex - pStartTile->y][nXIndex - pStartTile->x] = tileSetWeightVectorList[nShellIndex].GetRandom( false );
								}
								break;
							}
						}
					}
				}
				//
				startXPosIterator += 2;
				if ( startXPosIterator == xposList.end() )
				{
					break;
				}
				finishXPosIterator += 2;
				//
			}
		}
	}
	return true;
}



struct SAddFunctional
{
	SHeightPattern *pHeightPattern;
	//
	SAddFunctional( SHeightPattern *_pHeightPattern )
		: pHeightPattern( _pHeightPattern )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		pHeightPattern->heights[nYIndex][nXIndex] += fValue;
		return true;
	}
};


struct SSubstractFunctional
{
	SHeightPattern *pHeightPattern;
	//
	SSubstractFunctional( SHeightPattern *_pHeightPattern )
		: pHeightPattern( _pHeightPattern )
	{
		NI_ASSERT( pHeightPattern != 0, "Wrong parameter: pHeightPattern == 0" );
	}
	//
	bool operator()( int nXIndex, int nYIndex, float fValue )
	{ 
		pHeightPattern->heights[nYIndex][nXIndex] -= fValue;
		return true;
	}
};


bool CFieldState::FillProfilePattern(	SHeightPattern *pHeightPattern,
																			const NDb::SField &rField,
																			const CFieldPolygon &rPolygon,
																			const CTPoint<int> terrainSize,
																			float fTileSize,
																			CFieldDistanceMap *pDistances )
{
	NI_ASSERT( pHeightPattern != 0, "FillProfilePattern(): Invalid parameter pHeightPattern == 0" );
	// приводим в порядок входные данные
	float fHeight = rField.fHeight;
	float fPositiveRatio = rField.fPositiveRatio;
	CTPoint<int> patternSize( rField.patternSize.nMin, rField.patternSize.nMax );
	string szProfileFileName = rField.szProfileFileName;
	//
	if ( fHeight < 0.0f )
	{
		return true;
	}
	if ( fPositiveRatio > 1.0f )
	{
		fPositiveRatio = 1.0f;
	}
	else if ( fPositiveRatio < 0.0f ) 
	{
		fPositiveRatio = 0.0f;
	}
	if ( patternSize.x > 6 )
	{
		patternSize.x = 6;
	}
	else if ( patternSize.x < 1 )
	{
		patternSize.x = 1;
	}
	if ( patternSize.y > 6 )
	{
		patternSize.y = 6;
	}
	else if ( patternSize.y < 1 )
	{
		patternSize.y = 1;
	}
	if ( patternSize.x > patternSize.y )
	{
		const int nBackup = patternSize.x;
		patternSize.x = patternSize.y;
		patternSize.y = nBackup;
	}
	if ( !Singleton<IUserDataContainer>()->Get()->NormalizePath( &( szProfileFileName ), true, true, true, SUserData::NPT_EXPORT_DESTINATION, 0 ) )
	{
		return true;	
	}
	//
	CTRect<int> boundingRect( 0, 0, terrainSize.x, terrainSize.y );
	CTRect<float> boundingBox = GetPolygonBoundingBox( rPolygon );
	CTRect<int> indices( ( boundingBox.minx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.miny + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxy + ( fTileSize / 2.0f ) ) / fTileSize );
	//проверяем
	if ( ValidateRect( boundingRect, &indices ) < 0 )
	{
		return false;
	}
	SAddFunctional addFunctional( pHeightPattern );
	SSubstractFunctional substractFunctional( pHeightPattern );

	pHeightPattern->pos.x = indices.minx;
	pHeightPattern->pos.y = indices.miny;
	pHeightPattern->fRatio = 1.0f;
	pHeightPattern->heights.SetSizes( indices.maxx - indices.minx, indices.maxy - indices.miny );
	pHeightPattern->heights.FillZero();
	CTRect<int> patternRect( 0, 0, pHeightPattern->heights.GetSizeX(), pHeightPattern->heights.GetSizeY() );
	//
	// создаем градиент
	SGradient gradient;
	{
		CArray2D<DWORD> image;
		CFileStream stream( NVFS::GetMainVFS(), szProfileFileName );
		if ( stream.IsOk() )
		{
			if ( NImage::LoadTGAImage( image, &stream ) )
			{
				gradient.CreateFromImage( image, CTPoint<float>( 0.0f, 1.0f ), CTPoint<float>( 0.0f, fHeight ) );
			}
		}
	}
	vector<SHeightPattern> patternList;
	for ( int nPatternSize = patternSize.x; nPatternSize <= patternSize.y; ++nPatternSize )
	{
		vector<SHeightPattern>::iterator posPattern = patternList.insert( patternList.end(), SHeightPattern() );
		posPattern->CreateByGradient( 1.0f, nPatternSize * 2, gradient );
	}
	//пробегаем по тайлам
	for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
	{
		CXPosList xposList;
		int nCount = GetPolygonLine( nYIndex, fTileSize, rPolygon, CTPoint<int>( indices.minx, indices.maxx ), &xposList );
		// проверяем на четность
		if ( ( nCount > 0 ) && ( ( nCount & 0x1 ) == 0 ) )
		{
			CXPosList::const_iterator startXPosIterator = xposList.begin();
			CXPosList::const_iterator finishXPosIterator = xposList.begin();
			++finishXPosIterator;
			while ( true )
			{
				for ( int nXIndex = ( *startXPosIterator ); nXIndex <= ( *finishXPosIterator ); ++nXIndex )
				{
					float fDistance = 0.0f;
					if ( pDistances )
					{
						const LPARAM lParam = MAKELPARAM( nXIndex, nYIndex );
						CFieldDistanceMap::const_iterator distanceIterator = pDistances->find( lParam );
						if ( distanceIterator != pDistances->end() )
						{
							fDistance = distanceIterator->second;
						}
						else
						{
							const CVec2	vTileCenter( ( nXIndex * fTileSize ) + ( fTileSize / 2.0f ),
																			 ( nYIndex * fTileSize ) + ( fTileSize / 2.0f ) );
							fDistance = PolygonDistance( rPolygon, vTileCenter, true );
							( *pDistances )[lParam] = fDistance;
						}
					}
					else
					{
						const CVec2	vTileCenter( ( nXIndex * fTileSize ) + ( fTileSize / 2.0f ),
																		 ( nYIndex * fTileSize ) + ( fTileSize / 2.0f ) );
						fDistance = PolygonDistance( rPolygon, vTileCenter, true );
					}
					
					if ( fDistance >= 0 )
					{
						// обновляем высоты
						fDistance = ( fDistance / fTileSize ) + 0.5f;
						//
						const int nPatternIndex = NWin32Random::Random( patternList.size() );
						SHeightPattern &rHeightPattern = patternList[nPatternIndex];
						//
						if ( int( fDistance ) >= ( rHeightPattern.heights.GetSizeX() / 2 ) )
						{
							CTPoint<int> cornerTile( nXIndex - pHeightPattern->pos.x - rHeightPattern.heights.GetSizeX() / 2,
																			 nYIndex - pHeightPattern->pos.y - rHeightPattern.heights.GetSizeY() / 2 );
							rHeightPattern.pos = cornerTile;
							if ( NWin32Random::Random( 0.0f, 1.0f ) < fPositiveRatio )
							{
								ApplyHeightPattern( patternRect, rHeightPattern, addFunctional, true );
							}
							else
							{
								ApplyHeightPattern( patternRect, rHeightPattern, substractFunctional, true );
							}
						}
					}
				}
				//
				startXPosIterator += 2;
				if ( startXPosIterator == xposList.end() )
				{
					break;
				}
				finishXPosIterator += 2;
				//
			}
		}
	}
	return true;
}


template<class Type, class PointType>
struct ModifyTilesFunctional
{
	PointType bValue;
	Type *pLockArray;

	ModifyTilesFunctional( PointType _bValue, Type *_pLockArray )
		: bValue ( _bValue ), pLockArray( _pLockArray )
	{
		NI_ASSERT( pLockArray != 0, "Wrong parameter: pLockArray == 0" );
	}

	bool operator()( int nXIndex, int nYIndex )
	{ 
		(*pLockArray)[nYIndex][nXIndex] = bValue;
		return true;
	}
};


template<class Type, class PointType>
struct CheckTilesFunctional
{
	PointType bValue;
	const Type *pLockArray;
	bool isPresent;

	CheckTilesFunctional( PointType _bValue, const Type *_pLockArray )
		: bValue ( _bValue ), pLockArray( _pLockArray ), isPresent( false )
	{
		NI_ASSERT( pLockArray != 0, "Wrong parameter: pLockArray == 0" );
	}

	bool operator()( int nXIndex, int nYIndex )
	{ 
		if ( (*pLockArray)[nYIndex][nXIndex] == bValue )
		{
			isPresent = true;
		}
		return ( !isPresent );
	}
};


/**
template<class TYPE>
bool ApplyTilesInObjectsPassability( const CTRect<int> &rRect,										//границы применимости функционалов
																		 const SMapObjectInfo *pMapObjectInfo,				//казатель на массив обьектов
																		 int nMapObjectInfoCount,											//число обьектов
																		 TYPE &rApplyFunctional,											//функционал
																		 bool isIgnoreInvalidIndices = false )				//пропускать обьекты за краями карты
{
	IObjectsDB *pIDB = GetSingleton<IObjectsDB>();
	if ( !pIDB )
	{
		return false;
	}

	for ( int nObjectIndex = 0; nObjectIndex < nMapObjectInfoCount; ++nObjectIndex )
	{
		const SGDBObjectDesc* pGDBObjectDesc = pIDB->GetDesc( pMapObjectInfo[nObjectIndex].szName.c_str() );
		if ( ( pGDBObjectDesc != 0 ) &&
				 ( IsObjectHasPassability( pGDBObjectDesc->eGameType ) ) )
		{
			const SStaticObjectRPGStats* pStaticObjectRPGStats = NGDB::GetRPGStats<SStaticObjectRPGStats>( pMapObjectInfo[nObjectIndex].szName.c_str() );
			const CVec2 &rOrigin = pStaticObjectRPGStats->GetOrigin( pMapObjectInfo[nObjectIndex].nFrameIndex );
			const CArray2D<BYTE> &rPassability = pStaticObjectRPGStats->GetPassability( pMapObjectInfo[nObjectIndex].nFrameIndex );
			CTPoint<int> start( ( pMapObjectInfo[nObjectIndex].vPos.x - rOrigin.x + ( SAIConsts::TILE_SIZE / 2.0 ) ) / SAIConsts::TILE_SIZE,
													( pMapObjectInfo[nObjectIndex].vPos.y - rOrigin.y + ( SAIConsts::TILE_SIZE / 2.0 ) ) / SAIConsts::TILE_SIZE );
			
			CTRect<int> indices( start.x, start.y, start.x + rPassability.GetSizeX(), start.y + rPassability.GetSizeY() );
			int result = ValidateRect( rRect, &indices );
			//нет ни одного тайла
			if ( result < 0 )
			{
				if ( isIgnoreInvalidIndices )
				{
					//скипаем обьект, переходим к следующему
					continue;
				}
				else
				{
					//возвращаем ошибку
					return false;
				}
			}
			//пассабилити выходит за границы массива
			if ( ( result < 1 ) && !isIgnoreInvalidIndices )
			{
				//возвращаем ошибку
				return false;
			}
			//пробегаем по тайлам
			for ( int nXIndex = indices.minx; nXIndex < indices.maxx; ++nXIndex )
			{
				for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
				{
					if ( rPassability[nYIndex - start.y][nXIndex - start.x] != RMGC_UNLOCKED )
					{
						if ( !rApplyFunctional( nXIndex, nYIndex ) )
						{
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}
/**/


bool CFieldState::FillObjectSet( CMapInfoEditor *pMapInfoEditor,
																 CObjectBaseController *pObjectController,
																 const NDb::SField &rField,
																 const CFieldPolygon &rPolygon,
																 const CTPoint<int> terrainSize,
																 float fTileSize,
																 CFieldDistanceMap *pDistances,
																 CArray2D<BYTE> *pTileMap )
{
	NI_ASSERT( pMapInfoEditor != 0, "FillObjectSet(): Invalid parameter pMapInfoEditor == 0" );
	//
	if ( rField.objectShells.empty() )
	{
		return true;
	}
	if ( pTileMap )
	{
		NI_ASSERT( ( pTileMap->GetSizeX() >= terrainSize.x ) &&
							 ( pTileMap->GetSizeY() >= terrainSize.y ),
							 StrFmt( "FillObjectSet(): TileMap.size: [%d. %d], terrainSize: [%d, %d]",
											 pTileMap->GetSizeX(),
											 pTileMap->GetSizeY(),
											 terrainSize.x,
											 terrainSize.y ) );
	}
	//
	ModifyTilesFunctional<CArray2D<BYTE>, BYTE> tileMapModifyTiles( 1, pTileMap );
	CheckTilesFunctional<CArray2D<BYTE>, BYTE> tileMapCheckTiles( 1, pTileMap );
	//
	CTRect<int> boundingRect( 0, 0, terrainSize.x, terrainSize.y );
	CTRect<float> boundingBox = GetPolygonBoundingBox( rPolygon );
	CTRect<int> indices( ( boundingBox.minx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.miny + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxx + ( fTileSize / 2.0f ) ) / fTileSize,
											 ( boundingBox.maxy + ( fTileSize / 2.0f ) ) / fTileSize );
	//проверяем
	if ( ValidateRect( boundingRect, &indices ) < 0 )
	{
		return false;
	}
	// создаем список объектов с весами для каждой оболочки
	CObjectSetWeightVectorList objectSetWeightVectorList;
	CreateObjectSetWeightVectorList( &objectSetWeightVectorList, rField );
	//пробегаем по тайлам
	IManipulator *pManipulator = pMapInfoEditor->GetViewManipulator();
	IEditorScene *pScene = EditorScene();
	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	if ( ( !pManipulator ) || ( !pScene ) )
	{
		return false;
	}
	for ( int nYIndex = indices.miny; nYIndex < indices.maxy; ++nYIndex )
	{
		CXPosList xposList;
		int nCount = GetPolygonLine( nYIndex, fTileSize, rPolygon, CTPoint<int>( indices.minx, indices.maxx ), &xposList );
		// проверяем на четность
		if ( ( nCount > 0 ) && ( ( nCount & 0x1 ) == 0 ) )
		{
			CXPosList::const_iterator startXPosIterator = xposList.begin();
			CXPosList::const_iterator finishXPosIterator = xposList.begin();
			++finishXPosIterator;
			while ( true )
			{
				for ( int nXIndex = ( *startXPosIterator ); nXIndex <= ( *finishXPosIterator ); ++nXIndex )
				{
					const CVec2	vTileCenter( ( nXIndex * fTileSize ) + ( fTileSize / 2.0f ),
																		( nYIndex * fTileSize ) + ( fTileSize / 2.0f ) );

					float fDistance = 0.0f;
					if ( pDistances )
					{
						const LPARAM lParam = MAKELPARAM( nXIndex, nYIndex );
						CFieldDistanceMap::const_iterator distanceIterator = pDistances->find( lParam );
						if ( distanceIterator != pDistances->end() )
						{
							fDistance = distanceIterator->second;
						}
						else
						{
							fDistance = PolygonDistance( rPolygon, vTileCenter, true );
							( *pDistances )[lParam] = fDistance;
						}
					}
					else
					{
						fDistance = PolygonDistance( rPolygon, vTileCenter, true );
					}
					
					if ( fDistance >= 0 )
					{
						fDistance /= fTileSize;
						//определяем слой
						float fWidth = 0.0f;
						for ( int nShellIndex = 0; nShellIndex < rField.objectShells.size(); ++nShellIndex )
						{
							fWidth += rField.objectShells[nShellIndex].fWidth;
							if ( fWidth > fDistance )
							{																				
								if ( ( rField.objectShells[nShellIndex].nBetweenDistance > 0 ) && ( !rField.objectShells[nShellIndex].objects.empty() ) )
								{
									const float fAdditionalRatio = 1.0f / fabs2( rField.objectShells[nShellIndex].nBetweenDistance );
									if ( NWin32Random::Random( 0.0f, 1.0f ) <= ( rField.objectShells[nShellIndex].fRatio * fAdditionalRatio ) )
									{
										if ( CDBPtr<NDb::SHPObjectRPGStats> pHPObjectRPGStats = objectSetWeightVectorList[nShellIndex].GetRandom( false ) )
										{
											const CVec3 vPosition( ( nXIndex * fTileSize ) + NWin32Random::Random( 0.0f, fTileSize ),
																						( nYIndex * fTileSize ) + NWin32Random::Random( 0.0f, fTileSize ),
																						0.0f );
											const float fDirection = NWin32Random::Random( 0.0f, FP_2PI );
											/**
											if ( pTileMap )
											{
												tileMapCheckTiles.isPresent = false;
												if ( ApplyTilesInObjectsPassability( boundingRect, pHPObjectRPGStats, tileMapCheckTiles ) )
												{
													//локаем тайлы обьекта
													ApplyTilesInObjectsPassability( boundingRect, pHPObjectRPGStats, tileMapModifyTiles );
													bAddObject = true;
												}
											}
											/**/
											//добавляем объект
											NMapInfoEditor::SObjectCreateInfo objectCreateInfo;
											objectCreateInfo.vPosition = vPosition;
											objectCreateInfo.fDirection = fDirection;
											objectCreateInfo.szRPGStatsTypeName = NDb::GetClassTypeName( pHPObjectRPGStats->GetDBID() );
											objectCreateInfo.rpgStatsDBID = pHPObjectRPGStats->GetDBID();
											objectCreateInfo.nFrameIndex = INVALID_NODE_ID;
											objectCreateInfo.nPlayer = 0;
											objectCreateInfo.fHP = 1.0f;
											objectCreateInfo.bRotateTo90Degree = pMapInfoEditor->editorSettings.bRotateTo90Degree;
											objectCreateInfo.bFitToGrid = pMapInfoEditor->editorSettings.bFitToGrid;
											UINT nSimpleObjectInfoID = INVALID_NODE_ID;
											if ( NMapInfoEditor::SSimpleObjectInfo *pSimpleObjectInfo = pMapInfoEditor->objectInfoCollector.Insert( static_cast<NMapInfoEditor::SSimpleObjectInfo*>( 0 ), &nSimpleObjectInfoID ) )
											{
												pSimpleObjectInfo->Create( &objectCreateInfo, pScene, pObjectController, pManipulator );
											}
										}
									}
								}
								break;
							}
						}
					}
				}
				//
				startXPosIterator += 2;
				if ( startXPosIterator == xposList.end() )
				{
					break;
				}
				finishXPosIterator += 2;
			}
		}
	}
	return true;
}


CFieldState::SEditParameters* CFieldState::GetEditParameters()
{ 
	return ( ( pMapInfoEditor != 0 ) ? &( pMapInfoEditor->editorSettings.epFieldState ) : 0 );
}


int CFieldState::SEditParameters::operator&( IXmlSaver &xs )
{
	xs.Add( "MoveType", &eMoveType );
	xs.Add( "FieldIndex", &nFieldIndex );
	xs.Add( "Randomize", &bRandomize );
	xs.Add( "FillTerrain", &bFillTerrain );
	xs.Add( "FillObjects", &bFillObjects );
	xs.Add( "FillHeights", &bFillHeights );
	//
	//do not serialise this fields:
	// UINT nFlags;
	//CFieldList fieldList;
	//float fMinLength;
	//float fWidth;
	//float fDisturbance;
	// bool bUpdateMap;
	return 0;
}

// basement storage  


