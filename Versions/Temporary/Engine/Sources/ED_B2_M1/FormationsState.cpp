#include "stdafx.h"

#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "stats_b2_m1/IconsSet.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "resourcedefines.h"
#include "commandhandlerdefines.h"
#include "EditorScene.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/MaskManipulator.h"
#include "Tools_SceneGeometry.h"
#include "ExporterMethods.h"
#include "FormationMnemonics.h"
#include "SquadEditor.h"
#include "FormationsState.h"

#include <zconf.h>

static bool SetOrderPosition( IManipulator *pSquadManip, int nMemberIdx, const CVec3 &rvPos, 
														  NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation )
{
	CVariant v;

	if ( !pSquadManip->GetValue( "formations", &v ) )
		return false;

	int nFormationsNumber = v;

	int nFormationIdx = static_cast<int>(eSelectedFormation);
	if ( (nFormationIdx < 0) || (nFormationIdx > nFormationsNumber) )
		return false;

	string szOrderDBA = StrFmt( "formations.[%d].order", nFormationIdx );

	if ( !pSquadManip->GetValue( szOrderDBA, &v ) )
		return false;

	int nOrderElemNum = v;

	if ( (nMemberIdx < 0) || (nMemberIdx > nOrderElemNum) )
		return false;

	string szOrderPosDBA = StrFmt( "formations.[%d].order.[%d].Pos", nFormationIdx, nMemberIdx );

	if ( !CManipulatorManager::SetVec2( rvPos, pSquadManip, szOrderPosDBA ) ) 
		return false;

	return true;

}

static bool GetOrderPosition( CVec2 *pPos, IManipulator *pSquadManip, int nMemberIdx, 
														  NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation )
{
	CVariant v;

	if ( !pSquadManip->GetValue( "formations", &v ) )
		return false;

	int nFormationsNumber = v;

	int nFormationIdx = static_cast<int>(eSelectedFormation);
	if ( (nFormationIdx < 0) || (nFormationIdx > nFormationsNumber) )
		return false;

	string szOrderDBA = StrFmt( "formations.[%d].order", nFormationIdx );

	if ( !pSquadManip->GetValue( szOrderDBA, &v ) )
		return false;

	int nOrderElemNum = v;

	if ( (nMemberIdx < 0) || (nMemberIdx > nOrderElemNum) )
		return false;

	string szOrderPosDBA = StrFmt( "formations.[%d].order.[%d].Pos", nFormationIdx, nMemberIdx );

	if ( !CManipulatorManager::GetVec2<CVec2,float>( pPos, pSquadManip, szOrderPosDBA ) ) 
		return false;

	return true;
}

static bool SetFormationsAndOrders( IManipulator *pSquadManip )
{
	//
	//	после успешного выполнения функции:
	//	число элементов размещения (order) в каждой формации = числу людей во взводе (members)
	//  соответсвие между элементами members[] и formations[].orders[] происходит по индексу эл-та
	//

	CVariant v;

	if ( !pSquadManip->GetValue( "formations", &v ) )
		return false;

    int nFormationsNumber = v;

	if ( nFormationsNumber > typeFormationMnemonics.Size() )
	{
		// лишние типы формаций
		int nNumForDel = nFormationsNumber - typeFormationMnemonics.Size();
		int nDeletedIdx = typeFormationMnemonics.Size();
		for ( int d = 0; d < nNumForDel; ++d )
		{
			if ( !pSquadManip->RemoveNode( "formations", nDeletedIdx ) )
				return false;
		}
	}
	else if ( nFormationsNumber < typeFormationMnemonics.Size() )
	{
		// нет каких-то типов формаций
	}

	if ( !pSquadManip->GetValue( "members", &v ) )
		return false;

    int nMembersNumber = v;

	for ( int i = 0; i < nFormationsNumber; ++i )
	{
		string szOrdersDBA = StrFmt( "formations.[%d].order", i );

		CVariant numOrders;
		if ( !pSquadManip->GetValue( szOrdersDBA, &numOrders ) )
			return false;

		int nNumOrders = numOrders;

		if ( nNumOrders > nMembersNumber )
		{
			int nNumForDel = nNumOrders - nMembersNumber;
			int nDeletedIdx = nMembersNumber;

			for ( int d = 0; d < nNumForDel; ++d )
			{
				if ( !pSquadManip->RemoveNode( szOrdersDBA, nDeletedIdx ) )
					return false;
			}
		}
		else if ( nNumOrders < nMembersNumber )
		{
			int nNumForAdd = nMembersNumber - nNumOrders;

			for ( int a = 0; a < nNumForAdd; ++a )
			{
				if ( !pSquadManip->InsertNode( szOrdersDBA ) )
					return false;
			}
		}
	}

	return true;
}

