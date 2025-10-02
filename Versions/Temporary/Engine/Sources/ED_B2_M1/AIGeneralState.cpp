#include "stdafx.h"

#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "../stats_b2_m1/iconsset.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "aigeneraltypes.h"
#include "../sceneb2/scene.h"
#include "aigenparceldlg.h"
#include "aigenmobileiddlg.h"
#include "AIGeneralState.h"
#include "../MapEditorLib/Interface_MainFrame.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static DWORD CalcColor( NDb::EParcelType eType )
{
	switch ( eType )
	{
		case NDb::EPATCH_DEFENCE:
		{
			return PARCEL_COLOR_DEFENCE;
			break;
		}
		//
		case NDb::EPATCH_REINFORCE:
		{
			return PARCEL_COLOR_REINFORCE;
			break;
		}
	}
	//
	return PARCEL_COLOR_UNKNOWN;
}


//
//
//		AI GENERAL POINTS STATE
//
//

CAIGeneralPointsState::CAIGeneralPointsState( CMapInfoEditor *_pMapInfoEditor )
	:	pMapInfoEditor( _pMapInfoEditor ),
	bMoveParcel( false ),
	bMovePoint( false ),
	bRotateParcel( false ),
	bRotatePoint( false )
{
	NI_ASSERT( pMapInfoEditor, "CAIGeneralPointsState::CAIGeneralPointsState(): pMapInfoEditor == 0" );
	dialogData.nCurrentPlayer = 0;
}


void CAIGeneralPointsState::Enter()
{
	sceneDrawTool.Clear();
	//
	GetDataFromDB();
	RefreshWindowData();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_AIGEN_POINTS_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_SCENE, ID_SCENE_GET_FOCUS, 0) )
	{
		Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
	}
}


void CAIGeneralPointsState::Leave()
{
	SaveDataToDB();
	sceneDrawTool.Clear();
	dialogData.Clear();
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_AIGEN_POINTS_STATE );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SELECTION, this );
}


