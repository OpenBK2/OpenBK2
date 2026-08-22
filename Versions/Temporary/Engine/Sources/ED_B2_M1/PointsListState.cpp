#include "stdafx.h"

#include "Misc/2Darray.h"
#include "Misc/PlaneGeometry.h"
#include "SceneB2/VisObjDesc.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "SeasonMnemonics.h"
#include "PointListDialog.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "Main/GameTimer.h"

#include "PointsListState.h"

#include <cstdint>

#include <zconf.h>

//
//
//		POINTS LIST STATE
//
//

CPointsListState::CPointsListState( unsigned _nCHID, unsigned _nInstanceID, CBuildingEditor* _pBuildingEditor )
	:	pBuildingEditor( _pBuildingEditor ),
		pMaskManipulator( 0 ),
		nCHID( _nCHID ),
		nInstanceID( _nInstanceID ),
		pMarkers( 0 )
{
	NI_ASSERT( pBuildingEditor != 0, "CPointsListState(): Invalid parameter: pBuildingEditor == 0" );
	Singleton<ICommandHandlerContainer>()->Set( nCHID, this );
}


CPointsListState::~CPointsListState()
{
	Singleton<ICommandHandlerContainer>()->Remove( nCHID );
}


void CPointsListState::Enter()
{
	ClearMaskManipulator();
	drawTool.Clear();
	pMarkers = 0;

	currDialogData.nInstanceID = nInstanceID;
	currDialogData.nNumPoints = GetPointsNum( GetPointsArrayFieldName() ); 
	currDialogData.nSelectedPoint = 0;
	currDialogData.bChkPassability = false;
	currDialogData.bChkPropmask = true;
	//
	string szCurSeason = pBuildingEditor->GetCurrSeason();
	currDialogData.eSeason = NDb::ESeason( typeSeasonMnemonics.GetValue(szCurSeason) );

	SetPointListDialogData( &currDialogData );

	Singleton<ICommandHandlerContainer>()->Set( CHID_POINTS_LIST_DLG_LISTENER, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CPointsListState::Leave()
{
	pMarkers = 0;
	ClearMaskManipulator();

	currDialogData.nNumPoints = 0;

	Singleton<ICommandHandlerContainer>()->Remove( CHID_POINTS_LIST_DLG_LISTENER );

	drawTool.Clear();
	pMarkers = 0;
}


void CPointsListState::SetPointMarkers()
{ 
	IEditorScene *pScene = EditorScene();
	if ( !pScene )
		return;

	CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->
		CreateObjectManipulator( pBuildingEditor->GetObjectSet().szObjectTypeName, 
														 pBuildingEditor->GetObjectSet().objectNameSet.begin()->first );

	CVec2 vBuildingOrigin = VNULL2;
	CManipulatorManager::GetValue( &vBuildingOrigin, pManipulator, "Origin" );
	pBuildingEditor->SetBuildingOrigin( vBuildingOrigin );

	int nActiveIndex = GetSelectedPointIndex();

	string szScrText = GetPointsArrayFieldName() + StrFmt( " [Variant ID = %d]", nActiveIndex );
	pBuildingEditor->SetScreenTitle( szScrText );

	pMarkers = new SMarkerSet( pBuildingEditor->GetBuildingPos(), vBuildingOrigin );

	CVariant vNumElems;
	if ( pManipulator->GetValue( GetPointsArrayFieldName(), &vNumElems ) )
	{
		int nNumElems = vNumElems;
		for ( int i = 0; i < nNumElems; ++i )
		{
			CVec3 pos = VNULL3;
			CManipulatorManager::GetValue( &pos, pManipulator, StrFmt("%s.[%d].%s", GetPointsArrayFieldName().c_str(), i, GetPositionFieldName().c_str()) ); 

			CVariant dir = 0.0f;
			if ( !GetDirectionFieldName().empty() )
				pManipulator->GetValue( StrFmt("%s.[%d].%s", GetPointsArrayFieldName().c_str(), i, GetDirectionFieldName().c_str()), &dir );

			bool bMarkerIsActive		= (i == nActiveIndex);
			bool bOriginInUse				= IsOriginInUse();
			float fDirection				= static_cast<float>(dir);
			EDirMeasure eDirMeasure	= GetDirMeasure();
						
			pMarkers->AddMarker( pos, fDirection, eDirMeasure, bMarkerIsActive, true, bOriginInUse );

			AddPointSpecificMarker( pManipulator, i );
		}
	}

	pMarkers->AttachToScene();	
}


int CPointsListState::GetPointsNum( const string &rszField )
{
	const SObjectSet &objSet = pBuildingEditor->GetObjectSet();
	if ( objSet.objectNameSet.empty() )
		return 0;

	CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( objSet.szObjectTypeName, objSet.objectNameSet.begin()->first );

	NI_ASSERT( pManipulator, "CPointsListState::GetPointsNum  pManipulator==0" );

	CVariant vNumElems;
	if ( pManipulator->GetValue(rszField, &vNumElems) )
	{
		int nNumElems = vNumElems;
		return nNumElems;
	}

	return 0;
}


void CPointsListState::ClearMaskManipulator()
{
	if ( pMaskManipulator != 0 )
	{
		IView *pView = 0;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																													ID_PC_DIALOG_GET_VIEW, 
																													reinterpret_cast<uint32_t>(&pView) );
		if ( pView != 0 )
		{
			pView->RemoveViewManipulator();
			pView->SetViewManipulator( pBuildingEditor->CreateBuildingManipulator(), pBuildingEditor->GetObjectSet(), string() );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																													ID_PC_DIALOG_CREATE_TREE, 0 );
		pMaskManipulator = 0;	
	}
}


int CPointsListState::GetSelectedPointIndex()
{
	SPointListDialogData data;
	data.nInstanceID = nInstanceID;

	if ( GetPointListDialogData(&data) )
		return data.nSelectedPoint;

	return -1;
}


void CPointsListState::SetMaskManipulator()
{
	if ( pMaskManipulator != 0 )
		ClearMaskManipulator();

	IView *pView = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<uint32_t>(&pView) );
	if ( pView != 0 )
		pView->RemoveViewManipulator();

	pMaskManipulator = 0;	

	const string szMask = StrFmt( "%s.[%d].", GetPointsArrayFieldName().c_str(), GetSelectedPointIndex() );
	
	pMaskManipulator = new CMaskManipulator( szMask, pBuildingEditor->GetViewManipulator(), CMaskManipulator::SMART_MODE );

	vector<string> maskFields;
	GetMaskFields( &maskFields );

	for ( vector<string>::iterator it = maskFields.begin(); it < maskFields.end(); ++it )
	{
		string szM = (*it);
		pMaskManipulator->AddName( szM, false, "", INVALID_NODE_ID, false );
	}
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<uint32_t>(&pView) );
	bool bNeedCreateTree = true;
	if ( pView != 0 )
	{
		bNeedCreateTree = ( pView->GetViewManipulator() != pMaskManipulator );
		pView->SetViewManipulator( pMaskManipulator, pBuildingEditor->GetObjectSet(), string() );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												bNeedCreateTree ? ID_PC_DIALOG_CREATE_TREE : ID_PC_DIALOG_UPDATE_VALUES, 0 );

	ICommandHandler *pCommandHandler = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_COMMAND_HANDLER, 
																												reinterpret_cast<uint32_t>(&pCommandHandler) );
	if ( pCommandHandler != 0 )
		pCommandHandler->HandleCommand( ID_PC_EXPAND_ALL, 0 );
}