//
//	возвращает кол-во людей во взводе
//

static int GetMembersNum( IManipulator *pSquadManip )
{
	string szDBA = StrFmt( "members" );
	CVariant v;
	if ( pSquadManip->GetValue( szDBA, &v ) )
		return (int)v;
	return 0;
}

//
//	возвращает список моделей взвода
//

static void GetMembersModels( vector<string> *pModels, IManipulator *pSquadManip )
{
	pModels->clear();
	int nMembNum = GetMembersNum( pSquadManip );
	pModels->resize( nMembNum );

	for ( int i = 0; i < nMembNum; ++i )
	{
		string szDBA = StrFmt( "members.[%d]", i );
		try
		{
			CPtr<IManipulator> pInfantryManip = 
				CManipulatorManager::CreateManipulatorFromReference( szDBA, pSquadManip, 0, 0, 0 );
			CPtr<IManipulator> pVisObjManip = 
				CManipulatorManager::CreateManipulatorFromReference( "visualObject", pInfantryManip, 0, 0, 0 );
			if ( pVisObjManip == 0 )
			{
				NLog::Log( LT_ERROR, "Member %d with empty visual object!\n", i );
				pModels->clear();
				return;
			}
			string szModelName;
			CPtr<IManipulator> pModelManip = CreateModelManipulatorFromVisObj( pVisObjManip, &szModelName );
			(*pModels)[i] = szModelName;
		}
		catch ( ... ) 
		{
			NLog::GetLogger()->Log( LT_IMPORTANT, StrFmt( "Can't get model for %d squad member\n", i ) );
		}
	}
}

void GetExistingFormations( SFormationWindowDialogData *pData, IManipulator *pSquadManip )
{
	pData->squadFormations.clear();

	CVariant numForm;
	if ( !pSquadManip->GetValue( "formations", &numForm ) )
		return;

	int nNumForm = numForm;

	for ( int i = 0; i < nNumForm; ++i )
	{
		string szTypeDBA = StrFmt( "formations.[%d].type", i );

		CVariant type;

		if ( !pSquadManip->GetValue( szTypeDBA, &type ) )
			continue;

		NDb::SSquadRPGStats::SFormation::EFormationMoveType eType = 
			(NDb::SSquadRPGStats::SFormation::EFormationMoveType)typeFormationMnemonics.GetValue(type.GetStr());
		pData->squadFormations.push_back(eType);
	}
}

//
//
//			FORMATIONS STATE
//
//

CFormationsState::CFormationsState( CSquadEditor *_pSquadEditor )
	:	pSquadEditor( _pSquadEditor ),
	pMaskManipulator( 0 )
{
	NI_ASSERT( pSquadEditor != 0, "CFormationsState(): Invalid parameter: pSquadEditor == 0" );
}


void CFormationsState::Enter()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_SQUAD_FORMATIONS_STATE, this );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();

	GetExistingFormations( &currDialogData, pSquadManip );
    SetFormationWindowDialogData( &currDialogData );

	bool bRes = SetFormationsAndOrders( pSquadManip );
	if ( !bRes )
	{
		NI_ASSERT( 0, "SetFormationsAndOrders() == false" );
	}

	pSquadEditor->ShowAxis();
	RefreshState( true );
}

void CFormationsState::Leave()
{
	pSquadEditor->HideAxis();
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SQUAD_FORMATIONS_STATE );
}