void CAIGeneralPointsState::OnSetFocus( class CWnd* pNewWnd )
{
	CDefaultInputState::OnSetFocus( pNewWnd );
	Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CAIGeneralPointsState::Draw( CPaintDC *pPaintDC )
{
	sceneDrawTool.Clear();
	if ( dialogData.nCurrentPlayer == -1 )
		return;
	else if ( dialogData.players.size() < 1 )
		return;
	else if ( dialogData.players[dialogData.nCurrentPlayer].parcels.size() < 1 )
		return;

	//GetDataFromDB();
	//RefreshWindowData();

	const SAIGeneralPointsWindowData::SAIPlayerInfo &currentPlayer = dialogData.players[dialogData.nCurrentPlayer];
	int nParcel = 0;
	for ( vector<SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel>::const_iterator itParcel = currentPlayer.parcels.begin(); itParcel < currentPlayer.parcels.end(); ++itParcel, ++nParcel )
	{
		const SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &currentParcel = (*itParcel);
		//
		const DWORD dwParcelColor = CalcColor( currentParcel.eType );

		CVec3 vPos( currentParcel.vCenter, 0 );
		vPos.z = GetTerrainHeight( vPos.x, vPos.y );
		float fDir = AI2VisRad( currentParcel.fDefenceDirection );
		fDir += FP_PI2;
		if ( fDir >= FP_2PI )
			fDir -= FP_2PI;
		const CVec3 vDirPos = CreateFromPolarCoord( currentParcel.fRadius, fDir, 0 ) + vPos;

		sceneDrawTool.DrawCircle( vPos, currentParcel.fRadius+SELECTION_POINT_RADIUS/2, PARCEL_PARTS, dwParcelColor, false );
		sceneDrawTool.DrawCircle( vPos, currentParcel.fRadius-SELECTION_POINT_RADIUS/2, PARCEL_PARTS, dwParcelColor, false );
		sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS, PARCEL_PARTS, dwParcelColor, false );
		sceneDrawTool.DrawLine( vPos, vDirPos, dwParcelColor, false );
		sceneDrawTool.DrawCircle( vDirPos, SELECTION_POINT_RADIUS, PARCEL_PARTS, dwParcelColor, false );

		if ( nParcel == currentPlayer.nCurrentParcel )
		{
			{
				for ( int i = 0; i < 10; ++i )
				{
					sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS * (float)i / 10.0f, SELECTION_POINT_PARTS, dwParcelColor, false );
				}
			}
			if ( bRotateParcel )
			{
				for ( int i = 0; i < 10; ++i )
				{
					sceneDrawTool.DrawCircle( vDirPos, SELECTION_POINT_RADIUS * (float)i / 10.0f, SELECTION_POINT_PARTS, dwParcelColor, false );
				}
			}
		}
		//
		int nPoint = 0;
		for ( vector<NDb::SReinforcePoint>::const_iterator itReinfPoint = currentParcel.reinforcePoints.begin(); itReinfPoint < currentParcel.reinforcePoints.end(); ++itReinfPoint, ++nPoint )
		{
			const NDb::SReinforcePoint &currentReinfPoint = (*itReinfPoint);

			CVec3 vPos( currentParcel.vCenter + currentReinfPoint.vCenter, 0 );
			vPos.z = GetTerrainHeight(vPos.x, vPos.y );
			float fDir = AI2VisRad( currentReinfPoint.fDirection );
			fDir += FP_PI2;
			if ( fDir >= FP_2PI )
				fDir -= FP_2PI;
			const CVec3 vDirPos = CreateFromPolarCoord( PARCEL_REINFORCE_RAD, fDir, 0 ) + vPos;

			sceneDrawTool.DrawCircle( vPos, PARCEL_REINFORCE_RAD, PARCEL_POINT_PARTS, dwParcelColor, false );
			sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS, PARCEL_POINT_PARTS, dwParcelColor, false );
			sceneDrawTool.DrawLine( vPos, vDirPos, dwParcelColor, false );
			sceneDrawTool.DrawCircle( vDirPos, SELECTION_POINT_RADIUS, PARCEL_POINT_PARTS, dwParcelColor, false );

			if ( (nParcel == currentPlayer.nCurrentParcel) && (nPoint == currentParcel.nCurrentPoint) )
			{
				{
					for ( int i = 0; i < 10; ++i )
					{
						sceneDrawTool.DrawCircle( vPos, SELECTION_POINT_RADIUS * (float)i / 10.0f, SELECTION_POINT_PARTS, dwParcelColor, false );
					}
				}
				if ( bRotatePoint )
				{
					for ( int i = 0; i < 10; ++i )
					{
						sceneDrawTool.DrawCircle( vDirPos, SELECTION_POINT_RADIUS * (float)i / 10.0f, SELECTION_POINT_PARTS, dwParcelColor, false );
					}
				}
			}
		}
	}
	sceneDrawTool.Draw();
}


bool CAIGeneralPointsState::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID ) 
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			GetDataFromDB();
			RefreshWindowData();
			break;
		}
		case ID_AIGEN_POINTS_WINDOW_UI_EVENT:
		{
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_AIGEN_POINTS_WINDOW, 
																																ID_WINDOW_GET_DIALOG_DATA, 
																																reinterpret_cast<DWORD>(&dialogData)) )
			{
				switch ( dialogData.eLastAction )
				{
					case SAIGeneralPointsWindowData::AIGP_ID_ADD:
					{
						AddID();
						break;
					}
					//
					case SAIGeneralPointsWindowData::AIGP_ID_DEL:
					{
						DeleteID();
						break;
					}
					//
					case SAIGeneralPointsWindowData::AIGP_PARCEL_ADD:
					{
						AddParcel();
						break;
					}
					//
					case SAIGeneralPointsWindowData::AIGP_PARCEL_DEL:
					{
						DeleteParcel();
						break;
					}
					//
					case SAIGeneralPointsWindowData::AIGP_PARCEL_EDIT:
					{
						EditParcel();
						break;
					}
				}
				RefreshWindowData();
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
				//
				return true;
			}
			break;
		}
		//
		case ID_SELECTION_CLEAR:
		{
			DeletePoint();
			RefreshWindowData();
			return true;
		}
	}
	return false;
}


