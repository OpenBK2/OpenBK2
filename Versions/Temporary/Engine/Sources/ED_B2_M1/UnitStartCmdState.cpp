#include "stdafx.h"

#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "UnitStartCmdState.h"
#include "MapEditorLib/Interface_MainFrame.h"

#include <cstdint>

//
//
//	UNIT START COMMANDS LIST
//
//

SStartCommand::SStartCommand()
{
	Init();
}


void SStartCommand::Init()
{
	nCmdType = -1;
	unitLinkIDs.clear();
	nTgtLinkID = -1;
	vTgtPos = VNULL2;
	nData = 0;
}


bool SStartCommand::LoadFromDB( IManipulator *pManipulator, int nIndex )
{
	Init();

	if ( !pManipulator )
		return false;

	if ( nIndex < 0 )
		return false;

	const std::string szDBA = StrFmt( "startCommandsList.[%d]", nIndex );
	//
	std::string szDBAType = szDBA + ".CmdType";
	if ( !CManipulatorManager::GetValue( &nCmdType, pManipulator, szDBAType ) )
		return false;
	//
	std::string szDBAunitLinkIDs = szDBA + ".unitLinkIDs";
	if ( !CManipulatorManager::GetArray<std::vector<int>,int>( &unitLinkIDs, pManipulator, szDBAunitLinkIDs ) )
		return false;
	//
	std::string 	szDBATgtLinkID = szDBA + ".LinkID";
	if ( !CManipulatorManager::GetValue( &nTgtLinkID, pManipulator, szDBATgtLinkID ) )
		return false;
	//
	std::string szDBATgtPos = szDBA + ".Pos";
	if ( !CManipulatorManager::GetVec2<CVec2,float>( &vTgtPos, pManipulator, szDBATgtPos ) )
		return false;
	//
	std::string szDBAData = szDBA + ".Number";
	if ( !CManipulatorManager::GetValue( &nData, pManipulator, szDBAData ) )
		return false;

	return true;
}


bool SStartCommand::UpdateDB( IManipulator *pManipulator, int nIndex )
{
	if ( !pManipulator )
		return false;

	if ( nIndex < 0 )
		return false;

	const std::string szDBA = StrFmt( "startCommandsList.[%d]", nIndex );
	//
	std::string szDBAType = szDBA + ".CmdType";
	if ( !CManipulatorManager::SetValue( nCmdType, pManipulator, szDBAType ) )
		return false;
	//
	std::string szDBAunitLinkIDs = szDBA + ".unitLinkIDs";
	if ( !CManipulatorManager::SetArray( unitLinkIDs, pManipulator, szDBAunitLinkIDs ) )
		return false;
	//
	std::string 	szDBATgtLinkID = szDBA + ".LinkID";
	if ( !CManipulatorManager::SetValue( nTgtLinkID, pManipulator, szDBATgtLinkID ) )
		return false;
	//
	std::string szDBATgtPos = szDBA + ".Pos";
	if ( !CManipulatorManager::SetVec2( vTgtPos, pManipulator, szDBATgtPos ) )
		return false;
	//
	std::string szDBAData = szDBA + ".Number";
	if ( !CManipulatorManager::SetValue( nData, pManipulator, szDBAData ) )
		return false;

	return true;
}


//
//
//	UNIT START COMMANDS LIST
//
//

SStartCommandList::SStartCommandList()
{
	Init();
}


void SStartCommandList::Init()
{
	commands.clear();
}


bool SStartCommandList::UpdateDB( IManipulator *pManipulator )
{
	if ( !pManipulator )
		return false;

	if ( !pManipulator->RemoveNode( "startCommandsList" ) )
		return false;

	for ( int i = 0; i < commands.size(); ++i )
	{
		if ( !pManipulator->InsertNode( "startCommandsList" ) )
			return false;
		//
		if ( !commands[i].UpdateDB( pManipulator, i ) )
			return false;
	}

	return true;
}


bool SStartCommandList::LoadFromDB( IManipulator *pManipulator )
{
	if ( !pManipulator )
		return false;

	Init();

	int nCmdNum = 0;
	if ( !CManipulatorManager::GetValue( &nCmdNum, pManipulator, "startCommandsList" ) )
		return false;

	if ( nCmdNum == 0 )
		return true;

	for ( int i = 0; i < nCmdNum; ++i )
	{
		SStartCommand cmd;
		if ( !cmd.LoadFromDB( pManipulator, i ) )
			return false;
		commands.push_back( cmd );
	}

	return true;
}


