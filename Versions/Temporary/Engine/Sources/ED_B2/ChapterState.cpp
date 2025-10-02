#include "StdAfx.h"
#include "ChapterState.h"
#include "mapeditorlib/interface_commandhandler.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commoneditormethods.h"
#include "ChapterInterface.h"
#include "ChapterEditor.h"
#include "ED_Common/UIScene.h"
#include "libdb/ResourceManager.h"
#include "GameX/DBScenario.h"
#include "ui/ui.h"
#include "ui/dbuserinterface.h"
#include "UISpecificB2/UISpecificB2.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "LibDB/EditorDB.h"
#include "LibDB/ObjMan.h"
#include "mapeditorlib/MaskManipulator.h"

// CChapterState

CChapterState::CChapterState( CChapterEditor *_pChapterEditor ) :
	pChapterEditor( _pChapterEditor ),
	bDragging( false ),
	pPickedMission( 0 ),
	pPickedArrow( 0 )
{
}

void CChapterState::Enter()
{
//	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CChapterInterfaceCommand( new CChapterInterface() ) ) );

	// clear the scene
	Singleton<IUIScene>()->Create();

	LoadChapterMap();
	
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_SCROLLBARS, 1 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	//
	CDefaultInputState::Enter();
}

void CChapterState::Leave()
{
	CDefaultInputState::Leave();

	bDragging = false;
	pMissionBtn = 0;
	pBigMissionBtn = 0;
	pFrontLines = 0;
	pPickedMission = 0;
	pPickedArrow = 0;
	pFrontLines = 0;
	pHelper = 0;
	pChapter = 0;
	pScreen = 0;

	// Обновляем сцену
//	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );

	// clear the scene
	Singleton<IUIScene>()->Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_SCROLLBARS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}

