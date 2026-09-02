#include "stdafx.h"
#include <fmt/format.h>

#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "SceneB2/Scene.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "DrawToolsDC.h"
#include "EnterNameDialog.h"
#include "ScriptAreaState.h"

#include <cstdint>

void CScriptAreaState::Clear()
{
	scriptAreaMap.clear();
	scriptAreaIDToIndexCollector.Clear();
	scriptAreaIDCollector.Clear();
}


// прочитать список скриптовых областей из базы
void CScriptAreaState::GetScriptAreaMap()
{
	if ( ( pMapInfoEditor == 0 ) || ( pMapInfoEditor->GetViewManipulator() == 0  ) )
	{
		return;
	}
	//
	IManipulator *pManipulator = pMapInfoEditor->GetViewManipulator();
	int nScriptAreaCount = 0;
	if ( !CManipulatorManager::GetValue( &nScriptAreaCount, pManipulator, "ScriptAreas" ) )
	{
		return;
	}
	//
	Clear();
	//
	for ( int nScriptAreIndex = 0; nScriptAreIndex < nScriptAreaCount; ++nScriptAreIndex )
	{
		const std::string szScriptAreaPefix = fmt::format( "ScriptAreas.[{}]", nScriptAreIndex );
		//
		CScriptAreaState::SScriptArea scriptArea;
		// 
		std::string szType;
		if ( !CManipulatorManager::GetValue( &szType, pManipulator, szScriptAreaPefix + ".Type" ) )
		{
			continue;
		}
		//
		if ( szType == "EAT_RECTANGLE" )
		{
			scriptArea.eType = NDb::EAT_RECTANGLE;
		}
		else if ( szType == "EAT_CIRCLE" )
		{
			scriptArea.eType = NDb::EAT_CIRCLE;
		}
		//
		if ( !CManipulatorManager::GetValue( &scriptArea.szName, pManipulator, szScriptAreaPefix + ".Name" ) )
		{
			continue;
		}
		//
		CVec2 v0 = VNULL2;
		if ( !CManipulatorManager::GetVec2<CVec2,float>( &v0, pManipulator, szScriptAreaPefix + ".Center" ) ) 
		{
			continue;
		}
		//
		CVec2 v1 = VNULL2;
		if ( !CManipulatorManager::GetVec2<CVec2,float>( &v1, pManipulator, szScriptAreaPefix + ".AABBHalfSize" ) ) 
		{
			continue;
		}
		//
		float fR = 0;
		if ( !CManipulatorManager::GetValue( &fR, pManipulator, szScriptAreaPefix + ".R" ) ) 
		{
			continue;
		}
		//
		if ( scriptArea.eType == NDb::EAT_CIRCLE )
		{
			// для круговых областей радиус сохраняется как в виде float в .R
			// так и в виде радиус-вектора в .AABBHalfSize
			// (игра использует .R, редактор по возможности .AABBHalfSize)
			float r = fabs( v1 );
			if ( !( r > FP_EPSILON ) )
			{
				v1 = V2_AXIS_X * fR;
			}
		}
		// v0 == center
		scriptArea.cpList.push_back( CVec3( v0.x, v0.y, 0 ) );
		v1 += v0;
		scriptArea.cpList.push_back( CVec3( v1.x, v1.y, 0 ) );
		//
		scriptArea.nScriptAreaID = scriptAreaIDCollector.LockID();
		scriptAreaIDToIndexCollector.Insert( scriptArea.nScriptAreaID, nScriptAreIndex, false );
		scriptAreaMap[scriptArea.nScriptAreaID] = scriptArea;
	}
}