bool CPointsListState::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID ) 
	{
	case ID_POINTS_LIST_DLG_CHANGE_STATE:
		{
			int nPointsListDlgInstanceID = dwData;
			if ( nPointsListDlgInstanceID == nInstanceID )
				RefreshState();							

			return true;
		}
		break;
	}
	return false;
}


bool CPointsListState::GetPointListDialogData( SPointListDialogData *pData )
{
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_POINTS_LIST_DIALOG, 
																																		ID_WINDOW_GET_DIALOG_DATA, 
																																		reinterpret_cast<uint32_t>(pData) );
	return bRes;
}


bool CPointsListState::SetPointListDialogData( SPointListDialogData *pData )
{
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_POINTS_LIST_DIALOG, 
																																		ID_WINDOW_SET_DIALOG_DATA, 
																																		reinterpret_cast<uint32_t>(pData) );
	return bRes;
}


void CPointsListState::RefreshState()
{
	SPointListDialogData data;
	data.nInstanceID = nInstanceID;

	if ( GetPointListDialogData(&data) )
	{
		if ( data.nInstanceID != nInstanceID )
		{
			NI_ASSERT( 0, "data.nInstanceID != nInstanceID" );
			return;
		}
		if ( currDialogData.eSeason != data.eSeason )
			pBuildingEditor->ChangeSeason( data.eSeason );

		if ( currDialogData.nSelectedPoint != data.nSelectedPoint )
		{
			if ( data.bChkPropmask )
				SetMaskManipulator();
			else
				ClearMaskManipulator();
		}

		if ( currDialogData.bChkPropmask != data.bChkPropmask )
		{
			if ( data.bChkPropmask )
				SetMaskManipulator();
			else
				ClearMaskManipulator();
		}

		pBuildingEditor->SetDrawPassability( data.bChkPassability );
		currDialogData = data;
	}
	SetPointMarkers();

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


bool CPointsListState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CPointsListState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CPointsListState::UpdateCommand(), pbCheck == 0" );

	return true;
}