bool CAIGeneralPointsState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAIGeneralPointsState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAIGeneralPointsState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
		case ID_UPDATE_EDIT_PARAMETERS:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		case ID_AIGEN_POINTS_WINDOW_UI_EVENT:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		//
		case ID_SELECTION_CLEAR:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		default:
			return false;
	}
	//
	return false;
}


void CAIGeneralPointsState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( dialogData.players[dialogData.nCurrentPlayer].parcels.size() == 0 )
		return;

	CVec3 vPickedPosOnTerrain = VNULL3;
	Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
	vPickedPosOnTerrain.z = 0;

	SAIGeneralPointsWindowData::SAIPlayerInfo &currentPlayer = dialogData.players[dialogData.CurrentPlayer()];
	//
	int nParcel = 0;
	for ( vector<SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel>::iterator itParcel = currentPlayer.parcels.begin(); itParcel < currentPlayer.parcels.end(); ++itParcel, ++nParcel )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &currentParcel = (*itParcel);
		//
		if ( fabs(CVec3(currentParcel.vCenter, 0) - vPickedPosOnTerrain) < SELECTION_POINT_RADIUS )
		{
			currentPlayer.nCurrentParcel = nParcel;
			bMoveParcel = true;
			RefreshWindowData();
			return;
		}
		float fDir = AI2VisRad( currentParcel.fDefenceDirection );
		fDir += FP_PI2;
		if ( fDir >= FP_2PI )
			fDir -= FP_2PI;
		const CVec2 vDirRad = CreateFromPolarCoord( currentParcel.fRadius, fDir );
		if ( fabs(CVec3(currentParcel.vCenter + vDirRad, 0) - vPickedPosOnTerrain) < SELECTION_POINT_RADIUS )
		{
			currentPlayer.nCurrentParcel = nParcel;
			bRotateParcel = true;
			RefreshWindowData();
			return;
		}
	}
	nParcel = 0;
	for ( vector<SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel>::iterator itParcel = currentPlayer.parcels.begin(); itParcel < currentPlayer.parcels.end(); ++itParcel, ++nParcel )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &currentParcel = (*itParcel);
		//
		int nPoint = 0;
		for ( vector<NDb::SReinforcePoint>::iterator itReinfPoint = currentParcel.reinforcePoints.begin(); itReinfPoint < currentParcel.reinforcePoints.end(); ++itReinfPoint, ++nPoint )
		{
			NDb::SReinforcePoint &currentPoint = (*itReinfPoint);
			//
			if ( fabs(CVec3(currentParcel.vCenter + currentPoint.vCenter, 0) - vPickedPosOnTerrain) < SELECTION_POINT_RADIUS )
			{
				currentPlayer.nCurrentParcel = nParcel;
				currentParcel.nCurrentPoint = nPoint;
				bMovePoint = true;
				RefreshWindowData();
				return;
			}
			float fDir = AI2VisRad( currentPoint.fDirection );
			fDir += FP_PI2;
			if ( fDir >= FP_2PI )
				fDir -= FP_2PI;
			const CVec2 vDirRad = CreateFromPolarCoord( PARCEL_REINFORCE_RAD, fDir );
			if ( fabs(CVec3(currentParcel.vCenter + currentPoint.vCenter + vDirRad, 0) - vPickedPosOnTerrain) < SELECTION_POINT_RADIUS )
			{
				currentPlayer.nCurrentParcel = nParcel;
				currentParcel.nCurrentPoint = nPoint;
				bRotatePoint = true;
				RefreshWindowData();
				return;
			}
		}
	}

	dialogData.players[dialogData.nCurrentPlayer].nCurrentParcel = -1;
	RefreshWindowData();
}


void CAIGeneralPointsState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bMoveParcel || bMovePoint || bRotateParcel || bRotatePoint )
	{
		SaveDataToDB();

		bMoveParcel = false;
		bMovePoint = false;
		bRotateParcel = false;
		bRotatePoint = false;
		RefreshWindowData();
	}
}


void CAIGeneralPointsState::OnRButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	bMoveParcel = false;
	bMovePoint = false;
	bRotateParcel = false;
	bRotatePoint = false;

	AddPoint( rMousePoint );
	
	RefreshWindowData();
}