bool CFormationsState::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	switch( nCommandID ) 
	{
	case ID_FORMATION_WINDOW_CHANGE_STATE:
		{
			RefreshState( false );							
			return true;
		}
		break;
	}
	return false;
}

bool CFormationsState::GetFormationWindowDialogData( SFormationWindowDialogData *pData )
{
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_FORMATION_LIST_DIALOG, 
																																		ID_WINDOW_GET_DIALOG_DATA, 
																																		reinterpret_cast<uint32_t>(pData) );

	return bRes;
}

bool CFormationsState::SetFormationWindowDialogData( SFormationWindowDialogData *pData )
{
	bool bRes = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_FORMATION_LIST_DIALOG, 
																																		ID_WINDOW_SET_DIALOG_DATA, 
																																		reinterpret_cast<uint32_t>(pData) );

	return bRes;
}

void CFormationsState::RefreshState( bool bForce )
{
	SFormationWindowDialogData data;

	if ( GetFormationWindowDialogData(&data) )
	{
		if ( bForce || ( currDialogData.eSelectedFormation != data.eSelectedFormation ) )
		{
			ReloadSquad( data.eSelectedFormation );
			if ( data.bChkPropmask )
				SetMaskManipulator( data.eSelectedFormation );
			else
				ClearMaskManipulator();
		}
		if ( currDialogData.bChkPropmask != data.bChkPropmask )
		{
			if ( data.bChkPropmask )
				SetMaskManipulator( data.eSelectedFormation );
			else
				ClearMaskManipulator();
		}
		currDialogData = data;
	}
}

bool CFormationsState::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CFormationsState::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CFormationsState::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
	case ID_FORMATION_WINDOW_CHANGE_STATE:
		{
			return true;
		}
		break;
	}
	return false;
}

void CFormationsState::PostDraw( class CPaintDC *pPaintDC )
{
	int nOldBkMode = pPaintDC->SetBkMode( TRANSPARENT );
	COLORREF oldColor = pPaintDC->GetTextColor(); 
	pPaintDC->SetTextColor( RGB(255,128,64) ); 

	if ( !pickObjects.empty() )
	{
		int nSceneID = pickObjects.front();
		int nMemberIndex = pSquadEditor->GetMemberIndexBySceneID( nSceneID );
		if ( nMemberIndex != -1 )
		{
			string szSelId = StrFmt( "selected squad member id = %d", nMemberIndex );
			pPaintDC->TextOut( 8, 8, szSelId.c_str(), szSelId.length() );
		}
	}

	pPaintDC->SetTextColor( oldColor ); 
	pPaintDC->SetBkMode( nOldBkMode );
}

void CFormationsState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( nFlags & MK_RBUTTON )
	{
		return;
	}
	if ( IEditorScene *pScene = EditorScene() )
	{
		CVec3 vPos = VNULL3;
		Get3DPosOnMapHeights( &vPos, CVec2( rMousePoint.x, rMousePoint.y ) );
		pickObjects.clear();
		pScene->PickObjects( pickObjects, CVec2( rMousePoint.x, rMousePoint.y ) );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

void CFormationsState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( nFlags & MK_RBUTTON )
		return;

	if ( !pickObjects.empty() )
	{
		int nMemberModelID = pickObjects.front();

		CVec3 vPos = VNULL3;
		Get3DPosOnMapHeights( &vPos, CVec2( rMousePoint.x, rMousePoint.y ) );

		pSquadEditor->SetModelPosition( nMemberModelID, vPos );

		CVec3 vSquadRelativePos = (vPos - pSquadEditor->GetSquadCenterPos());

		CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();
		SetOrderPosition( pSquadManip, pSquadEditor->GetModelMemberIndex(nMemberModelID), 
						  vSquadRelativePos, currDialogData.eSelectedFormation );
	}

	pickObjects.clear();

	if ( IEditorScene *pScene = EditorScene() )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_UPDATE_VALUES, 0 );
}