void CPointsListState::PostDraw( CPaintDC *pPaintDC )
{
	if ( !IsValid(pProcess) || !pProcess->Update(GameTimer()->GetAbsTime()) )
		pProcess = 0;

	pBuildingEditor->DrawPassability( pPaintDC );
}


void CPointsListState::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( nChar == 'G' )
	{
		CPtr<IEditorScene> pScene = EditorScene();
		if ( pScene )
			pScene->ToggleShow( SCENE_SHOW_AI_GRID );
	}
}


//
//
//		SMOKE POINTS STATE
//
//

void CSmokePointsState::GetMaskFields(vector<string> *pMaskFields )
{
	pMaskFields->clear();
	//
	pMaskFields->push_back( "Pos.x" );
	pMaskFields->push_back( "Pos.y" );
	pMaskFields->push_back( "Pos.z" );
	pMaskFields->push_back( "Direction" );
	pMaskFields->push_back( "VerticalAngle" );		
	pMaskFields->push_back( "FireEffect" );		
	pMaskFields->push_back( "Coverage" );		
	pMaskFields->push_back( "PicturePosition.x" );		
	pMaskFields->push_back( "PicturePosition.y" );		
}


//
//
//		SLOT POINTS STATE
//
//

void CSlotPointsState::GetMaskFields(vector<string> *pMaskFields )
{
	pMaskFields->clear();
	//
	pMaskFields->push_back( "LocatorName" );
	pMaskFields->push_back( "NumFirePlaces" );
	pMaskFields->push_back( "Pos.x" );		
	pMaskFields->push_back( "Pos.y" );		
	pMaskFields->push_back( "Pos.z" );		
	pMaskFields->push_back( "Direction" );		
	pMaskFields->push_back( "Angle" );		
	pMaskFields->push_back( "SightMultiplier" );		
	pMaskFields->push_back( "Coverage" );
	pMaskFields->push_back( "Gun.Weapon" );
	pMaskFields->push_back( "Gun.Priority" );
	pMaskFields->push_back( "Gun.IsPrimary" );
	pMaskFields->push_back( "Gun.Ammo" );
	pMaskFields->push_back( "Gun.Direction" );
	pMaskFields->push_back( "Gun.ReloadCost" );
	pMaskFields->push_back( "RotationSpeed" );
	pMaskFields->push_back( "Window.DayObj" );
	pMaskFields->push_back( "Window.NightObj" );
	pMaskFields->push_back( "Window.DestroyedObj" );
	pMaskFields->push_back( "Window.DestroyEffect" );
}


//
//
//		ENTRANCE POINTS STATE
//
//

void CEntrancePointsState::GetMaskFields(vector<string> *pMaskFields )
{
	pMaskFields->clear();
	//
	pMaskFields->push_back( "Pos.x" );
	pMaskFields->push_back( "Pos.y" );
	pMaskFields->push_back( "Pos.z" );
	pMaskFields->push_back( "Stormable" );
	pMaskFields->push_back( "Dir" );
}


//
//
//		SURFACE POINTS STATE
//
//

void CSurfacePointsState::GetMaskFields(vector<string> *pMaskFields )
{
	pMaskFields->clear();
	//
	pMaskFields->push_back( "Pos.x" );
	pMaskFields->push_back( "Pos.y" );
	pMaskFields->push_back( "Pos.z" );
	pMaskFields->push_back( "Orient.x" );
	pMaskFields->push_back( "Orient.y" );
	pMaskFields->push_back( "Orient.z" );
	pMaskFields->push_back( "Orient.w" );
}