void CScriptAreaState::UpdateScriptArea( unsigned nScriptAreaID )
{
	if ( ( pMapInfoEditor == 0 ) || ( pMapInfoEditor->GetViewManipulator() == 0  ) )
	{
		return;
	}
	CScriptAreaMap::const_iterator posScriptArea = scriptAreaMap.find( nScriptAreaID );
	if ( posScriptArea == scriptAreaMap.end() )
	{
		return;
	}
	IManipulator *pManipulator = pMapInfoEditor->GetViewManipulator();
	const int nScriptAreaIndex = scriptAreaIDToIndexCollector.Get( nScriptAreaID );
	const std::string szScriptAreaPefix = fmt::format( "ScriptAreas.[{}]", nScriptAreaIndex );
	//
	std::string szType;
	if ( posScriptArea->second.eType == NDb::EAT_RECTANGLE )
	{
		szType = "EAT_RECTANGLE";
	}
	else if ( posScriptArea->second.eType == NDb::EAT_CIRCLE )
	{ 
		szType = "EAT_CIRCLE";
	}
	//
	CManipulatorManager::SetValue( szType, pManipulator, szScriptAreaPefix + ".Type" );
	CManipulatorManager::SetValue( posScriptArea->second.szName, pManipulator, szScriptAreaPefix + ".Name" );
	CVec2 vCenter = posScriptArea->second.GetCenter();
	CManipulatorManager::SetVec2( vCenter, pManipulator, szScriptAreaPefix + ".Center" );
	if ( posScriptArea->second.eType == NDb::EAT_RECTANGLE )
	{
		CVec2 vAABBHalfSize = posScriptArea->second.GetAABBHalfSize();
		CManipulatorManager::SetVec2( vAABBHalfSize, pManipulator, szScriptAreaPefix + ".AABBHalfSize" );
	}
	else if ( posScriptArea->second.eType == NDb::EAT_CIRCLE )
	{
		float fR = posScriptArea->second.GetRadius();
		CManipulatorManager::SetValue( fR, pManipulator, szScriptAreaPefix + ".R" );
	}
}


unsigned CScriptAreaState::InsertScriptArea( NDb::EScriptAreaTypes eType,
																				 const std::string &rszName,
																				 const CVec3 &rStart,
																				 const CVec3 &rFinish )
{
	if ( ( pMapInfoEditor == 0 ) || ( pMapInfoEditor->GetViewManipulator() == 0  ) )
	{
		return INVALID_NODE_ID;
	}
	IManipulator *pManipulator = pMapInfoEditor->GetViewManipulator();
	//
	int nScriptAreaIndex = 0;
	if ( !CManipulatorManager::GetValue( &nScriptAreaIndex, pManipulator, "ScriptAreas" ) )
	{
		return INVALID_NODE_ID;
	}
	if ( !pManipulator->InsertNode( "ScriptAreas" ) )
	{
		return INVALID_NODE_ID;
	}
	//
	SScriptArea scriptArea;
	scriptArea.eType = eType;
	scriptArea.szName = rszName;
	scriptArea.cpList.push_back( rStart );
	scriptArea.cpList.push_back( rFinish );
	scriptArea.nScriptAreaID = scriptAreaIDCollector.LockID();
	scriptAreaIDToIndexCollector.Insert( scriptArea.nScriptAreaID, nScriptAreaIndex, false );
	scriptAreaMap[scriptArea.nScriptAreaID] = scriptArea;
	UpdateScriptArea( scriptArea.nScriptAreaID );
	//
	SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SET_ALL );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
	//
	return scriptArea.nScriptAreaID;
}


void CScriptAreaState::RemoveScriptArea( unsigned nScriptAreaID )
{
	if ( ( pMapInfoEditor == 0 ) || ( pMapInfoEditor->GetViewManipulator() == 0  ) )
	{
		return;
	}
	CScriptAreaMap::iterator posScriptArea = scriptAreaMap.find( nScriptAreaID );
	if ( posScriptArea == scriptAreaMap.end() )
	{
		return;
	}
	IManipulator *pManipulator = pMapInfoEditor->GetViewManipulator();
	const int nScriptAreaIndex = scriptAreaIDToIndexCollector.Get( nScriptAreaID );
	//
	if ( pManipulator->RemoveNode( "ScriptAreas", nScriptAreaIndex ) )
	{
		scriptAreaIDCollector.FreeID( nScriptAreaID );
		scriptAreaIDToIndexCollector.Remove( nScriptAreaID, true );
		scriptAreaMap.erase( posScriptArea );
	}
	//
	SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SET_ALL );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
}


CScriptAreaState::CScriptAreaState( CMapInfoEditor *_pMapInfoEditor ) : pMapInfoEditor( _pMapInfoEditor ), bShift( false ), scriptAreaIDToIndexCollector( INVALID_NODE_ID )
{
	NI_ASSERT( pMapInfoEditor, "CScriptAreaState::CScriptAreaState(): pMapInfoEditor == 0" );
}