void CFormationsState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( nFlags & MK_RBUTTON ) 
		return;
	
	if ( pickObjects.empty() )
		return; 
	
	CVec3 vPos = VNULL3;
	Get3DPosOnMapHeights( &vPos, CVec2( rMousePoint.x, rMousePoint.y ) );
	
	if ( IEditorScene *pScene = EditorScene() )
	{
		pScene->MoveObject( pickObjects.front(), vPos, CQuat( 0.0f, V3_AXIS_Z ) ); 
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

void CFormationsState::ReloadSquad( NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation )
{
	pSquadEditor->RemoveAllModels();

	vector<string> memberModelNameList;
	CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();
	GetMembersModels( &memberModelNameList, pSquadManip );

	vector<const NDb::SModel*> memberModels;
	for ( vector<string>::iterator it = memberModelNameList.begin(); it != memberModelNameList.end(); ++it )
		memberModels.push_back( NDb::Get<NDb::SModel>( CDBID( *it ) ) );

	// позиции
	vector<CVec2> positions;
	for ( int m = 0; m < memberModelNameList.size(); ++m )
	{
		CVec2 pos = VNULL2;
		CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();
		GetOrderPosition( &pos, pSquadManip, m, eSelectedFormation );
		positions.push_back( pos );
	}

	NI_ASSERT( (positions.size() == memberModels.size()), "(positions.size() != memberModels.size())" );

	// расстановка
	int nMemberIndex = 0;
	vector<CVec2>::iterator itPos = positions.begin();
	for (	vector<const NDb::SModel*>::iterator itModel = memberModels.begin(); 
			itModel != memberModels.end(); ++itModel, ++itPos )
	{
		const NDb::SModel *pModel = *itModel;
		if ( pModel ) 
		{
			CVec2 pos = *itPos;
			pSquadEditor->AddModel( pModel, CVec3(pos.x, pos.y, 0.0f), nMemberIndex ); 
		}
		++nMemberIndex;
	}
}

void CFormationsState::SetMaskManipulator( NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation )
{
	if ( pMaskManipulator != 0 )
		ClearMaskManipulator();

	IView *pView = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<uint32_t>(&pView) );
	if ( pView != 0 )
	{
		pView->RemoveViewManipulator();
	}
	pMaskManipulator = 0;	


	SFormationWindowDialogData tmpData; 
	CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();
	GetExistingFormations( &tmpData, pSquadManip ); 
	int nElemIndex = -1;
	for ( int f = 0; f < tmpData.squadFormations.size(); ++f )
	{
		if ( tmpData.squadFormations[f] == eSelectedFormation )
		{
			nElemIndex = f;
			break;
		}
	}

	const string szMask = StrFmt( "%s.[%d].", "formations", nElemIndex );
	
	pMaskManipulator = new CMaskManipulator( szMask, pSquadEditor->GetViewManipulator(), CMaskManipulator::SMART_MODE );

	pMaskManipulator->AddName( "type", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "order", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "LieFlag", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "SpeedBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "DispersionBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "FireRateBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "RelaxTimeBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "CoverBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "VisibleBonus", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "changesByEvent", false, "", INVALID_NODE_ID, false );
		
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<uint32_t>(&pView) );
	bool bNeedCreateTree = true;
	if ( pView != 0 )
	{
		bNeedCreateTree = ( pView->GetViewManipulator() != pMaskManipulator );
		pView->SetViewManipulator( pMaskManipulator, pSquadEditor->GetObjectSet(), string() );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												bNeedCreateTree ? ID_PC_DIALOG_CREATE_TREE : ID_PC_DIALOG_UPDATE_VALUES, 
																												0 );

	ICommandHandler *pCommandHandler = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_COMMAND_HANDLER, 
																												reinterpret_cast<uint32_t>(&pCommandHandler) );
	if ( pCommandHandler != 0 )
	{
		pCommandHandler->HandleCommand( ID_PC_EXPAND_ALL, 0 );
	}
}

void CFormationsState::ClearMaskManipulator()
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
			CPtr<IManipulator> pSquadManip = pSquadEditor->CreateSquadManipulator();
			pView->SetViewManipulator( pSquadManip, pSquadEditor->GetObjectSet(), string() );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
		pMaskManipulator = 0;	
	}
}