void SStartCommandList::RemoveCommands( const std::vector<int> &rIndices )
{
	// удаляет команды с индексами из rIndices
	//
	if ( rIndices.empty() || commands.empty() )
		return;
	//
	std::vector<uint8_t> mustCopy;
	mustCopy.resize( commands.size() );
	for ( int i = 0; i < commands.size(); ++i )
		mustCopy[i] = true;
	for ( int j = 0; j < rIndices.size(); ++j )
	{
		if ( rIndices[j] >= mustCopy.size() )
			continue;
		mustCopy[rIndices[j]] = false;
	}
	//
	std::vector<SStartCommand> tmp = commands;
	commands.clear();
	for ( int k = 0; k < tmp.size(); ++k )
	{
		if ( mustCopy[k] )
			commands.push_back( tmp[k] );
	}
}


//
//
//		UNIT START COMMANDS STATE
//
//

CUnitStartCmdState::CUnitStartCmdState( CMapInfoEditor* _pMapInfoEditor ) : 
	pMapInfoEditor( _pMapInfoEditor )
{
	NI_ASSERT( GetMapInfoEditor() != 0, "CUnitStartCmdState(): Invalid parameter: pMapInfoEditor == 0" );
	//
	ResetStateData();
}


NMapInfoEditor::SObjectInfoCollector* CUnitStartCmdState::GetObjectInfoCollector()
{ 
	return &( pMapInfoEditor->objectInfoCollector );
}


CMapInfoEditor* CUnitStartCmdState::GetMapInfoEditor()
{ 
	return pMapInfoEditor;
}

void CUnitStartCmdState::Enter()
{
	CMapObjectState::Enter();
	Singleton<ICommandHandlerContainer>()->Set( CHID_UNIT_START_CMD_STATE, this );
	ResetStateData();

	commandTypesMnemonic.clear();
	std::vector<SUnitCommandTypeInfo> tmp;
	if ( LoadUnitCommandTypesFromXML( &tmp ) )
	{
		for ( int i = 0; i < tmp.size(); ++i )
		{
			 commandTypesMnemonic[tmp[i].nValue] = tmp[i];
		}
	}
	//
	pEdUnitStartCmd = new CEdUnitStartCmd( this );
	pEdUnitStartCmd->Create( CEdUnitStartCmd::IDD );
	pEdUnitStartCmd->ShowWindow( SW_HIDE );
	//
	if ( !commandsList.LoadFromDB( GetMapInfoEditor()->GetViewManipulator() ) )
	{
		NI_ASSERT( 0, "CUnitStartCmdState::Enter(): can't read start commands list" );
	}
	else
	{
		RefreshDockingWindow( 0 );
	}
}


void CUnitStartCmdState::Leave()
{
	CMapObjectState::Leave();
	ResetStateData();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_UNIT_START_CMD_STATE );
}


bool CUnitStartCmdState::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	if ( CMapObjectState::HandleCommand(nCommandID, dwData) )
		return true;

	switch ( nCommandID )
	{
		case ID_UNIT_START_CMD_WINDOW_UI_EVENT:
		{
			SUnitStartCmdWindowData data;
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_UNIT_START_CMD_WINDOW, 
																																ID_WINDOW_GET_DIALOG_DATA, 
																																reinterpret_cast<uint32_t>(&data)) )
			{
				switch ( data.eLastAction )
				{
					case SUnitStartCmdWindowData::NO_CMD:
						break;
					case SUnitStartCmdWindowData::ADD_CMD:
						UsrEvtAddCmd( data );
						break;
					case SUnitStartCmdWindowData::DEL_CMD:
						UsrEvtDelCmd( data );
						break;
					case SUnitStartCmdWindowData::EDIT_CMD:
						UsrEvtEditCmd( data );
						break;
					case SUnitStartCmdWindowData::ORDER_DOWN_CMD:
						UsrEvtMoveCmd( MV_DOWN, data );
						break;
					case SUnitStartCmdWindowData::ORDER_UP_CMD:
						UsrEvtMoveCmd( MV_UP, data );
						break;
					case SUnitStartCmdWindowData::SEL_CHANGE:
						UsrEvtSelChange( data );
						break;
				}
			}
			return true;
		}
		case ID_UNIT_START_CMD_REFRESH_WINDOW:
		{
			if ( commandsList.LoadFromDB( GetMapInfoEditor()->GetViewManipulator() ) )
			{
				RefreshDockingWindow( 0 );
			}
			return true;	
		}
	}
	return false;
}


bool CUnitStartCmdState::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CUnitStartCmdState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CUnitStartCmdState::UpdateCommand(), pbCheck == 0" );

	if ( CMapObjectState::UpdateCommand( nCommandID, pbEnable, pbCheck ) )
		return true;

	switch ( nCommandID )
	{
		case ID_UNIT_START_CMD_WINDOW_UI_EVENT:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
		case ID_UNIT_START_CMD_REFRESH_WINDOW:
		{
			(*pbEnable) = true;
			(*pbCheck) = false;
			return true;
		}
	}
	return false;
}