void CAIGeneralPointsState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( !(bMoveParcel || bMovePoint || bRotateParcel || bRotatePoint) )
		return;

	CVec3 vPickedPosOnTerrain = VNULL3;
	Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
	CVec2 vPickPos( vPickedPosOnTerrain.x, vPickedPosOnTerrain.y );

	SAIGeneralPointsWindowData::SAIPlayerInfo &currentPlayer = dialogData.players[dialogData.CurrentPlayer()];
	SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &currentParcel = currentPlayer.parcels[dialogData.CurrentParcel()];
	
	if ( bMoveParcel )
	{
		currentParcel.vCenter = vPickPos;
	}
	else if ( bMovePoint )
	{
		currentParcel.reinforcePoints[dialogData.CurrentPoint()].vCenter = vPickPos - currentParcel.vCenter;
	}
	else if ( bRotateParcel )
	{
		float fOldParcelDir = AI2VisRad( currentParcel.fDefenceDirection );
		const float fDirOff = GetPolarAngle( vPickPos - currentParcel.vCenter ) - FP_PI2 - fOldParcelDir;

		for ( vector<NDb::SReinforcePoint>::iterator itReinfPoint = currentParcel.reinforcePoints.begin(); itReinfPoint < currentParcel.reinforcePoints.end(); ++itReinfPoint )
		{
			const float fOldPointDir = AI2VisRad( itReinfPoint->fDirection );

			float fNewPointDir = GetPolarAngle( itReinfPoint->vCenter ) + fDirOff;
			if ( fNewPointDir < 0 )
				fNewPointDir += FP_2PI;
			else if ( fNewPointDir >= FP_2PI )
				fNewPointDir -= FP_2PI;
			itReinfPoint->vCenter = CreateFromPolarCoord( fabs(itReinfPoint->vCenter), fNewPointDir );

			float fNewDir = fOldPointDir + fDirOff;
			if ( fNewDir < 0 )
				fNewDir += FP_2PI;
			else if ( fNewDir >= FP_2PI )
				fNewDir -= FP_2PI;
			itReinfPoint->fDirection = Vis2AIRad( fNewDir );
		}
		float fNewParcelDir = fOldParcelDir + fDirOff;
		if ( fNewParcelDir < 0 )
			fNewParcelDir += FP_2PI;
		else if ( fNewParcelDir >= FP_2PI )
			fNewParcelDir -= FP_2PI;
		currentParcel.fDefenceDirection = Vis2AIRad( fNewParcelDir );

		currentParcel.fRadius = fabs( vPickPos - currentParcel.vCenter );
		if ( currentParcel.fRadius < PARCEL_REINFORCE_RAD )
			currentParcel.fRadius = PARCEL_REINFORCE_RAD;
	}
	else if ( bRotatePoint )
	{
		float fNewDir = GetPolarAngle( vPickPos - (currentParcel.vCenter + currentParcel.reinforcePoints[dialogData.CurrentPoint()].vCenter) ) - FP_PI2;
		if ( fNewDir < 0 )
			fNewDir += FP_2PI;
		else if ( fNewDir >= FP_2PI )
			fNewDir -= FP_2PI;
		currentParcel.reinforcePoints[dialogData.CurrentPoint()].fDirection = Vis2AIRad( fNewDir );
	}
}


void CAIGeneralPointsState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch ( nChar )
	{
		case VK_SPACE:
		{
			if ( (dialogData.CurrentPlayer() < 0) || (dialogData.CurrentParcel() < 0) )
				return;

			SAIGeneralPointsWindowData::SAIPlayerInfo &currentPlayer = dialogData.players[dialogData.CurrentPlayer()];
			SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &currentParcel = currentPlayer.parcels[dialogData.CurrentParcel()];

			for ( vector<NDb::SReinforcePoint>::iterator itReinfPoint = currentParcel.reinforcePoints.begin(); itReinfPoint < currentParcel.reinforcePoints.end(); ++itReinfPoint )
			{
				itReinfPoint->fDirection = currentParcel.fDefenceDirection;
			}
			RefreshWindowData();
			break;
		}
	}
}