void CScriptAreaState::Enter()
{
	CPolygonState::Enter();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_SCRIPT_AREA_STATE, this );
	//
	GetScriptAreaMap();
	//
	SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SET_ALL );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CScriptAreaState::Leave()
{
	CPolygonState::Leave();
	//
	dialogData.Clear();
	Clear();
	//
	SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SET_ALL );
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCRIPT_AREA_STATE );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CScriptAreaState::Draw( CPaintDC *pPaintDC )
{
	sceneDrawTool.Clear();
	//
	CPolygonState::Draw( pPaintDC );
	//
	for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
	{
		itScriptArea->second.Draw( pPaintDC, &sceneDrawTool );
	}
	//
	sceneDrawTool.Draw();
}


CPolygonState::EMoveType CScriptAreaState::GetMoveType()
{
	return CPolygonState::MT_MULTI;
}


void CScriptAreaState::GetBounds( int *pnMinCount, int *pnMaxCount )
{
	*pnMinCount = 2;
	*pnMaxCount = 2;
}


CPolygonState::CControlPointList* CScriptAreaState::GetControlPoints( int nPolygonID )
{
	CScriptAreaMap::iterator posScriptArea = scriptAreaMap.find( nPolygonID );
	if ( posScriptArea != scriptAreaMap.end() )
	{
		return &( posScriptArea->second.cpList );
	}
	return 0;
}


bool CScriptAreaState::PrepareControlPoints( CPolygonState::CControlPointList *pControlPointList )
{
	return true;
}


void CScriptAreaState::PickPolygon( const CVec3 &rvPos, CPolygonIDList *pPickPolygonIDList )
{
	if ( !bShift )
	{
		// single-selection
		for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
		{
			itScriptArea->second.bSelected = false;
		}
	}
	//
	for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )	
	{
		if ( itScriptArea->second.Pick( rvPos ) )
		{
			pPickPolygonIDList->push_back( itScriptArea->second.nScriptAreaID );
			itScriptArea->second.bSelected = true;
			if ( !bShift )
			{
				// single-selection
				break;
			}
			else
			{
				// multi-selection
				continue;
			}
		}
	}
	//
	SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SELECTION );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CScriptAreaState::UpdatePolygon( int nPolygonID, EUpdateType eEpdateType )
{
	switch ( eEpdateType )
	{
	case UT_START:
	case UT_START_MOVE:
	case UT_CHANGE_POINT_NUMBER:
	case UT_CANCEL:
	case UT_FINISH:
		break;
	case UT_CONTINUE_MOVE:
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		break;
	case UT_FINISH_MOVE:
		{
			UpdateScriptArea( nPolygonID );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_UPDATE_VALUES, 0 );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		break;
	}
}


unsigned CScriptAreaState::InsertPolygon( const CControlPointList &rControlPointList )
{
	CEnterNameDialog enterNameDialog( Singleton<IMainFrameContainer>()->GetSECWorkbook(), "Area name", "Area name" );
	if ( enterNameDialog.DoModal() == IDOK )
	{
		std::string szName; 
		enterNameDialog.GetName( &szName );
		if ( !szName.empty() )
		{
			if ( rControlPointList.size() == 2 )
			{
				return InsertScriptArea( dialogData.eAreaType, szName, rControlPointList.front(), rControlPointList.back() );
			}
		}
	}
	return INVALID_NODE_ID;
}


void CScriptAreaState::RemovePolygon( int nPolygonID )
{
	RemoveScriptArea( nPolygonID );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


bool CScriptAreaState::SetScriptAreaWindowData( SScriptAreaWindowData::EChangeMask eChangeMask )
{
	dialogData.scriptAreaList.clear();
	dialogData.selectedScriptAreaIDList.clear();
	//
	for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
	{
		SScriptAreaWindowData::SScriptArea scriptAreaWindowData;
		scriptAreaWindowData.szName = itScriptArea->second.szName;
		scriptAreaWindowData.nScriptAreaID = itScriptArea->second.nScriptAreaID;
		scriptAreaWindowData.eType = itScriptArea->second.eType;
		//
		dialogData.scriptAreaList.push_back( scriptAreaWindowData ); 
		//
		if ( itScriptArea->second.bSelected )
		{
			dialogData.selectedScriptAreaIDList.push_back( itScriptArea->second.nScriptAreaID );
		}
	}
	//
	dialogData.eChangeMask = eChangeMask;
	uintptr_t dwData = reinterpret_cast<uintptr_t>( &dialogData );
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_AREA_WINDOW, ID_WINDOW_SET_DIALOG_DATA, dwData );
	//
	return bRes;
}