void CChapterState::PostDraw( class CPaintDC *pPaintDC )
{
	CTRect<float> wrc;
	wrc.SetEmpty();

	if ( pPickedMission )
	{
		if ( IsValid( pPickedMission->pWnd ) )
		{
			pPickedMission->pWnd->FillWindowRect( &wrc );
			Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( wrc, &wrc );
		}
	}
	else if ( pPickedArrow )
	{
		wrc = pPickedArrow->rcBounds;
		Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( wrc, &wrc );
	}
	
	if ( !wrc.IsEmpty() )
	{
		int nOldBkMode = pPaintDC->SetBkMode( TRANSPARENT );

		const int x1 = wrc.left;
		const int y1 = wrc.top;
		const int x2 = wrc.right-1;
		const int y2 = wrc.bottom-1;
		const int HS = 4; // helper's size

		int nOldROP2   = pPaintDC->SetROP2( R2_NOT );

		// draw picked rect
		CPen pen(PS_DOT, 1, RGB(255,255,255));
		CPen * pOldPen = pPaintDC->SelectObject( &pen );
		pPaintDC->MoveTo( x1, y1 );
		pPaintDC->LineTo( x2, y1 );
		pPaintDC->LineTo( x2, y2 );
		pPaintDC->LineTo( x1, y2 );
		pPaintDC->LineTo( x1, y1 );
		pPaintDC->SelectObject( pOldPen );

		CBrush solidBrush;
		solidBrush.CreateSolidBrush( RGB(0,255,0) ); 
		CRect rc;
		
		rc.SetRect( x1-HS, y1-HS, x1+HS, y1+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		rc.SetRect( x2-HS, y1-HS, x2+HS, y1+HS );
		pPaintDC->FillRect( &rc, &solidBrush );
		
		rc.SetRect( x2-HS, y2-HS, x2+HS, y2+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		rc.SetRect( x1-HS, y2-HS, x1+HS, y2+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		rc.SetRect( x1-HS, (y1+y2)/2-HS, x1+HS, (y1+y2)/2+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		rc.SetRect( x2-HS, (y1+y2)/2-HS, x2+HS, (y1+y2)/2+HS );
		pPaintDC->FillRect( &rc, &solidBrush );
		
		rc.SetRect( (x1+x2)/2-HS, y1-HS, (x1+x2)/2+HS, y1+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		rc.SetRect( (x1+x2)/2-HS, y2-HS, (x1+x2)/2+HS, y2+HS );
		pPaintDC->FillRect( &rc, &solidBrush );

		pPaintDC->SetROP2( nOldROP2 );

		pPaintDC->SetBkMode( nOldBkMode );
	}

	CDefaultInputState::PostDraw( pPaintDC );
}

void CChapterState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	CVec2 vScreenPos;
	Singleton<IUIInitialization>()->GetVirtualScreenController()->ScreenToVirtual( CVec2(rMousePoint.x, rMousePoint.y), &vScreenPos );
	vStartPos = vScreenPos;
	vPrevPos = vScreenPos;
	
	pPickedMission = 0;
	pPickedArrow = 0;

	int nMinDist2 = -1;
	for ( int i = 0; i < pHelper->missions.size(); ++i )
	{
		SChapterMapMenuHelper::SMission &mission = pHelper->missions[i];
		IWindow *pWnd = mission.pWnd;
		if ( !pWnd )
			continue;
			
		CTRect<float> rcWnd = pWnd->GetWindowRect();
		if ( rcWnd.IsInside( vScreenPos ) )
		{
			int nDist2 = fabs2( vScreenPos - mission.vPos );
			if ( nMinDist2 == -1 || nDist2 < nMinDist2 )
			{
				pPickedMission = &mission;
				pPickedArrow = 0;
				nMinDist2 = nDist2;
			}
		}

		for ( int j = 0; j < mission.arrows.size(); ++j )
		{
			SChapterMapMenuHelper::SArrow &arrow = mission.arrows[j];

			if ( arrow.rcBounds.IsInside( vScreenPos ) )
			{
				int nDist2 = fabs2( vScreenPos.x - (arrow.rcBounds.x1 + arrow.rcBounds.x2) / 2.0f, 
					vScreenPos.y - (arrow.rcBounds.y1 + arrow.rcBounds.y2) / 2.0f );
				if ( nMinDist2 == -1 || nDist2 < nMinDist2 )
				{
					pPickedMission = 0;
					pPickedArrow = &arrow;
					nMinDist2 = nDist2;
				}
			}
		}
	}

	bDragging = (pPickedMission != 0) || (pPickedArrow != 0);

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::OnLButtonDown( nFlags, rMousePoint );
	
	UpdateMaskManipulator();
}

void CChapterState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bDragging )
	{
		CVec2 vScreenPos;
		Singleton<IUIInitialization>()->GetVirtualScreenController()->ScreenToVirtual( CVec2(rMousePoint.x, rMousePoint.y), &vScreenPos );
		
		if ( pPickedMission )
			MoveMission( pPickedMission, vScreenPos, true );
		else if ( pPickedArrow )
			MoveArrow( pPickedArrow, vScreenPos, true );
		
		vPrevPos = vScreenPos;

		UpdateFrontLines();
	}

	bDragging = false;

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::OnLButtonUp( nFlags, rMousePoint );
}

void CChapterState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	pPickedMission = 0;

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::OnRButtonDown( nFlags, rMousePoint );
}

void CChapterState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bDragging )
	{
		CVec2 vScreenPos;
		Singleton<IUIInitialization>()->GetVirtualScreenController()->ScreenToVirtual( CVec2(rMousePoint.x, rMousePoint.y), &vScreenPos );

		bool bMoved = false;
		if ( pPickedMission )
			bMoved = MoveMission( pPickedMission, vScreenPos, false );
		else if ( pPickedArrow )
			bMoved = MoveArrow( pPickedArrow, vScreenPos, false );

		if ( bMoved )
		{
			vPrevPos = vScreenPos;

			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}

	CDefaultInputState::OnMouseMove( nFlags, rMousePoint );
}

void CChapterState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	CWaitCursor wc;
	switch( nChar )
	{
		case VK_UP:
			OnKeyArrows( 0, -1 );
			break;
			
		case VK_DOWN:
			OnKeyArrows( 0, +1 );
			break;

		case VK_LEFT:
			OnKeyArrows( -1, 0 );
			break;

		case VK_RIGHT:
			OnKeyArrows( +1, 0 );
			break;

		default:
			break;
	}

	CDefaultInputState::OnKeyDown( nChar, nRepCnt, nFlags );
}