void CAIGeneralPointsState::GetDataFromDB()
{
	dialogData.Clear();

	for ( vector<NDb::SMapPlayerInfo>::const_iterator itPlayer = GetMapInfoEditor()->pMapInfo->players.begin();
																										itPlayer != GetMapInfoEditor()->pMapInfo->players.end(); ++itPlayer )
	{
		const NDb::SMapPlayerInfo &player = (*itPlayer);

		SAIGeneralPointsWindowData::SAIPlayerInfo newPlayer;

		for ( vector<NDb::SAIGeneralParcel>::const_iterator itParcel = player.general.parcels.begin();
																												itParcel != player.general.parcels.end(); ++itParcel )
		{
			const NDb::SAIGeneralParcel &parcel = (*itParcel);

			SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel newParcel;
			newParcel.eType = parcel.eType;
			newParcel.vCenter = parcel.vCenter;
			newParcel.fRadius = parcel.fRadius;
			newParcel.fDefenceDirection = parcel.fDefenceDirection;
			newParcel.fImportance = parcel.fImportance;
			newParcel.reinforcePoints = parcel.reinforcePoints;

			newPlayer.parcels.push_back( newParcel );
		}

		newPlayer.mobileScriptIDs = player.general.mobileScriptIDs;

		dialogData.players.push_back( newPlayer );
	}
	if( dialogData.nCurrentPlayer >= GetMapInfoEditor()->pMapInfo->players.size() )
	{
		dialogData.nCurrentPlayer = 0;
	}
}