void CUnitStartCmdState::OnMouseMove( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bEdCmdVisible )
	{
		if ( GetCurrentCommandType() && GetCurrentCommandType()->nNeedTargetUnit )
		{
			// выбор объекта
			CMapObjectState::OnMouseMove( nFlags, rMousePoint );
			if ( GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.size() != 1 )
			{
				return;
			}
			else
			{
				CObjectSelectionPartMap::iterator itObjectSelectionPart = GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.begin();
				if ( SObjectInfo *pObjectInfo = GetObjectInfoCollector()->GetObjectInfo( itObjectSelectionPart->first ) )
				{
					vCurrTargetPos = VNULL3;
					nCurrTargetUnit = itObjectSelectionPart->first;
					std::string szUnitName = GetMapObjectName( pObjectInfo );
					pEdUnitStartCmd->UpdateTarget( szUnitName );
				}
			}
		}
		else
		{
			if ( nFlags & MK_LBUTTON )
			{
				// указание точки
				nCurrTargetUnit = -1;
				Get3DPosOnMapHeights( &vCurrTargetPos, CVec2( rMousePoint.x, rMousePoint.y ) );
				vCurrTargetPos.z = GetTerrainHeight( vCurrTargetPos.x, vCurrTargetPos.y );
				UpdateCmdMarkers();
				//
				pEdUnitStartCmd->UpdateTarget( StrFmt( "%.0f : %.0f", vCurrTargetPos.x, vCurrTargetPos.y ) );
			}
		}
	}
	else
	{
		CMapObjectState::OnMouseMove( nFlags, rMousePoint );
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CUnitStartCmdState::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bEdCmdVisible )
	{
		if ( GetCurrentCommandType() && GetCurrentCommandType()->nNeedTargetUnit )
		{
			// выбор объекта
			CMapObjectState::OnLButtonDown( nFlags, rMousePoint );
			if ( GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.size() != 1 )
			{
				return;
			}
			else
			{
				CObjectSelectionPartMap::iterator itObjectSelectionPart = GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.begin();
				if ( SObjectInfo *pObjectInfo = GetObjectInfoCollector()->GetObjectInfo( itObjectSelectionPart->first ) )
				{
					vCurrTargetPos = VNULL3;
					nCurrTargetUnit = itObjectSelectionPart->first;
					std::string szUnitName = GetMapObjectName( pObjectInfo );
					pEdUnitStartCmd->UpdateTarget( szUnitName );
				}
			}
		}
		else
		{
			// указание точки
			nCurrTargetUnit = -1;
			Get3DPosOnMapHeights( &vCurrTargetPos, CVec2( rMousePoint.x, rMousePoint.y ) );
			vCurrTargetPos.z = GetTerrainHeight( vCurrTargetPos.x, vCurrTargetPos.y );
			UpdateCmdMarkers();
		}
	}
	else
	{
		CMapObjectState::OnLButtonDown( nFlags, rMousePoint );
		FilterCommandsyBySelection();
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CUnitStartCmdState::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bEdCmdVisible )
	{
		if ( GetCurrentCommandType() && GetCurrentCommandType()->nNeedTargetUnit )
		{
			// выбор объекта
			CMapObjectState::OnLButtonUp( nFlags, rMousePoint );
			if ( GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.size() != 1 )
			{
				return;
			}
			else
			{
				CObjectSelectionPartMap::iterator itObjectSelectionPart = GetObjectInfoCollector()->objectSelection.objectSelectionPartMap.begin();
				if ( SObjectInfo *pObjectInfo = GetObjectInfoCollector()->GetObjectInfo( itObjectSelectionPart->first ) )
				{
					vCurrTargetPos = VNULL3;
					nCurrTargetUnit = itObjectSelectionPart->first;
					std::string szUnitName = GetMapObjectName( pObjectInfo );
					pEdUnitStartCmd->UpdateTarget( szUnitName );
				}
			}
		}
		else
		{
			// указание точки
			nCurrTargetUnit = -1;
			Get3DPosOnMapHeights( &vCurrTargetPos, CVec2( rMousePoint.x, rMousePoint.y ) );
			vCurrTargetPos.z = GetTerrainHeight( vCurrTargetPos.x, vCurrTargetPos.y );
			UpdateCmdMarkers();
		}
	}
	else
	{
		CMapObjectState::OnLButtonUp( nFlags, rMousePoint );
		FilterCommandsyBySelection();
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CUnitStartCmdState::Draw( CPaintDC *pDC )
{
	CMapObjectState::Draw( pDC );
	DrawCommandMarkers();
	sceneDrawTool.Draw();
}


void CUnitStartCmdState::OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EDlgEvents eEvt )
{
	switch ( eEvt )
	{
		case CEdUnitStartCmd::EV_OK:
			EdCmdOK();
			GetObjectInfoCollector()->ClearSelection();
			break;
		case CEdUnitStartCmd::EV_CANCEL:
			GetObjectInfoCollector()->ClearSelection();
			EdCmdCancel();
			break;
		case CEdUnitStartCmd::EV_CLEAR:
			EdCmdClear();
			break;
		case CEdUnitStartCmd::EV_TYPE_CHANGE:
			EdCmdTypeChange();
			break;
	}
}