void CSurfacePointsState::Draw( CPaintDC *pPaintDC )
{
	for ( vector<NDb::SHPObjectRPGStats::SModelSurfacePoint>::iterator it = surfPoints.begin(); it < surfPoints.end(); ++it )
	{
		NDb::SHPObjectRPGStats::SModelSurfacePoint &point = *it;

		CVec3 vPos = point.vPos;
		CVec4 vOrient = point.vOrient;

		CQuat qRot( 0.0f, V3_AXIS_Z );
		CQuat qNorm( vOrient );

		CVec3 vNorm( 0, 0, 1.0f );
		qRot.Rotate( &vNorm, vNorm );
		qNorm.Rotate( &vNorm, vNorm );

		vPos += pBuildingEditor->GetBuildingPos();
		drawTool.DrawLine( vPos, vPos + (120.0f * vNorm), 0xFF123456, false );
		drawTool.DrawCircle( vPos + (120.0f * vNorm), 10.0f, 5, 0xFF123400, false );
	}
  drawTool.Draw();
}


void CSurfacePointsState::AddPointSpecificMarker( IManipulator *pManipulator, int nPointIndex )
{
	if ( nPointIndex == 0 )
		surfPoints.clear();
	//
	NDb::SHPObjectRPGStats::SModelSurfacePoint point;
	point.vPos = VNULL3;
	point.vOrient = VNULL4;

	CManipulatorManager::GetValue( &point.vPos, pManipulator, StrFmt("SurfacePoints.[%d].Pos", nPointIndex) );
	CManipulatorManager::GetValue( &point.vOrient, pManipulator, StrFmt("SurfacePoints.[%d].Orient", nPointIndex) );

	surfPoints.push_back( point );
}


//
//
//		DAMAGE LEVELS STATE
//
//

void CDamageLevelsState::GetMaskFields(vector<string> *pMaskFields )
{
	pMaskFields->clear();
	//
	pMaskFields->push_back( "DamageHP" );
	pMaskFields->push_back( "VisObj" );
	pMaskFields->push_back( "DamageEffectWindow" );
	pMaskFields->push_back( "DamageEffectSmoke" );
}


void CDamageLevelsState::RefreshState()
{
	CPointsListState::RefreshState();
	//
	CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->
		CreateObjectManipulator( pBuildingEditor->GetObjectSet().szObjectTypeName, 
														 pBuildingEditor->GetObjectSet().objectNameSet.begin()->first );

	CMOBuilding *pBuilding = pBuildingEditor->GetBuildingPtr();
	//
  int nActiveIndex = GetSelectedPointIndex();
	SAINotifyRPGStats pNewStats;
	pNewStats.fHitPoints = 1.0;
	pNewStats.fFuel = 0.0f;
	pNewStats.time = GameTimer()->GetAbsTime();
	if ( nActiveIndex > 0 ) // ( != -1) && ( != 0)
		CManipulatorManager::GetValue( &pNewStats.fHitPoints, pManipulator, StrFmt("%s.[%d].DamageHP", GetPointsArrayFieldName().c_str(), nActiveIndex-1) );

	pNewStats.fHitPoints *= static_cast<CMapObj*>(pBuilding)->GetStats()->fMaxHP;
	pProcess = pBuilding->AIUpdateRPGStats( pNewStats, 0, static_cast<NDb::ESeason>(typeSeasonMnemonics.GetValue(pBuildingEditor->GetCurrSeason())) );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
}


void CDamageLevelsState::Leave()
{
	CMOBuilding *pBuilding = pBuildingEditor->GetBuildingPtr();

	SAINotifyRPGStats pNewStats;
	pNewStats.fFuel = 0.0f;
	pNewStats.time = GameTimer()->GetAbsTime();
	pNewStats.fHitPoints = static_cast<CMapObj*>(pBuilding)->GetStats()->fMaxHP;
	pProcess = pBuilding->AIUpdateRPGStats( pNewStats, 0, static_cast<NDb::ESeason>(typeSeasonMnemonics.GetValue(pBuildingEditor->GetCurrSeason())) );

	CPointsListState::Leave();
}