void CAIGeneralPointsState::SaveDataToDB()
{
	if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
		{
			bool bResult = true;

			for ( int nPlayer = 0; nPlayer < dialogData.players.size(); ++nPlayer )
			{
				const SAIGeneralPointsWindowData::SAIPlayerInfo &player = dialogData.players[nPlayer];
				for ( int nParcel = 0; nParcel < player.parcels.size(); ++nParcel )
				{
					const SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &parcel = player.parcels[nParcel];
					const string szType = typeAIGeneralParcel.GetMnemonic((int)dialogData.players[nPlayer].parcels[nParcel].eType).c_str();

					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Type", nPlayer, nParcel), string(szType), pManipulator );
					bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( StrFmt("Players.[%d].general.parcels.[%d].Center", nPlayer, nParcel), CVec2(parcel.vCenter.x, parcel.vCenter.y), pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Radius", nPlayer, nParcel), float(parcel.fRadius) , pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].DefenceDirection", nPlayer, nParcel), float(parcel.fDefenceDirection) , pManipulator );
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Importance", nPlayer, nParcel), float(parcel.fImportance) , pManipulator );

					for ( int nReinforcePoint = 0; nReinforcePoint < parcel.reinforcePoints.size(); ++nReinforcePoint )
					{
						const NDb::SReinforcePoint &reinfPoint = parcel.reinforcePoints[nReinforcePoint];

						bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints.[%d].Center", nPlayer, nParcel, nReinforcePoint), CVec2(reinfPoint.vCenter.x, reinfPoint.vCenter.y), pManipulator );
						bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints.[%d].Direction", nPlayer, nParcel, nReinforcePoint), float(reinfPoint.fDirection), pManipulator );
					}
				}
				for ( int nScriptID = 0; nScriptID < dialogData.players[nPlayer].mobileScriptIDs.size(); ++nScriptID )
				{
					bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.mobileScriptIDs.[%d]", nPlayer, nScriptID), int(player.mobileScriptIDs[nScriptID]), pManipulator );
				}
			}
			if ( bResult )
			{
				pObjectController->Redo( false, true, GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}


void CAIGeneralPointsState::RefreshWindowData()
{
	if ( dialogData.nCurrentPlayer == -1 )
		return;

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_AIGEN_POINTS_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<DWORD>(&dialogData) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CAIGeneralPointsState::AddID()
{
	if ( dialogData.CurrentPlayer() < 0 )
		return;

	int nNewMobileID = 0;
	CAIGenMobileDlg dlg( Singleton<IMainFrameContainer>()->GetSECWorkbook(), &nNewMobileID );
	if ( dlg.DoModal() == IDOK )
	{
		dialogData.players[dialogData.CurrentPlayer()].mobileScriptIDs.push_back( nNewMobileID );

		if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
			{
				if ( pObjectController->AddInsertOperation(StrFmt("Players.[%d].general.mobileScriptIDs", dialogData.CurrentPlayer()), NODE_ADD_INDEX, pManipulator) )
				{
					bool bResult = pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.mobileScriptIDs.[%d]", dialogData.CurrentPlayer(), dialogData.players[dialogData.CurrentPlayer()].mobileScriptIDs.size() - 1),
																																dialogData.players[dialogData.CurrentPlayer()].mobileScriptIDs[dialogData.players[dialogData.CurrentPlayer()].mobileScriptIDs.size() - 1], pManipulator );

					if ( bResult )
					{
						pObjectController->Redo( false, true, GetMapInfoEditor() );
						Singleton<IControllerContainer>()->Add( pObjectController );
					}
				}
			}
		}
	}
}


void CAIGeneralPointsState::AddParcel()
{
	if ( dialogData.CurrentPlayer() < 0 )
		return;

	CVec3 vCamAnchor;
	GetCameraPosition( &vCamAnchor );
	Vis2AI( &vCamAnchor );

	const int nParcel = dialogData.players[dialogData.CurrentPlayer()].parcels.size();

	SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel newParcel;
	newParcel.eType = NDb::EPATCH_UNKNOWN;
	newParcel.fRadius = 500;
	newParcel.fDefenceDirection = 0;
	newParcel.fImportance = 0;
	newParcel.vCenter = CVec2(vCamAnchor.x, vCamAnchor.y);
	dialogData.players[dialogData.CurrentPlayer()].parcels.push_back( newParcel );

	if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
		{
			bool bResult = true;

			if ( pObjectController->AddInsertOperation(StrFmt("Players.[%d].general.parcels", dialogData.CurrentPlayer()), NODE_ADD_INDEX, pManipulator) )
			{
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Type", dialogData.CurrentPlayer(), nParcel), 
																																		string(typeAIGeneralParcel.GetMnemonic((int)newParcel.eType).c_str()), pManipulator );
				bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( StrFmt("Players.[%d].general.parcels.[%d].Center", dialogData.CurrentPlayer(), nParcel),
																																											newParcel.vCenter, pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Radius", dialogData.CurrentPlayer(), nParcel),
																																		float(newParcel.fRadius), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].DefenceDirection", dialogData.CurrentPlayer(), nParcel),
																																		float(newParcel.fDefenceDirection), pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].Importance", dialogData.CurrentPlayer(), nParcel),
																																		float(newParcel.fImportance), pManipulator );
			}
			if ( bResult )
			{
				pObjectController->Redo( false, true, GetMapInfoEditor() );
				Singleton<IControllerContainer>()->Add( pObjectController );
			}
		}
	}
}