void CUnitStartCmdState::UsrEvtAddCmd( const SUnitStartCmdWindowData &data )
{
	if ( bEdCmdVisible )
		return;

	GetSelectedUnitIDs( &currCmdUnits );
	if ( currCmdUnits.empty() )
		return;

	GetObjectInfoCollector()->ClearSelection();

	UpdateCmdMarkers();
	
	CEdUnitStartCmd::SDlgData dd;
	pEdUnitStartCmd->SetDialogData( &dd );
	bEdCmdVisible = true;
	pEdUnitStartCmd->ShowWindow( SW_SHOW );
}


void CUnitStartCmdState::UsrEvtDelCmd( const SUnitStartCmdWindowData &data )
{
	if ( bEdCmdVisible )
		return;

	if ( data.selectedCommands.empty() )
		return;

	CString strMessage;
	strMessage.LoadString( IDS_MIMO_DELETE_OBJECT_MESSAGE );
	if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
	{
		commandsList.RemoveCommands( data.selectedCommands );
		commandsList.UpdateDB( GetMapInfoEditor()->GetViewManipulator() );
		ClearCmdMarkers();
		RefreshDockingWindow( &data.selectedCommands );
	}
}


void CUnitStartCmdState::UsrEvtEditCmd( const SUnitStartCmdWindowData &data )
{
	if ( bEdCmdVisible )
		return;

	if ( data.selectedCommands.size() != 1 )
		return;

	CEdUnitStartCmd::SDlgData dd;
	//
	dd.bEditMode = false;
	dd.nCommandIndex = data.selectedCommands.front();
	//
	SStartCommand cmd = commandsList.commands[dd.nCommandIndex];
	//
	dd.nSelectedCmdType = cmd.nCmdType;
	dd.nData = cmd.nData;
	if ( cmd.nTgtLinkID != -1 )
	{
		dd.bSelectedCmdNeedTargetUnit = true;
		int nObjectID = GetObjectInfoCollector()->linkIDMap[cmd.nTgtLinkID];
		if ( nObjectID != -1 )
		{
			SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
			if ( pOI )
			{
				dd.szTarget = GetMapObjectName( pOI );
			}
		}
		nCurrTargetUnit = nObjectID;
	}
	else
	{
		nCurrTargetUnit = -1;
		dd.bSelectedCmdNeedTargetUnit = false;
		dd.szTarget = StrFmt( "%.0f : %.0f", cmd.vTgtPos.x, cmd.vTgtPos.y );
	}

	pEdUnitStartCmd->SetDialogData( &dd );
	bEdCmdVisible = true;
	pEdUnitStartCmd->ShowWindow( SW_SHOW );
}


void CUnitStartCmdState::UsrEvtMoveCmd( EMoveDir eDir, const SUnitStartCmdWindowData &data )
{
	if ( bEdCmdVisible )
		return;

	std::vector<int> ordCmdIDs = data.selectedCommands;
	std::vector<int> newSelectedIdx;
	switch ( eDir )
	{
		case MV_UP:
		{
			sort( ordCmdIDs.begin(), ordCmdIDs.end() );
			if ( ordCmdIDs.empty() || ( ordCmdIDs.front() == 0 ) )
				return;

			for ( int i = 0; i < ordCmdIDs.size(); ++i )
			{
				int nOldIndex = ordCmdIDs[i];
				if ( nOldIndex <= 0 )
					continue;
				std::swap( commandsList.commands[nOldIndex], commandsList.commands[nOldIndex-1] );
				newSelectedIdx.push_back( nOldIndex - 1 );
			}
			break;
		}
		//
		case MV_DOWN:
		{
			sort( ordCmdIDs.begin(), ordCmdIDs.end() );
			if ( ordCmdIDs.empty() || (ordCmdIDs.back() >= (commandsList.commands.size()-1)) )
				return;

			for ( int i = ordCmdIDs.size()-1; i >= 0; --i )
			{
				int nOldIndex = ordCmdIDs[i];
				if ( nOldIndex >= commandsList.commands.size() )
					continue;
				std::swap( commandsList.commands[nOldIndex], commandsList.commands[nOldIndex+1] );
				newSelectedIdx.push_back( nOldIndex + 1 );
			}
			break;
		}
	}

	commandsList.UpdateDB( GetMapInfoEditor()->GetViewManipulator() );
	RefreshDockingWindow( &newSelectedIdx );
}