void CScriptAreaState::ClearSelection()
{
	// сбросить selection для областей
	for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
	{
		itScriptArea->second.bSelected = false;
	}
}


bool CScriptAreaState::ProcessScriptAreaWindowData()
{
	uintptr_t dwData = reinterpret_cast<uintptr_t>( &dialogData );
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCRIPT_AREA_WINDOW, ID_WINDOW_GET_DIALOG_DATA, dwData );
	if ( !bRes )
	{
		return false;
	}

	// удаление
	if ( dialogData.eChangeMask & SScriptAreaWindowData::CHANGE_DEL_SEL )
	{
		if ( !dialogData.selectedScriptAreaIDList.empty() )
		{
			for ( int nScriptAreIndex = 0; nScriptAreIndex < dialogData.selectedScriptAreaIDList.size(); ++nScriptAreIndex )
			{
				RemoveScriptArea( dialogData.selectedScriptAreaIDList[nScriptAreIndex] );
			}
			//
			ClearSelection();
			dialogData.selectedScriptAreaIDList.clear();
		}
		// обновить окно
		SetScriptAreaWindowData( SScriptAreaWindowData::CHANGE_SET_ALL );
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	//
	// кто поселекчен
	if ( dialogData.eChangeMask & SScriptAreaWindowData::CHANGE_SELECTION )
	{
		ClearSelection();
		if ( !dialogData.selectedScriptAreaIDList.empty() )
		{
			for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
			{
				for ( int nScriptAreIndex = 0; nScriptAreIndex < dialogData.selectedScriptAreaIDList.size(); ++nScriptAreIndex )
				{
					if ( dialogData.selectedScriptAreaIDList[nScriptAreIndex] == itScriptArea->second.nScriptAreaID )
					{
						itScriptArea->second.bSelected = true;
						CVec3 vAnchor = CVec3( itScriptArea->second.GetCenter(), 0.0f );
						AI2Vis( &vAnchor );
						Camera()->SetAnchor( vAnchor );
						break;
					}
				}
			}		
		}
	}
	// обновить список областей в окне редактора (
	SetScriptAreaWindowData( SScriptAreaWindowData::EChangeMask(SScriptAreaWindowData::CHANGE_AREAS | SScriptAreaWindowData::CHANGE_SELECTION) );
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	//
	return bRes;
}


bool CScriptAreaState::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	if ( !CPolygonState::HandleCommand( nCommandID, dwData ) )
	{
		switch( nCommandID ) 
		{
		case ID_SCRIPT_AREA_WINDOW_UI_EVENT:
			{
				ProcessScriptAreaWindowData();
				return true;
				break;
			}
		}
		return false;
	}
	return true;
}