void CChapterState::LoadChapterMap()
{
	NI_ASSERT( Singleton<IUIScene>() != 0, "CChapterState::LoadChapterMap: Singleton<IUIScene>() == 0" );
	const string &szTypeName = pChapterEditor->GetObjectSet().szObjectTypeName;

	// get pointer to selected element
	const NDb::CResource *pSelected = NDb::GetObject( pChapterEditor->GetObjectSet().objectNameSet.begin()->first );
	NI_ASSERT( pSelected != 0, "CChapterState::LoadChapterMap: pSelected == 0" );

	pChapter = dynamic_cast<const NDb::SChapter*>( pSelected );
	NI_VERIFY( pChapter != 0, "CChapterState::LoadChapterMap: pChapter == 0", return );
	
	CreateScreen();

	if ( pScreen )
	{
		pMissionBtn = GetChildChecked<IButton>( pScreen, "ChapterMapTarget", true );
		pBigMissionBtn = GetChildChecked<IButton>( pScreen, "ChapterMapTargetBig", true );
		pFrontLines = GetChildChecked<IPotentialLines>( pScreen, "Frontlines", true );
		IWindow *pChapterMap = GetChildChecked<IWindow>( pScreen, "Map", true );
		pNumberView = GetChildChecked<ITextView>( pScreen, "Number", true );
		pBigNumberView = GetChildChecked<ITextView>( pScreen, "BigNumber", true );

		if ( pChapterMap )
		{
			int nSizeX;
			int nSizeY;
			pChapterMap->GetPlacement( 0, 0, &nSizeX, &nSizeY );
			vChapterMapSize.Set( nSizeX, nSizeY );

			pChapterMap->SetTexture( pChapter->pMapPicture );
		}

		pHelper = new SChapterMapMenuHelper( pChapter, pChapterMap );
		
		if ( pFrontLines )
		{
			pFrontLines->SetParams( pChapter->szSeaNoiseMask, pChapter->szDifferentColourMap, pHelper->vMainStrike, pChapter->nPositiveColour, pChapter->nNegativeColour );
			pFrontLines->ShowWindow( true );
		}

		for ( int i = 0; i < pHelper->missions.size(); ++i )
		{
			SChapterMapMenuHelper::SMission &mission = pHelper->missions[i];

			IWindow *pWnd = AddWindowCopy( pChapterMap, (i == 0) ? pBigMissionBtn : pMissionBtn );
			mission.pWnd = pWnd;
			if ( !pWnd )
				continue;

			int nSizeX;
			int nSizeY;
			pWnd->GetPlacement( 0, 0, &nSizeX, &nSizeY );
			int nX = mission.vPos.x - nSizeX / 2;
			int nY = mission.vPos.y - nSizeY / 2;
			pWnd->SetPlacement( nX, nY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
			pWnd->ShowWindow( true );

			ITextView *pNumber = dynamic_cast<ITextView*>( AddWindowCopy( pWnd, (i == 0) ? pBigNumberView : pNumberView ) );
			if ( pNumber )
				pNumber->SetText( pNumber->GetDBText() + NStr::ToUnicode( StrFmt( "%d", i ) ) );

			for ( int j = 0; j < mission.arrows.size(); ++j )
			{
				const SChapterMapMenuHelper::SArrow &arrow = mission.arrows[j];
				
				if ( pFrontLines )
					pFrontLines->AddArrow( arrow.points, arrow.fWidth, arrow.pTexture, arrow.GetColor() );
			}
		}
	}
	
	UpdateFrontLines();
}

void CChapterState::CreateScreen()
{
	const CChapterEditorSettings &editorSettings = pChapterEditor->GetEditorSettings();

	// direct ID to db templates
	const CDBID &dbidWindowScreenID = editorSettings.dbidTemplateScreenID;
	NI_VERIFY( !dbidWindowScreenID.IsEmpty(), "CChapterState::CreateScreen: dbidWindowScreenID is empty", return );

	// get pointer to WindowScreen
	const NDb::SWindowScreen *pScreenDesc = NDb::Get<NDb::SWindowScreen>( dbidWindowScreenID );
	NI_VERIFY( pScreenDesc != 0, "CChapterState::CreateScreen: pScreenDesc == 0", return );

	// create template screen
	IWindow *pScreenWnd = Singleton<IUIInitialization>()->CreateScreenFromDesc( pScreenDesc, 0, 0, Singleton<IUIScene>()->GetG2DView(), 0, 0 );
	pScreen = dynamic_cast<IScreen*>( pScreenWnd );
	NI_VERIFY( pScreen != 0, "CChapterState::CreateScreen: pScreen == 0", return );

	// add screen to scene
	Singleton<IUIScene>()->AddWindow( pScreen );

	// avoid fitting UI to viewport size
	Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
}

bool CChapterState::MoveMission( SChapterMapMenuHelper::SMission *pMission, const CVec2 &vPos, bool bSave )
{
	if ( !pMission || !IsValid( pMission->pWnd ) )
		return false;

	bool bMoved = false;
	CVec2 vDelta = vPos - vPrevPos;
	int nDeltaX = Clamp( pMission->vPos.x + vDelta.x, 0.0f, vChapterMapSize.x ) - pMission->vPos.x;
	int nDeltaY = Clamp( pMission->vPos.y + vDelta.y, 0.0f, vChapterMapSize.y ) - pMission->vPos.y;
	if ( nDeltaX != 0 || nDeltaY != 0 )
	{
		int nX;
		int nY;
		pMission->pWnd->GetPlacement( &nX, &nY, 0, 0 );
		pMission->pWnd->SetPlacement( nX + nDeltaX, nY + nDeltaY, 0, 0, EWPF_POS_X | EWPF_POS_Y );
		
		pMission->vPos.x += nDeltaX;
		pMission->vPos.y += nDeltaY;
		
		bMoved = true;
	}

	CVec2 vStartDelta = vPos - vStartPos;
	if ( bSave && pHelper->pDetailsMap && (vStartDelta.x != 0.0f || vStartDelta.y != 0.0f) )
	{
		NDb::IObjMan *pObjMan = NDb::GetManipulator( pHelper->pDetailsMap->GetDBID() );
		if ( pObjMan )
		{
			NDb::SMapInfo *pDetailsMap = dynamic_cast<NDb::SMapInfo*>( pObjMan->GetObject() );
			if ( pDetailsMap )
			{
				pHelper->UpdateMission( pDetailsMap, pMission );
				NDb::MarkChanged( pHelper->pDetailsMap->GetDBID() );
			}
		}
	}
	
	return bMoved;
}

bool CChapterState::MoveArrow( SChapterMapMenuHelper::SArrow *pArrow, const CVec2 &vPos, bool bSave )
{
	if ( !pArrow )
		return false;

	bool bMoved = false;
	CVec2 vDelta = vPos - vPrevPos;
	CVec2 vCenter( floorf( (pArrow->rcBounds.x1 + pArrow->rcBounds.x2) / 2.0f ), 
		floorf( (pArrow->rcBounds.y1 + pArrow->rcBounds.y2) / 2.0f ) );
	int nDeltaX = Clamp( vCenter.x + vDelta.x, 0.0f, vChapterMapSize.x ) - vCenter.x;
	int nDeltaY = Clamp( vCenter.y + vDelta.y, 0.0f, vChapterMapSize.y ) - vCenter.y;
	if ( nDeltaX != 0 || nDeltaY != 0 )
	{
		pHelper->MoveArrow( pArrow, CVec2( nDeltaX, nDeltaY ) );
		
		if ( pFrontLines )
			pFrontLines->ClearArrows();

		for ( int i = 0; i < pHelper->missions.size(); ++i )
		{
			SChapterMapMenuHelper::SMission &mission = pHelper->missions[i];

			for ( int j = 0; j < mission.arrows.size(); ++j )
			{
				const SChapterMapMenuHelper::SArrow &arrow = mission.arrows[j];
				
				if ( pFrontLines )
					pFrontLines->AddArrow( arrow.points, arrow.fWidth, arrow.pTexture, arrow.GetColor() );
			}
		}

		bMoved = true;
	}

	CVec2 vStartDelta = vPos - vStartPos;
	if ( pHelper->pDetailsMap && (vStartDelta.x != 0.0f || vStartDelta.y != 0.0f) )
	{
		if ( bSave )
		{
			NDb::IObjMan *pObjMan = NDb::GetManipulator( pHelper->pDetailsMap->GetDBID() );
			if ( pObjMan )
			{
				NDb::SMapInfo *pDetailsMap = dynamic_cast<NDb::SMapInfo*>( pObjMan->GetObject() );
				if ( pDetailsMap )
				{
					pHelper->UpdateArrow( pDetailsMap, pArrow );
					NDb::MarkChanged( pHelper->pDetailsMap->GetDBID() );
				}
			}
		}
	}

	return bMoved;
}

void CChapterState::UpdateFrontLines()
{
	if ( pFrontLines )
		pFrontLines->SetParams( pChapter->szSeaNoiseMask, pChapter->szDifferentColourMap, pHelper->vMainStrike, pChapter->nPositiveColour, pChapter->nNegativeColour );

	if ( pFrontLines )
		pFrontLines->ClearNodes();

	for ( int i = 0; i < pHelper->missions.size(); ++i )
	{
		SChapterMapMenuHelper::SMission &mission = pHelper->missions[i];

		if ( pFrontLines )
			pFrontLines->SetNode( mission.vPos.x, mission.vPos.y, mission.vEndOffset.x, mission.vEndOffset.y, 
				mission.bShowPotentialComplete ? mission.fPotentialComplete : mission.fPotentialIncomplete );
	}
}

void CChapterState::SetMaskManipulatorMission( SChapterMapMenuHelper::SMission *pMission )
{
	IView *pView = ClearView();

	const string szMask = StrFmt( "MissionPath.[%d].", pMission->nIndex );
	pMaskManipulator = new CMaskManipulator( szMask, pChapterEditor->GetViewManipulator(), CMaskManipulator::SMART_MODE );

	pMaskManipulator->AddName( "PotentialIncomplete", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "PotentialComplete", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "ShowPotentialComplete", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "EndOffset", false, "", INVALID_NODE_ID, false );

	SetView( pView );
}

void CChapterState::SetMaskManipulatorArrow( SChapterMapMenuHelper::SArrow *pArrow )
{
	IView *pView = ClearView();

	CPtr<IManipulator> pMapManipulator;
	if ( pHelper->pDetailsMap )
	{
		pMapManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( 
			"MapInfo", pHelper->pDetailsMap->GetDBID().ToString() );
	}
	const string szMask = StrFmt( "Roads.[%d].", pArrow->nID );
	pMaskManipulator = new CMaskManipulator( szMask, pMapManipulator, CMaskManipulator::SMART_MODE );

	pMaskManipulator->AddName( "CMArrowType", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "CMArrowMission", false, "", INVALID_NODE_ID, false );
	pMaskManipulator->AddName( "CMArrowMission2", false, "", INVALID_NODE_ID, false );

	SetView( pView );
}

IView* CChapterState::ClearView()
{
	if ( pMaskManipulator != 0 )
		ClearMaskManipulator();

	IView *pView = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<DWORD>(&pView) );
	if ( pView != 0 )
	{
		pView->RemoveViewManipulator();
	}
	pMaskManipulator = 0;	
	
	return pView;
}