void CUnitStartCmdState::UsrEvtSelChange( const SUnitStartCmdWindowData &data )
{
	ClearCmdMarkers();
	currCmdUnits.clear();

	for ( int i = 0; i < data.selectedCommands.size(); ++i )
	{
		SStartCommand &cmd = commandsList.commands[data.selectedCommands[i]];
		for ( int j = 0; j < cmd.unitLinkIDs.size(); ++j )
		{
			int nLinkID = cmd.unitLinkIDs[j];
			if ( GetObjectInfoCollector()->linkIDMap.find( nLinkID ) != GetObjectInfoCollector()->linkIDMap.end() )
			{
				int nObjectID = GetObjectInfoCollector()->linkIDMap[nLinkID];
				currCmdUnits.push_back( nObjectID );
			}
		}
		//
		if ( cmd.nTgtLinkID == -1 )
		{
			vCurrTargetPos = CVec3( cmd.vTgtPos.x, cmd.vTgtPos.y, 0 );
		}
		else
		{
			if ( GetObjectInfoCollector()->linkIDMap.find( cmd.nTgtLinkID ) != GetObjectInfoCollector()->linkIDMap.end() )
			{
				int nObjectID = GetObjectInfoCollector()->linkIDMap[cmd.nTgtLinkID];
				nCurrTargetUnit = nObjectID;
			}
		}
	}

	UpdateCmdMarkers();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CUnitStartCmdState::GetSelectedUnitIDs( std::vector<CMapObjID> *pIDs )
{
	if ( !pIDs )
		return;

	pIDs->clear();
	//
	CObjectSelectionPartMap &objectSelectionPartMap = GetObjectInfoCollector()->objectSelection.objectSelectionPartMap;
	for ( CObjectSelectionPartMap::const_iterator itSel = objectSelectionPartMap.begin(); itSel != objectSelectionPartMap.end(); ++itSel )
	{
		if ( SObjectInfo *pObjectInfo = GetObjectInfoCollector()->GetObjectInfo(itSel->first) )
		{
			pIDs->push_back( pObjectInfo->nObjectInfoID );
		}
	}
}


void CUnitStartCmdState::ResetStateData()
{
	currCmdUnits.clear();
	//
	nCurrTargetUnit = INVALID_NODE_ID;
	if ( GetObjectInfoCollector() )
	{
		GetObjectInfoCollector()->ClearSelection();
	}
	commandsList.commands.clear();
	if ( pEdUnitStartCmd )
	{
		pEdUnitStartCmd->ShowWindow( SW_HIDE );
	}
	bEdCmdVisible = false;
}


void CUnitStartCmdState::EdCmdOK()
{
	bEdCmdVisible = false; 

	CEdUnitStartCmd::SDlgData data;
	pEdUnitStartCmd->GetDialogData( &data );

	if ( data.bEditMode == true )
	{ // new command
		SStartCommand cmd;
		cmd.nData = data.nData;
		cmd.nCmdType = data.nSelectedCmdType;
		std::vector<int> targetUnitElemIndices;
		if ( GetMapObjectElementIDs( nCurrTargetUnit, &targetUnitElemIndices ) && !targetUnitElemIndices.empty() )
		{
			cmd.nTgtLinkID = targetUnitElemIndices.front();
		}
		else
		{
			cmd.nTgtLinkID = -1;
		}
		cmd.vTgtPos = CVec2( vCurrTargetPos.x, vCurrTargetPos.y );
		GetLinkIDs( &cmd.unitLinkIDs );

		commandsList.commands.push_back( cmd );
	}
	else
	{ // edit command
		SStartCommand cmd;
		cmd.nData = data.nData;
		cmd.nCmdType = data.nSelectedCmdType;
		if ( nCurrTargetUnit != -1 )
		{
			std::vector<int> targetUnitElemIndices;
			if ( GetMapObjectElementIDs( nCurrTargetUnit, &targetUnitElemIndices ) && !targetUnitElemIndices.empty() )
			{
				cmd.nTgtLinkID = targetUnitElemIndices.front();
			}
			else
			{
				cmd.nTgtLinkID = -1;
			}
		}
		else
		{
			cmd.nTgtLinkID = -1;
		}
		cmd.vTgtPos = CVec2( vCurrTargetPos.x, vCurrTargetPos.y );
		GetLinkIDs( &cmd.unitLinkIDs );
		
		if ( data.nCommandIndex < 0 || data.nCommandIndex >= commandsList.commands.size() )
		{
			NI_ASSERT( 0, "CUnitStartCmdState::EdCmdOK(): bad command index" );
		}
		else
		{
			commandsList.commands[data.nCommandIndex] = cmd;
		}
	}

	commandsList.UpdateDB( GetMapInfoEditor()->GetViewManipulator() );
	RefreshDockingWindow( 0 );
}


void CUnitStartCmdState::EdCmdCancel()
{
	bEdCmdVisible = false;
}


void CUnitStartCmdState::EdCmdClear()
{
	nCurrTargetUnit = -1;
	vCurrTargetPos = VNULL3;
	pEdUnitStartCmd->UpdateTarget( "" );
	UpdateCmdMarkers();
}


void CUnitStartCmdState::RefreshDockingWindow( const std::vector<int> *pSelection )
{
	ClearCmdMarkers();

	SUnitStartCmdWindowData data;
	for ( int i = 0; i < commandsList.commands.size(); ++i )
	{
		SStartCommand &cmd = commandsList.commands[i];
		SUnitStartCmdWindowData::SCmd wcmd;

		wcmd.nIndex = i;
		if ( cmd.nTgtLinkID == -1 )
		{
			wcmd.szTarget = StrFmt( "( %.0f , %.0f )", cmd.vTgtPos.x, cmd.vTgtPos.y );
		}
		else
		{
			int nObjectID = GetObjectInfoCollector()->linkIDMap[cmd.nTgtLinkID];
			if ( nObjectID != -1 )
			{
				SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
				if ( pOI )
				{
					wcmd.szTarget = GetMapObjectName( pOI );
				}
			}
		}
		
		if ( commandTypesMnemonic.find(cmd.nCmdType) != commandTypesMnemonic.end() )
		{
			wcmd.szType = commandTypesMnemonic[cmd.nCmdType].szName;
		}
		else
		{
			wcmd.szType = RCSTR("<UNKNOWN>");
		}
		
		data.commands.push_back( wcmd );
	}

	if ( pSelection )
	{
		data.selectedCommands = (*pSelection);
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_UNIT_START_CMD_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA, 
																												reinterpret_cast<uint32_t>(&data) );
}


void CUnitStartCmdState::DrawCommandMarker( const SCmdMarker &rMarker )
{
	const SCmdMarker &m = rMarker;
	CVec3 vTargetPos = m.vTargetPos;
	for ( int j = 0; j < m.unitPositions.size(); ++j )
	{
		sceneDrawTool.DrawCircle( m.unitPositions[j], 50.0f, 15, 0xFFFFFF00, false );
		if ( vTargetPos.x != 0 || vTargetPos.y != 0 )
		{
			sceneDrawTool.DrawLine( vTargetPos, m.unitPositions[j], 0x00FFFF00, false );
		}
	}
	if ( m.bTargetIsUnit && m.bTargetSelected )
	{
		sceneDrawTool.DrawCircle( vTargetPos, 50.0f, 15, 0xFF0000FF, false );
		sceneDrawTool.DrawCircle( vTargetPos, 51.0f, 15, 0xFF0000FF, false );
		sceneDrawTool.DrawCircle( vTargetPos, 52.0f, 15, 0xFF0000FF, false );
	}
	else
	{
		if ( vTargetPos.x != 0 || vTargetPos.y != 0 )
		{
			CVec3 p = vTargetPos;
			sceneDrawTool.DrawCircle( p, 10.0f, 5, 0xFF804000, false );
			CVec3 pp0 = p;
			pp0.x += 5;
			CVec3 pp1 = pp0;
			pp1.x += 30.0f;
			sceneDrawTool.DrawLine( pp0, pp1, 0xFF804000, false );
			sceneDrawTool.DrawCircle( pp1, 2.0f, 3, 0xFF804000, false );
			pp0 = p;
			pp0.x -= 5;
			pp1 = pp0;
			pp1.x -= 30.0f;
			sceneDrawTool.DrawLine( pp0, pp1, 0xFF804000, false );
			sceneDrawTool.DrawCircle( pp1, 2.0f, 3, 0xFF804000, false );
			pp0 = p;
			pp0.y += 5;
			pp1 = pp0;
			pp1.y += 30.0f;
			sceneDrawTool.DrawLine( pp0, pp1, 0xFF804000, false );
			sceneDrawTool.DrawCircle( pp1, 2.0f, 3, 0xFF804000, false );
			pp0 = p;
			pp0.y -= 5;
			pp1 = pp0;
			pp1.y -= 30.0f;
			sceneDrawTool.DrawLine( pp0, pp1, 0xFF804000, false );
			sceneDrawTool.DrawCircle( pp1, 2.0f, 3, 0xFF804000, false );
		}
	}
}


void CUnitStartCmdState::DrawCommandMarkers()
{
	if ( bEdCmdVisible )
	{
		DrawCommandMarker( currentCmdMarker );
	}
	else
	{
		for ( int i = 0; i < markers.size(); ++i )
		{
			DrawCommandMarker( markers[i] );
		}
	}
}


bool CUnitStartCmdState::GetLinkIDs( std::vector<int> *pLinkIDs )
{
	if ( !pLinkIDs )
		return false;

	pLinkIDs->clear();
	//
	for ( int i = 0; i < currCmdUnits.size(); ++i )
	{
		int nMapObjectID = currCmdUnits[i];
		//				
		SObjectInfoCollector::CObjectInfoMap::iterator it = GetObjectInfoCollector()->objectInfoMap.find( nMapObjectID );
		if ( it != GetObjectInfoCollector()->objectInfoMap.end() )
		{
			SObjectInfo *pMO = it->second;
			if ( !pMO )
				continue;
			//
			// по элементам объекта
			for ( SObjectInfo::CMapInfoElementMap::iterator itElem = pMO->mapInfoElementMap.begin();
				itElem != pMO->mapInfoElementMap.end(); ++itElem )
			{
				SObjectInfo::SMapInfoElement *pElem = &itElem->second;
				if ( itElem->first != INVALID_NODE_ID )
				{
					pLinkIDs->push_back( itElem->first );
				}
			}
		}
	}
	return true;
}


std::string CUnitStartCmdState::GetMapObjectName( SObjectInfo *pMO )
{
	if ( !pMO )
		return "<error>";

	std::string szResult;
	for ( SObjectInfo::CMapInfoElementMap::iterator itElem = pMO->mapInfoElementMap.begin();
		itElem != pMO->mapInfoElementMap.end(); ++itElem )
	{
		SObjectInfo::SMapInfoElement *pElem = &itElem->second;
		const unsigned nPlayer = pElem->nPlayer;
		const std::string szType = pElem->szRPGStatsTypeName;
		const std::string szName = pElem->rpgStatsDBID.ToString();
		szResult += StrFmt( "plr(%d) %s:%s\n", nPlayer, szType.c_str(), szName.c_str() );
	}
	return szResult;
}


// returns type of command being edited currently
const SUnitCommandTypeInfo* CUnitStartCmdState::GetCurrentCommandType()
{
	if ( !bEdCmdVisible )
		return 0;
	//
	if ( pEdUnitStartCmd )
	{
		int nCommandType = pEdUnitStartCmd->GetSelectedCommandType();
		if ( commandTypesMnemonic.find( nCommandType ) != commandTypesMnemonic.end() )
		{
			return &commandTypesMnemonic[nCommandType];
		}
	}
	return 0;
}


bool CUnitStartCmdState::GetMapObjectElementIDs( int nObjectID, std::vector<int> *pRes )
{
	if ( !pRes )
		return false;

	SObjectInfo *pObjectInfo = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
	//
	if ( pObjectInfo )
	{
		for ( SObjectInfo::CMapInfoElementMap::iterator itElem = pObjectInfo->mapInfoElementMap.begin();
			itElem != pObjectInfo->mapInfoElementMap.end(); ++itElem )
		{
			pRes->push_back( itElem->first );
		}
		return true;
	}
	return false;
}


void CUnitStartCmdState::FilterCommandsyBySelection()
{
	if ( bEdCmdVisible )
		return;

	std::vector<CMapObjID> selection;
	GetSelectedUnitIDs( &selection );
	if ( selection.empty() )
	{
		RefreshDockingWindow( 0 );
		return;
	}

	SUnitStartCmdWindowData data;
	std::vector<byte> usedCommands;
	usedCommands.resize( commandsList.commands.size(), 0 );

	for ( int i = 0; i < selection.size(); ++i )
	{
		std::vector<int> elemIndices;
		if ( GetMapObjectElementIDs( selection[i], &elemIndices ) && !elemIndices.empty() )
		{
			int nCmdIndex = 0;
			for ( std::vector<int>::const_iterator itElem = elemIndices.begin(); itElem < elemIndices.end(); ++itElem )
			{
				const int nLinkID = (*itElem);
				//
				for ( std::vector<SStartCommand>::const_iterator itCmd = commandsList.commands.begin(); itCmd < commandsList.commands.end(); ++itCmd )
				{
					if ( usedCommands[nCmdIndex] )
						continue;

					for ( std::vector<int>::const_iterator itLinkId = itCmd->unitLinkIDs.begin(); itLinkId < itCmd->unitLinkIDs.end(); ++itLinkId )
					{
						if ( (*itLinkId) == nLinkID )
						{
							usedCommands[nCmdIndex] = 1;

							SUnitStartCmdWindowData::SCmd wcmd;

							wcmd.nIndex = nCmdIndex;
							if ( itCmd->nTgtLinkID == -1 )
								wcmd.szTarget = StrFmt( "%.0f : %.0f", itCmd->vTgtPos.x, itCmd->vTgtPos.y );
							else
							{
								int nObjectID = GetObjectInfoCollector()->linkIDMap[itCmd->nTgtLinkID];
								if ( nObjectID != -1 )
								{
									SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
									if ( pOI )
									{
										wcmd.szTarget = GetMapObjectName( pOI );
									}
								}
							}

							if ( commandTypesMnemonic.find( itCmd->nCmdType ) != commandTypesMnemonic.end() )
								wcmd.szType = commandTypesMnemonic[itCmd->nCmdType].szName;
							else
								wcmd.szType = RCSTR("<UNKNOWN>");

							data.commands.push_back( wcmd );

							break;
						}
					}
					++nCmdIndex;
				}
			}
		}
	}
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_UNIT_START_CMD_WINDOW, 
																												ID_WINDOW_SET_DIALOG_DATA,
																												reinterpret_cast<uint32_t>(&data) );
}