bool CScriptAreaState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CScriptAreaState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CScriptAreaState::UpdateCommand(), pbCheck == 0" );
	//
	if ( !CPolygonState::UpdateCommand( nCommandID, pbEnable, pbCheck ) )
	{
		switch( nCommandID ) 
		{
		case ID_SCRIPT_AREA_WINDOW_UI_EVENT:
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


void CScriptAreaState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( nFlags & MK_SHIFT )
	{
		// с шифтом можно поселектить несколько областей
		bShift = true;
	}
	else
	{
		bShift = false;
	}
	CPolygonState::OnLButtonDown( nFlags, rMousePoint );
}


CScriptAreaState::SScriptArea::SScriptArea() : eType( NDb::EAT_CIRCLE ), nScriptAreaID( -1 ), bSelected( false )
{
}

void CScriptAreaState::SScriptArea::Draw( CPaintDC *pPaintDC, CSceneDrawTool *pSceneDrawTool ) const
{
	const int N_NUM_CIRCLE_PARTS = 20;
	uint32_t clr = bSelected ? CLR_SELECTED_AREA : CLR_NORMAL_AREA;
	//
	switch ( eType )
	{
		case NDb::EAT_CIRCLE:
		{
			CVec3 vCenter = CVec3( GetCenter(), 0.0f );
			UpdateTerrainHeight( &vCenter );
			pSceneDrawTool->DrawCircle( vCenter, GetRadius(), N_NUM_CIRCLE_PARTS, clr, false );
			break;
		}
		//
		case NDb::EAT_RECTANGLE:
		{
			std::list<CVec3> points;
			//
			float fX = GetAABBHalfSize().x;
			float fY = GetAABBHalfSize().y;
			//
			const CVec3 vCenter = CVec3( GetCenter(), 0.0f );
			{
				CVec3 p0 = vCenter + CVec3( fX, fY, 0.0f );
				UpdateTerrainHeight( &p0 );
				points.push_back( p0 );
			}
			{
				CVec3 p1 = vCenter + CVec3( fX, -fY, 0.0f );
				UpdateTerrainHeight( &p1 );
				points.push_back( p1 );
			}
			{
				CVec3 p2 = vCenter + CVec3( -fX, -fY, 0.0f );
				UpdateTerrainHeight( &p2 );
				points.push_back( p2 );
			}
			{
				CVec3 p3 = vCenter + CVec3( -fX, fY, 0.0f );
				UpdateTerrainHeight( &p3 );
				points.push_back( p3 );
			}
			pSceneDrawTool->DrawPolyline( points, clr, true, false );
			break;
		}
	}
}


bool CScriptAreaState::SScriptArea::Pick( const CVec3 &rPoint ) const
{
	switch ( eType )
	{
		case NDb::EAT_CIRCLE:
		{
			float fDistance = fabs( CVec2( rPoint.x, rPoint.y ) - GetCenter() );
			if ( fDistance <= GetRadius() )
			{
				return true;
			}
			break;
		}
		//
		case NDb::EAT_RECTANGLE:
		{
			std::list<CVec2> pointList;
			//
			float fHalfA = fabs( GetAABBHalfSize().x );
			float fHalfB = fabs( GetAABBHalfSize().y );
			CVec2 vCenter = GetCenter();
			//
			pointList.push_back( vCenter + CVec2( +fHalfA, +fHalfB ) );
			pointList.push_back( vCenter + CVec2( -fHalfA, +fHalfB ) );
			pointList.push_back( vCenter + CVec2( -fHalfA, -fHalfB ) );
			pointList.push_back( vCenter + CVec2( +fHalfA, -fHalfB ) );
			//
			if ( CP_INSIDE == ClassifyConvexPolygon( pointList, CVec2( rPoint.x, rPoint.y ) ) )
			{
				return true;
			}
			break;
		}
	}
	//
	return false;
}


CVec2 CScriptAreaState::SScriptArea::GetAABBHalfSize() const
{
	if ( cpList.size() != 2 )
	{
		NI_ASSERT( 0, "CScriptAreaState::SScriptArea{}: cpList.size() != 2" );
		return VNULL2;
	}
	//
	CVec3 res = cpList.back() - cpList.front();
	return CVec2( res.x, res.y );
}


CVec2 CScriptAreaState::SScriptArea::GetCenter() const
{
	if ( cpList.size() != 2 )
	{
		NI_ASSERT( 0, "CScriptAreaState::SScriptArea{}: cpList.size() != 2" );
		return VNULL2;
	}
	//
	CVec3 res = cpList.front();
	return CVec2( res.x, res.y );
}


float CScriptAreaState::SScriptArea::GetRadius() const
{
	if ( cpList.size() != 2 )
	{
		NI_ASSERT( 0, "CScriptAreaState::SScriptArea{}: cpList.size() != 2" );
		return 0;
	}
	//
	if ( eType != NDb::EAT_CIRCLE )
	{
		NI_ASSERT( 0, "CScriptAreaState::SScriptArea::GetRadius(): eType != NDb::EAT_CIRCLE" );
		return 0;
	}
	//
	float fR = fabs( cpList.back() - cpList.front() );
	return fR;
}


void CScriptAreaState::PostDraw( CPaintDC *pPaintDC )
{
	ICamera *pCam = Camera();
	CPtr<IEditorScene> pScene = EditorScene();
  if ( !pCam )
		return;
	//
	for ( CScriptAreaMap::iterator itScriptArea = scriptAreaMap.begin(); itScriptArea != scriptAreaMap.end(); ++itScriptArea )
	{
		CVec3 pos = CVec3( itScriptArea->second.GetCenter().x, itScriptArea->second.GetCenter().y, 0 ); 	
		pos.z = GetTerrainHeight( pos.x, pos.y );
		
		CTransformStack ts = pCam->GetTransform();
		
		CVec2 res = VNULL2;
		
		AI2Vis( &pos, pos );

		if ( TestRayInFrustrum( pos, ts, pScene->GetScreenRect(), &res ) )
		{
			NDrawToolsDC::DrawLabelDC( pPaintDC, itScriptArea->second.szName, res );			
		}
	}
}