void CChapterState::SetView( IView *pView )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_VIEW, 
																												reinterpret_cast<DWORD>(&pView) );
	bool bNeedCreateTree = true;
	if ( pView != 0 )
	{
		bNeedCreateTree = ( pView->GetViewManipulator() != pMaskManipulator );
		pView->SetViewManipulator( pMaskManipulator, pChapterEditor->GetObjectSet(), string() );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												bNeedCreateTree ? ID_PC_DIALOG_CREATE_TREE : ID_PC_DIALOG_UPDATE_VALUES, 
																												0 );

	ICommandHandler *pCommandHandler = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, 
																												ID_PC_DIALOG_GET_COMMAND_HANDLER, 
																												reinterpret_cast<DWORD>(&pCommandHandler) );
	if ( pCommandHandler != 0 )
	{
		pCommandHandler->HandleCommand( ID_PC_EXPAND_ALL, 0 );
	}
}

void CChapterState::ClearMaskManipulator()
{
	if ( pMaskManipulator != 0 )
	{
		IView *pView = 0;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG,
																													ID_PC_DIALOG_GET_VIEW, 
																													reinterpret_cast<DWORD>(&pView) );
		if ( pView != 0 )
		{
			pView->RemoveViewManipulator();
			CPtr<IManipulator> pChapterManipulator = pChapterEditor->CreateChapterManipulator();
			pView->SetViewManipulator( pChapterManipulator, pChapterEditor->GetObjectSet(), string() );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
		pMaskManipulator = 0;
	}
}

void CChapterState::UpdateMaskManipulator()
{
	if ( pPickedMission != 0 )
	{
		SetMaskManipulatorMission( pPickedMission );
	}
	else if ( pPickedArrow != 0 )
	{
		SetMaskManipulatorArrow( pPickedArrow );
	}
	else
	{
		ClearMaskManipulator();
	}
}

void CChapterState::OnKeyArrows( int nDeltaX, int nDeltaY )
{
	if ( pPickedMission || pPickedArrow )
	{
		CVec2 vScreenPos( vPrevPos.x + nDeltaX, vPrevPos.y + nDeltaY );

		if ( pPickedMission )
			MoveMission( pPickedMission, vScreenPos, true );
		else if ( pPickedArrow )
			MoveArrow( pPickedArrow, vScreenPos, true );
			
		vPrevPos = vScreenPos;

		UpdateFrontLines();

		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}

void CChapterState::UpdateView()
{
	pHelper->ReReadPotentials();

	UpdateFrontLines();

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