void CUnitStartCmdState::UpdateCmdMarkers()
{
	ClearCmdMarkers();
	
	currentCmdMarker.unitPositions.clear();
	for ( int j = 0; j < currCmdUnits.size(); ++j )
	{
		SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( currCmdUnits[j] );
		if ( pOI )
		{
			CVec3 p = pOI->vPosition;
			p.z = GetTerrainHeight( p.x, p.y );
			currentCmdMarker.unitPositions.push_back( p );
		}
	}
	if ( nCurrTargetUnit != -1 )
	{
		SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nCurrTargetUnit );
		if ( pOI )
		{
			CVec3 p = pOI->vPosition;
			p.z = GetTerrainHeight( p.x, p.y );
			currentCmdMarker.vTargetPos = p;
			currentCmdMarker.bTargetSelected = true;
			currentCmdMarker.bTargetIsUnit = true;
		}
	}
	else
	{
			currentCmdMarker.vTargetPos = vCurrTargetPos;
			currentCmdMarker.bTargetSelected = false;
			currentCmdMarker.bTargetIsUnit = false;
	}
	///////

	SUnitStartCmdWindowData data;
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand(CHID_UNIT_START_CMD_WINDOW, 
																														ID_WINDOW_GET_DIALOG_DATA,
																														reinterpret_cast<uint32_t>(&data)) )
	{
		for ( int i = 0; i < data.selectedCommands.size(); ++i )
		{
			int nIdx = data.selectedCommands[i];
			if ( nIdx < 0 || nIdx >= commandsList.commands.size() )
				continue;
			//
			SStartCommand &cmd = commandsList.commands[nIdx];
			//
			SCmdMarker m;
			m.bTargetIsUnit = ( ( cmd.nTgtLinkID == -1 ) ? false : true );
			m.bTargetSelected = true;
			if ( m.bTargetIsUnit )
			{
				if ( cmd.nTgtLinkID != -1 )
				{
					int nObjectID = GetObjectInfoCollector()->linkIDMap[cmd.nTgtLinkID];
					SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
					if ( pOI )
						m.vTargetPos = pOI->vPosition;
				}
			}
			else
			{
				m.vTargetPos = CVec3( cmd.vTgtPos.x, cmd.vTgtPos.y, 0 );
			}
			for ( int i = 0; i < cmd.unitLinkIDs.size(); ++i )
			{
				int nObjectID = GetObjectInfoCollector()->linkIDMap[cmd.unitLinkIDs[i]];
				SObjectInfo *pOI = GetObjectInfoCollector()->GetObjectInfo( nObjectID );
				if ( pOI )
				{
					CVec3 p = pOI->vPosition;
					p.z = GetTerrainHeight( p.x, p.y );
					m.unitPositions.push_back( p );
				}
			}
			markers.push_back( m );
		}
	}
}


void CUnitStartCmdState::ClearCmdMarkers()
{
	markers.clear();
	currentCmdMarker = SCmdMarker();
}