void CAIGeneralPointsState::AddPoint( const CTPoint<int> &rMousePoint )
{
	if ( (dialogData.CurrentParcel() < 0) || (dialogData.CurrentParcel() >= dialogData.players[dialogData.CurrentPlayer()].parcels.size()) )
		return;

	CVec3 vPickedPosOnTerrain = VNULL3;
	Get2DPosOnMapHeights( &vPickedPosOnTerrain, CVec2(rMousePoint.x, rMousePoint.y) );
	NDb::SReinforcePoint newReinfPoint;

	SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &parcel = dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()];
	newReinfPoint.vCenter = CVec2(vPickedPosOnTerrain.x, vPickedPosOnTerrain.y) - parcel.vCenter;
	newReinfPoint.fDirection = parcel.fDefenceDirection;
	parcel.reinforcePoints.push_back( newReinfPoint );

	if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
	{
		if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
		{
			if ( pObjectController->AddInsertOperation(StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints", dialogData.CurrentPlayer(), dialogData.CurrentParcel()), NODE_ADD_INDEX, pManipulator) )
			{
				int nPointsNewCount = GetMapInfoEditor()->pMapInfo->players[dialogData.CurrentPlayer()].general.parcels[dialogData.CurrentParcel()].reinforcePoints.size();

				bool bResult = true;

				bResult = bResult && pObjectController->AddChangeVec2Operation<CVec2, float>( StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints.[%d].Center", dialogData.CurrentPlayer(), dialogData.CurrentParcel(), nPointsNewCount - 1),newReinfPoint.vCenter, pManipulator );
				bResult = bResult && pObjectController->AddChangeOperation( StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints.[%d].Direction", dialogData.CurrentPlayer(), dialogData.CurrentParcel(), nPointsNewCount - 1), newReinfPoint.fDirection, pManipulator );

				dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()].nCurrentPoint = nPointsNewCount - 1;

				if ( bResult )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
}


void CAIGeneralPointsState::DeletePoint()
{
	if ( (dialogData.CurrentPlayer() < 0) || (dialogData.CurrentParcel() < 0) || (dialogData.CurrentPoint() < 0) )
		return;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox(Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), 
										strMessage, 
										Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), 
										MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &parcel = dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()];
		int i = 0;
		for ( vector<NDb::SReinforcePoint>::iterator itPoint = parcel.reinforcePoints.begin(); itPoint < parcel.reinforcePoints.end(); ++itPoint, ++i )
		{
			if ( i == dialogData.CurrentPoint() )
			{
				parcel.reinforcePoints.erase( itPoint );
				break;
			}
		}

		if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
			{
				if ( pObjectController->AddRemoveOperation(StrFmt("Players.[%d].general.parcels.[%d].reinforcePoints", dialogData.CurrentPlayer(), dialogData.CurrentParcel()), dialogData.CurrentID(), pManipulator) )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
}


void CAIGeneralPointsState::DeleteID()
{
	if ( (dialogData.CurrentPlayer() < 0) || (dialogData.CurrentID() < 0) )
		return;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox(Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), 
										strMessage, 
										Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), 
										MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo &player = dialogData.players[dialogData.CurrentPlayer()];
		int i = 0;
		for ( vector<int>::iterator itID = player.mobileScriptIDs.begin(); itID < player.mobileScriptIDs.end(); ++itID, ++i )
		{
			if ( i == dialogData.CurrentID() )
			{
				dialogData.players[dialogData.CurrentPlayer()].mobileScriptIDs.erase( itID );
				break;
			}
		}

		if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
			{
				if ( pObjectController->AddRemoveOperation(StrFmt("Players.[%d].general.mobileScriptIDs"), dialogData.CurrentID(), pManipulator) )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
}


void CAIGeneralPointsState::DeleteParcel()
{
	if ( (dialogData.CurrentPlayer() < 0) || (dialogData.CurrentParcel() < 0) )
		return;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox(Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), 
										strMessage, 
										Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), 
										MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo &player = dialogData.players[dialogData.CurrentPlayer()];
		int i = 0;
		for ( vector<SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel>::iterator itParcel = player.parcels.begin(); itParcel < player.parcels.end(); ++itParcel, ++i )
		{
			if ( i == dialogData.CurrentParcel() )
			{
				player.parcels.erase( itParcel );
				break;
			}
		}

		if ( CPtr<CObjectBaseController> pObjectController = pMapInfoEditor->CreateController() )
		{
			if ( CPtr<IManipulator> pManipulator = pMapInfoEditor->GetViewManipulator() )
			{
				if ( pObjectController->AddRemoveOperation(StrFmt("Players.[%d].general.parcels", dialogData.CurrentPlayer()), dialogData.CurrentParcel(), pManipulator) )
				{
					pObjectController->Redo( false, true, GetMapInfoEditor() );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
}


void CAIGeneralPointsState::EditParcel()
{
	NDb::EParcelType newParcelType = dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()].eType;
	float fNewImportance = dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()].fImportance;
	CAIGenParcelDlg dlg( Singleton<IMainFrameContainer>()->GetSECWorkbook(), &newParcelType, &fNewImportance );
	if ( dlg.DoModal() == IDOK )
	{
		SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &parcel = dialogData.players[dialogData.CurrentPlayer()].parcels[dialogData.CurrentParcel()];
		//
		parcel.eType = newParcelType;
		parcel.fImportance = fNewImportance;

		SaveDataToDB();
		RefreshWindowData();
	}
}



