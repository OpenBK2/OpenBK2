#include "StdAfx.h"
#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\ui\ui.h"
#include "..\input\gamemessage.h"
#include "..\ui\uifactory.h"
#include "UIScene.h"

#include "..\MapEditorLib\CommonEditorMethods.h"
#include "..\MapEditorLib\ResourceDefines.h"
#include "..\MapEditorLib\CommandHandlerDefines.h"
#include "..\MapEditorLib\StringManager.h"
#include "..\MapEditorLib\Tools_HashSet.h"
#include "../libdb/ResourceManager.h"
#include "..\libdb\EditorDB.h"

#include "WindowSimpleSharedEditor.h"
#include "WindowSimpleSharedState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



namespace 
{
	const char WINDOW_SIMPLE_SHARED_TYPE_NAME[] = "WindowSimpleShared";
	const struct 
	{
		const char * szInstanceName;
		const char * szSharedName;
	} tblChildTypes[] = 
	{
		"WindowSimple",				"WindowSimpleShared",
		"WindowMSButton",			"WindowMSButtonShared",
		"WindowTabControl",		"WindowTabControlShared",
		//"WindowEditLine",     "WindowEditLineShared",
		"WindowScrollBar",    "WindowScrollBarShared",
		//"WindowSlider",       "WindowSliderShared",
		"WindowTextView",     "WindowTextViewShared",
		// H5-specific
		"Window1LineList",		"Window1LineListShared",
		"WindowStaticText",		"WindowStaticTextShared",
		//
		0, 0
	};
	const char * tblPushableTypes[] =
	{
		"WindowSimpleShared",
		//"WindowMSButtonShared",
		0
	};
};


CWindowSimpleSharedState::CWindowSimpleSharedState( CWindowSimpleSharedEditor *_pEditor ) 
	: pEditor( _pEditor ), pScreen( 0 ), pMainWindow( 0 ), pPickedWindow( 0 ), bDragging( false )
{
}


void CWindowSimpleSharedState::Enter()
{
	DebugTrace( "CWindowSimpleSharedState::Enter()" );
	NI_ASSERT( 1 == pEditor->GetObjectSet().objectNameSet.size(), "CWindowSimpleSharedState::Enter(): Only single selection supported" );

	CWaitCursor wc;

	ResetSelection();

	// clear the scene
	Singleton<IUIScene>()->Create();

	LoadWindow();
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_SCROLLBARS, 1 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::Enter();
}


void CWindowSimpleSharedState::Leave()
{
	DebugTrace( "CWindowSimpleSharedState::Leave()" );
	CDefaultInputState::Leave();

	CWaitCursor wc;

	ResetSelection();

	pScreen = 0;
	pMainWindow = 0;
	
	// clear the scene
	Singleton<IUIScene>()->Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_SCROLLBARS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CWindowSimpleSharedState::LoadWindow()
{
	NI_ASSERT( Singleton<IUIScene>() != 0, "CWindowSimpleSharedEditor::LoadWindow: Singleton<IUIScene>() == 0" );

	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedEditor::LoadWindow: pTableManipulator == 0" );

	szEditorTypeName = pEditor->GetObjectSet().szObjectTypeName;
	editorDBID = pEditor->GetObjectSet().objectNameSet.begin()->first;

	// direct ID to db templates
	NI_ASSERT( !pEditor->editorSettings.templateWindowDBID.IsEmpty(), "CWindowSimpleSharedEditor::LoadWindow: windowSimpleDBID is empty" );
	NI_ASSERT( !pEditor->editorSettings.templateScreenDBID.IsEmpty(), "CWindowSimpleSharedEditor::LoadWindow: windowScreenDBID is empty" );

	// get pointer to selected element
	const NDb::CResource *pSelected = NDb::Get<NDb::CResource>( editorDBID );
	NI_ASSERT( pSelected != 0, "CWindowSimpleSharedEditor::LoadWindow: pSelected == 0" );
	
	if ( const NDb::SWindowScreenShared *pShared = dynamic_cast<const NDb::SWindowScreenShared*>( pSelected ) )
	{
		// handle WindowScreenShared
		
		// get pointer to WindowScreen template
		CDBPtr<NDb::SWindowScreen> pTemplate = NDb::Get<NDb::SWindowScreen>( pEditor->editorSettings.templateScreenDBID );
		NI_ASSERT( pTemplate != 0, "CWindowSimpleSharedEditor::LoadWindow: pTemplate == 0" );

		// duplicate WindowScreen template to set off "read-only"
		CPtr<NDb::SWindowScreen> pInstance = pTemplate->Duplicate();
		NI_ASSERT( pInstance != 0, "CWindowSimpleSharedEditor::LoadWindow: pInstance == 0" );

		// connect selected WindowSimpleShared object WindowSimple template
		pInstance->pShared = pShared;

		MakeUIScreenWithElement( pInstance );
	}
	else if ( const NDb::SWindowSimpleShared *pShared = dynamic_cast<const NDb::SWindowSimpleShared*>( pSelected ) )
	{
		// handle WindowSimpleShared

		// get pointer to WindowSimple template
		CDBPtr<NDb::SWindowSimple> pTemplate = NDb::Get<NDb::SWindowSimple>( pEditor->editorSettings.templateWindowDBID );
		NI_ASSERT( pTemplate != 0, "CWindowSimpleSharedEditor::LoadWindow: pTemplate == 0" );

		// duplicate WindowSimple template to set off "read-only"
		CPtr<NDb::SWindowSimple> pInstance = pTemplate->Duplicate();
		NI_ASSERT( pInstance != 0, "CWindowSimpleSharedEditor::LoadWindow: pInstance == 0" );

		// connect selected WindowSimpleShared object WindowSimple template
		pInstance->pShared = pShared;

		MakeUIScreenWithElement( pInstance );
	}
	else if ( const NDb::SWindow *pInstance = dynamic_cast<const NDb::SWindow*>( pSelected ) )
	{
		// Handle any Window instance

		MakeUIScreenWithElement( pInstance );
	}
	else
	{
		NI_ASSERT( 0, "CWindowSimpleSharedState::LoadWindow: Unknown widget type" );
	}

	// add screen to scene
	Singleton<IUIScene>()->AddWindow( pScreen );

	// avoid fitting UI to viewport size
	Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
}

void CWindowSimpleSharedState::MakeUIScreenWithElement( const struct NDb::SUIDesc *pElement )
{
	if ( pElement == 0 )
		return;
		
	if ( dynamic_cast<const NDb::SWindowScreen*>( pElement ) )
	{
		// create custom screen
		pScreen = Singleton<IUIInitialization>()->CreateScreenFromDesc( pElement, 0, 0, Singleton<IUIScene>()->GetG2DView(), 0, 0 );
		NI_ASSERT( pScreen != 0, "CWindowSimpleSharedEditor::LoadWindow: pScreen == 0" );

		pMainWindow = pScreen;
	}
	else
	{
		// get ID to template screen
		NI_ASSERT( !pEditor->editorSettings.templateScreenDBID.IsEmpty(), "CWindowSimpleSharedEditor::LoadWindow: windowScreenDBID is empty" );

		// get pointer to WindowScreen
		CDBPtr<NDb::SWindowScreen> pScreenDesc = NDb::Get<NDb::SWindowScreen>( pEditor->editorSettings.templateScreenDBID );
		NI_ASSERT( pScreenDesc != 0, "CWindowSimpleSharedEditor::LoadWindow: pScreenDesc == 0" );

		// create template screen
		pScreen = Singleton<IUIInitialization>()->CreateScreenFromDesc( pScreenDesc, 0, 0, Singleton<IUIScene>()->GetG2DView(), 0, 0 );
		NI_ASSERT( pScreen != 0, "CWindowSimpleSharedEditor::LoadWindow: pScreen == 0" );

		// avoid fitting UI to viewport size
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( 1024, 768 );
		
		// create UI window based on WindowSimple template
		pMainWindow = Singleton<IUIInitialization>()->CreateWindowFromDesc( pElement );
		NI_ASSERT( pMainWindow != 0, "CWindowSimpleSharedEditor::LoadWindow: pMainWindow == 0" );

		// add it to the screen
		pScreen->AddChild( pMainWindow, true );
	}
}

void CWindowSimpleSharedState::ResetSelection()
{
	pPickedWindow = 0;
	bDragging = false;
}


void CWindowSimpleSharedState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	CWaitCursor wc;
	switch( nChar )
	{
	case VK_DELETE:
		OnKeyDelete();
		break;
	case VK_RETURN:
		OnKeyEnter();
		break;
	case VK_BACK:
		OnKeyBack();
		break;
	case VK_TAB:
		OnKeyTab();
		break;
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


void CWindowSimpleSharedState::OnKeyArrows( int dx, int dy )
{
	if ( pPickedWindow != 0 )
	{
		ReplaceChild( pPickedWindow, CTPoint<int>(dx, dy), RCH_DELTA | RCH_X | RCH_Y );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
}


void CWindowSimpleSharedState::OnKeyTab()
{
	pEditor->PushRunModeState( szEditorTypeName, editorDBID );
}


void CWindowSimpleSharedState::OnKeyDelete()
{
	IWindow *pWindow = pPickedWindow; //BUGFIX: RemoveChild redraws screen
	ResetSelection();
	RemoveChild( pWindow );
	UpdatePropertyControl( true );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CWindowSimpleSharedState::OnKeyEnter()
{
	if ( pPickedWindow != 0 )
	{
		const CDBID sharedDBID = pPickedWindow->GetSharedDesc()->GetDBID();
		const string szSharedTypeName = NDb::GetClassTypeName( sharedDBID );
		//
		if ( IsPushableType( szSharedTypeName ) )
		{
			SObjectSet objectSet;
			objectSet.szObjectTypeName = szSharedTypeName;
			InsertHashSetElement( &( objectSet.objectNameSet ), sharedDBID );

			pEditor->PushState( objectSet, new CWindowSimpleSharedState( pEditor ) );
		}
	}
}


void CWindowSimpleSharedState::OnKeyBack()
{
	if ( pEditor->HasMoreThanOnePushedStates() )
		pEditor->PopState();
}


void CWindowSimpleSharedState::OnLButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	//CVec2 vScreenPos;
	//Singleton<IUIInitialization>()->GetVirtualScreenController()->ScreenToVirtual( CVec2(rMousePoint.x, rMousePoint.y), &vScreenPos );
	pPickedWindow = pMainWindow->Pick( CVec2(rMousePoint.x, rMousePoint.y), false );

	if ( pPickedWindow != 0 )
	{
		rLastPoint  = rMousePoint;
		rStartPoint = rMousePoint;
		bDragging = true;
	}
	else if ( (nFlags & (MK_CONTROL | MK_SHIFT)) == (MK_CONTROL | MK_SHIFT) )
	{
		CWaitCursor wc;
		InsertChild( rMousePoint );
		UpdatePropertyControl( true );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::OnLButtonDown( nFlags, rMousePoint );
}


void CWindowSimpleSharedState::OnLButtonUp( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bDragging )
	{
		const CTPoint<int> deltaPoint( rMousePoint - rStartPoint );
		if ( 0 != deltaPoint.x || 0 != deltaPoint.y )
		{
			CWaitCursor wc;
			ReplaceChild( pPickedWindow, deltaPoint, RCH_DELTA | RCH_X | RCH_Y );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
		else
		{
			CWaitCursor wc;
			UpdatePropertyControl();
		}
		bDragging = false;
	}
	CDefaultInputState::OnLButtonUp( nFlags, rMousePoint );
}

void CWindowSimpleSharedState::OnMouseMove( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	if ( bDragging )
	{
		const CTPoint<int> deltaPoint( rMousePoint - rLastPoint );
		if ( 0 != deltaPoint.x || 0 != deltaPoint.y )
		{
			int x, y;
			pPickedWindow->GetPlacement( &x, &y, 0, 0);
			pPickedWindow->SetPlacement( x+deltaPoint.x, y+deltaPoint.y, 0, 0, EWPF_POS_X |  EWPF_POS_Y );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			rLastPoint = rMousePoint;
		}
	}
	CDefaultInputState::OnMouseMove( nFlags, rMousePoint );
}


void CWindowSimpleSharedState::OnRButtonDown( UINT nFlags, const CTPoint<int> &rMousePoint )
{
	CWaitCursor wc;

	// reset selection
	ResetSelection();
	UpdatePropertyControl();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );

	CDefaultInputState::OnRButtonDown( nFlags, rMousePoint );
}


void CWindowSimpleSharedState::UpdatePropertyControl( bool bHardUpdate )
{
	CDBID sharedDBID;
	string szSharedTypeName;
	if ( pMainWindow && pMainWindow->GetSharedDesc() )
	{
		sharedDBID = pMainWindow->GetSharedDesc()->GetDBID();
		szSharedTypeName = NDb::GetClassTypeName( sharedDBID );
	}
	if ( pPickedWindow && pPickedWindow->GetDesc() )
	{
		sharedDBID = pPickedWindow->GetDesc()->GetDBID();
		szSharedTypeName = NDb::GetClassTypeName( sharedDBID );
	}

	IView *pView = 0;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<DWORD>( &pView ) );
	if ( pView != 0 )
	{
		SObjectSet objectSet;
		objectSet.szObjectTypeName = szSharedTypeName;
		InsertHashSetElement( &( objectSet.objectNameSet ), sharedDBID );

		CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szSharedTypeName, sharedDBID );
		NI_ASSERT( pManipulator != 0, "CWindowSimpleSharedEditor::UpdatePropertyControl: pManipulator == 0" );

		if ( pView->GetViewManipulator() != pManipulator )
		{
			pView->SetViewManipulator( pManipulator, objectSet, string() );
			bHardUpdate = true;
		}
		DWORD dwCommand = bHardUpdate ? ID_PC_DIALOG_CREATE_TREE : ID_PC_DIALOG_UPDATE_VALUES;
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, dwCommand, 0 );
	}
}


void CWindowSimpleSharedState::PostDraw( CPaintDC *pPaintDC )
{
	CTRect<float> wrc;
	int nOldBkMode = pPaintDC->SetBkMode( TRANSPARENT );

	// draw virtual screen borders
	{
		wrc.SetRect(0, 0, 1024, 768);
		Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( wrc, &wrc );

		// draw screen rect
		CPen pen(PS_DOT, 1, RGB(144,144,144));
		CPen * pOldPen = pPaintDC->SelectObject( &pen );
		pPaintDC->MoveTo( wrc.left,  wrc.top );
		pPaintDC->LineTo( wrc.right, wrc.top );
		pPaintDC->LineTo( wrc.right, wrc.bottom );
		pPaintDC->LineTo( wrc.left,  wrc.bottom );
		pPaintDC->LineTo( wrc.left,  wrc.top );
		pPaintDC->SelectObject( pOldPen );
	}

	if ( pMainWindow != 0 )
	{
		pMainWindow->FillWindowRect( &wrc );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( wrc, &wrc );

		// draw screen rect
		CPen pen(PS_DOT, 1, RGB(224,224,224));
		CPen * pOldPen = pPaintDC->SelectObject( &pen );
		pPaintDC->MoveTo( wrc.left,  wrc.top );
		pPaintDC->LineTo( wrc.right, wrc.top );
		pPaintDC->LineTo( wrc.right, wrc.bottom );
		pPaintDC->LineTo( wrc.left,  wrc.bottom );
		pPaintDC->LineTo( wrc.left,  wrc.top );
		pPaintDC->SelectObject( pOldPen );
	}

	if ( pPickedWindow != 0 )
	{
		pPickedWindow->FillWindowRect( &wrc );
		Singleton<IUIInitialization>()->GetVirtualScreenController()->VirtualToScreen( wrc, &wrc );

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

		pPaintDC->SetROP2( nOldROP2 );
	}
	pPaintDC->SetBkMode( nOldBkMode );
	CDefaultInputState::PostDraw( pPaintDC );
}


void CWindowSimpleSharedState::RemoveChild( IWindow *pWindow )
{
	NI_ASSERT( pWindow != 0, "CWindowSimpleSharedEditor::RemoveChild: pWindow != 0" );
	NI_ASSERT( pMainWindow == pWindow->GetParentWindow(), "CWindowSimpleSharedEditor::RemoveChild: pMainWindow == pWindow->GetParentWindow()" );

	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedEditor::RemoveChild: pTableManipulator == 0" );

	string typeName = NDb::GetClassTypeName( pWindow->GetDesc()->GetDBID() );

	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( typeName );
	NI_ASSERT( pFolderManipulator != 0, "CWindowSimpleSharedEditor::IsUniqueChild: pFolderManipulator == 0" );

	string objName = pWindow->GetDesc()->GetDBID().ToString();

	string fullNameOfPicked;
	CStringManager::GetRefValueFromTypeAndName( &fullNameOfPicked, typeName, objName, TYPE_SEPARATOR_CHAR );

	CVariant value;
	pEditor->GetViewManipulator()->GetValue( "Children", &value );
	const int nChildren = (int)value;

	for ( int i = 0; i < nChildren; ++i )
	{
		const string itemName = StrFmt( "Children.[%d]", i );
		if ( pEditor->GetViewManipulator()->GetValue( itemName, &value ) )
		{
			const string fullName = value.GetStr();

			if ( fullName == fullNameOfPicked )
			{
				pMainWindow->RemoveChild( pWindow );
				if ( pEditor->UOBegin( pEditor->GetViewManipulator(), szEditorTypeName, editorDBID ) )
				{
					pEditor->UORemoveNode( "Children", i, szEditorTypeName, editorDBID );
					pEditor->UOEnd();
					pEditor->RemoveObject( pFolderManipulator, objName );
				}
				break;
			}
		}
	}
}


void CWindowSimpleSharedState::ReplaceChild( IWindow *pWindow, const CTPoint<int> &rPoint, int nFlags )
{
	NI_ASSERT( pWindow != 0, "CWindowSimpleSharedEditor::ReplaceChild: pWindow != 0" );
	NI_ASSERT( pMainWindow == pWindow->GetParentWindow(), "CWindowSimpleSharedEditor::ReplaceChild: pMainWindow == pWindow->GetParentWindow()" );

	if ( const NDb::SUIDesc *pInstanceDesc = pWindow->GetDesc() )
	{
		CDBID instanceDBID = pInstanceDesc->GetDBID();
		string szInstanceTypeName = NDb::GetClassTypeName( instanceDBID );
		if ( CPtr<IManipulator> pInstanceManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szInstanceTypeName, instanceDBID ) )
		{
			CVariant value;

			CTPoint<int> newPos;
			if ( nFlags & RCH_DELTA )
			{
				CTPoint<int> orgPos( 0, 0 );
				pInstanceManipulator->GetValue( "Placement.Position.First.x", &value );
				orgPos.x = (int)value;
				pInstanceManipulator->GetValue( "Placement.Position.First.y", &value );
				orgPos.y = (int)value;
				newPos = orgPos + rPoint;
			}
			else
			{
				CVec2 vPos;
				Singleton<IUIInitialization>()->GetVirtualScreenController()->ScreenToVirtual( CVec2(rPoint.x, rPoint.y), &vPos );
				CTRect<float> rcParent;
				pMainWindow->FillWindowRect( &rcParent );
				newPos.Set( vPos.x-(int)rcParent.left, vPos.y-(int)rcParent.top );
			}
			if ( nFlags & RCH_X )
			{
				pWindow->SetPlacement( newPos.x, 0, 0, 0, EWPF_POS_X );
			}
			if ( nFlags & RCH_Y )
			{
				pWindow->SetPlacement( 0, newPos.y, 0, 0, EWPF_POS_Y );
			}
			if ( pEditor->UOBegin( pInstanceManipulator, szInstanceTypeName, instanceDBID ) )
			{
				// to sure the instance position is used
				pInstanceManipulator->GetValue( "Placement.Position.Second", &value );
				if ( false == (bool)value )
				{
					pEditor->UOSetValue( "Placement.Position.Second", CVariant( true ) );
				}
				// update position
				if ( nFlags & RCH_X )
				{
					pEditor->UOSetValue( "Placement.Position.First.x", CVariant( (float)(newPos.x) ) );
				}
				if ( nFlags & RCH_Y )
				{
					pEditor->UOSetValue( "Placement.Position.First.y", CVariant( (float)(newPos.y) ) );
				}
				pEditor->UOEnd();
			}
		}
	}
}


void CWindowSimpleSharedState::InsertChild( const CTPoint<int> &rMousePoint )
{
	SObjectSet objectSet;
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_OBJECTSET, reinterpret_cast<DWORD>( &objectSet ) ) )
		return;

	if ( 1 != objectSet.objectNameSet.size() )
	{
		//MessageBox( 0, "To insert select only one child window", "WindowSimpleSharedEditor", MB_OK );
		return;
	}

	if ( CheckInsertChild( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
	{
		string instanceTypeName;
		FindInstanceTypeNameByShared( objectSet.szObjectTypeName, &instanceTypeName );

		CDBID instanceDBID;
		if ( InsertChildInstanceToDB( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first, &instanceDBID ) )
		{
			IWindow * pChildWindow = InsertChildInstanceToUI( instanceTypeName, instanceDBID );
			ReplaceChild( pChildWindow, rMousePoint, RCH_X | RCH_Y );

			pPickedWindow = pChildWindow;
		}
	}
}


bool CWindowSimpleSharedState::CheckInsertChild( const string & szTypeName, const CDBID &rDBID )
{
	string instanceTypeName;
	if ( !FindInstanceTypeNameByShared( szTypeName, &instanceTypeName ) )
	{
		// selected item cannot be accepted as child window
		return false;
	}
	if ( ( szTypeName == WINDOW_SIMPLE_SHARED_TYPE_NAME ) &&
			 ( pMainWindow->GetSharedDesc()->GetDBID() == rDBID ) )
	{
		// choosen child is main window
		return false;
	}

	return true;
}


bool CWindowSimpleSharedState::FindInstanceTypeNameByShared( const string & szSharedName, string * szInstanceName )
{
	for ( int i = 0; 0 != tblChildTypes[i].szSharedName; ++i )
	{
		if ( szSharedName == tblChildTypes[i].szSharedName )
		{
			*szInstanceName = tblChildTypes[i].szInstanceName;
			return true;
		}
	}
	return false;
}


bool CWindowSimpleSharedState::IsPushableType( const string & szTypeName )
{
	for ( int i = 0; 0 != tblPushableTypes[i]; ++i )
	{
		if ( szTypeName == tblPushableTypes[i] )
		{
			return true;
		}
	}
	return false;
}



bool CWindowSimpleSharedState::InsertChildInstanceToDB( const string & szSharedTypeName, const CDBID &rSharedDBID, CDBID *pInstanceDBID )
{
	string instanceTypeName;
	if ( !FindInstanceTypeNameByShared( szSharedTypeName, &instanceTypeName ) )
		return false;

	string instanceFullName;
	if ( !GenerateChildInstance( szSharedTypeName, rSharedDBID, &instanceFullName, pInstanceDBID ) )
		return false;

	CVariant value;
	if ( !pEditor->GetViewManipulator()->GetValue( "Children", &value ) )
		return 0;

	const int nChildIndex = (int)value;

	//TODO: join "insert" and "set value" to single undo-op
	bool bResult = true;
	if ( pEditor->UOBegin( pEditor->GetViewManipulator(), szEditorTypeName, editorDBID ) )
	{
		bResult = bResult && pEditor->UOInsertNode( "Children", instanceTypeName, *pInstanceDBID );
		bResult = bResult && pEditor->UOEnd();
	}

	if ( pEditor->UOBegin( pEditor->GetViewManipulator(), szEditorTypeName, editorDBID ) )
	{
		bResult = bResult && pEditor->UOSetValue( StrFmt( "Children.[%d]", nChildIndex ), CVariant(instanceFullName) );
		bResult = bResult && pEditor->UOEnd();
	}

	return bResult;
}


IWindow * CWindowSimpleSharedState::InsertChildInstanceToUI( const string & szInstanceTypeName, const CDBID &rInstanceDBID )
{
	CPtr<IWindow> pChildWindow;

	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedEditor::InsertChildInstanceToUI: pTableManipulator == 0" );

	//const int nTypeID = pTableManipulator->GetID( szInstanceTypeName );

	//BUGFIX{ : reset db cache to properly run GetResource
	//Singleton<IResourceManager>()->ResetCache();
	//BUGFIX}

	CDBPtr<NDb::CResource> pWindowRes = NDb::Get<NDb::CResource>( rInstanceDBID );
	if ( pWindowRes )
	{
		// create UI window based on WindowSimple template
		pChildWindow = Singleton<IUIInitialization>()->CreateWindowFromDesc( static_cast<const NDb::SUIDesc*>(pWindowRes.GetPtr()) );
		NI_ASSERT( pChildWindow != 0, "CWindowSimpleSharedEditor::InsertChildInstanceToUI: pChildWindow == 0" );

		// add created child to the main window
		pMainWindow->AddChild( pChildWindow, true );
	}
	
	//BUGFIX{ : reset db cache to properly flush result of GetResource
	//Singleton<IResourceManager>()->ResetCache();
	//BUGFIX}

	return pChildWindow;
}


bool CWindowSimpleSharedState::GetEditorObjName( string *pObjName )
{
	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( WINDOW_SIMPLE_SHARED_TYPE_NAME );
	NI_ASSERT( pFolderManipulator != 0, "CWindowSimpleSharedEditor::GenerateChildInstanceName: pFolderManipulator == 0" );

	*pObjName = pMainWindow->GetSharedDesc()->GetDBID().ToString();
	return true;
}


bool CWindowSimpleSharedState::MakeInstanceName( const string & szInstanceTypeName, const string & szSharedShortName, string *pShortName, string *pFullName, string *pObjName )
{
	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( szInstanceTypeName );
	NI_ASSERT( pFolderManipulator != 0, "CWindowSimpleSharedEditor::GenerateChildInstanceName: pFolderManipulator == 0" );

	string editorObjName;
	if ( !GetEditorObjName( &editorObjName ) )
		return false;

	int index = 0;
	string shortName;
	string objName;

	do
	{
		shortName = (index == 0) ? szSharedShortName : StrFmt( "%s_%d", szSharedShortName.c_str(), index );
		objName = editorObjName + PATH_SEPARATOR_CHAR + shortName;
		++index;
	} while ( pFolderManipulator->GetID( objName ) != INVALID_NODE_ID );

	string refValue;
	CStringManager::GetRefValueFromTypeAndName( &refValue, szInstanceTypeName, objName, TYPE_SEPARATOR_CHAR );

	*pFullName  = refValue;
	*pShortName = shortName;
	*pObjName   = objName;

	return true;
}


bool CWindowSimpleSharedState::MakeSharedName( const string & szSharedTypeName, const CDBID &rDBID, string *pSharedShortName, string *pSharedFullName )
{
	string objName = rDBID.ToString();
	CStringManager::GetRefValueFromTypeAndName( pSharedFullName, szSharedTypeName, objName, TYPE_SEPARATOR_CHAR );

	const slashPos = objName.rfind( PATH_SEPARATOR_CHAR );
	*pSharedShortName = (slashPos == string::npos) ? objName : objName.substr( slashPos+1 );

	return true;
}


bool CWindowSimpleSharedState::GenerateChildInstance( const string & szSharedTypeName, const CDBID &rSharedDBID, string *szInstanceFullName, CDBID *pInstanceDBID )
{
	string instanceTypeName;
	if ( !FindInstanceTypeNameByShared( szSharedTypeName, &instanceTypeName ) )
		return false;

	string sharedShortName, sharedFullName;
	if ( !MakeSharedName( szSharedTypeName, rSharedDBID, &sharedShortName, &sharedFullName ) )
		return false;

	string shortName, fullName, objName;
	if ( !MakeInstanceName( instanceTypeName, sharedShortName, &shortName, &fullName, &objName ) )
		return false;

	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( instanceTypeName );
	NI_ASSERT( pFolderManipulator != 0, "CWindowSimpleSharedEditor::GenerateChildInstance: pFolderManipulator == 0" );

	if ( !pEditor->InsertObject( pFolderManipulator, objName ) )
		return false;

	const CDBID instanceDBID( objName );

	CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( instanceTypeName, instanceDBID );
	NI_ASSERT( pManipulator != 0, "CWindowSimpleSharedEditor::GenerateChildInstance: pManipulator == 0" );

	CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
	NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedState::GenerateChildInstance: pTableManipulator == 0" );

	const int nInstanceTypeID = pTableManipulator->GetID( instanceTypeName );

	bool bResult = true;

	//if ( pEditor->UOBegin( pManipulator, nInstanceTypeID, nInstanceID ) )
	{
		bResult = bResult && pManipulator->SetValue( "Shared", CVariant(sharedFullName) );
		bResult = bResult && pManipulator->SetValue( "Name", CVariant(shortName) );
		bResult = bResult && pManipulator->SetValue( "Placement.Position.Second", CVariant(true) );
		//bResult = bResult && pEditor->UOSetValue( "Shared", sharedFullName );
		//bResult = bResult && pEditor->UOSetValue( "Name", shortName );
		//bResult = bResult && pEditor->UOSetValue( "Placement.Position.Second", CVariant(true) );

		CPtr<IManipulator> pSharedManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szSharedTypeName, rSharedDBID );
		NI_ASSERT( pSharedManipulator != 0, "CWindowSimpleSharedEditor::GenerateChildInstance: pSharedManipulator == 0" );

		CVariant value;
		if ( pSharedManipulator->GetValue( "VisualStates", &value ) )
		{
			// insert button states = amount of visual states in shared
			const int nVisualStates = (int)value;
			for ( int i = 0; i < nVisualStates; ++i )
			{
				//bResult = bResult && pEditor->UOInsertNode( "ButtonStates", string(), 0 );
				bResult = bResult && pManipulator->InsertNode( "ButtonStates" );
			}
		}
		//bResult = bResult && pEditor->UOEnd();
	}

	*szInstanceFullName = fullName;
	*pInstanceDBID = instanceDBID;

	return bResult;
}


void CWindowSimpleSharedState::UndoChange( const string & szTypeName, const CDBID &rDBID, const string & szName, const CVariant & oldValue )
{
	/**
	if ( string::npos != szName.find( "Placement.Position.First." ) )
	{
		CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
		NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedState::UndoChange: pTableManipulator == 0" );
		if ( CPtr<IWindow> pChild = pMainWindow->GetChild( szTypeName, rDBID, false ) )
		{
			const int nValue = (int)(float)oldValue;
			if ( string::npos != szName.find( ".x" ) )
			{
				pChild->SetPlacement( nValue, 0, 0, 0, EWPF_POS_X );
			}
			else if ( string::npos != szName.find( ".y" ) )
			{
				pChild->SetPlacement( 0, nValue, 0, 0, EWPF_POS_Y );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
	/**/
}


void CWindowSimpleSharedState::UndoInsert( const string & szTypeName, const CDBID &rDBID, const string & szName )
{
	/**
	if ( string::npos != szName.find( "Children" ) )
	{
		CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
		NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedState::UndoInsert: pTableManipulator == 0" );
		if ( CPtr<IWindow> pChild = pMainWindow->GetChild( szTypeName, rDBID, false ) )
		{
			if ( pChild == pPickedWindow )
				pPickedWindow = 0;
			pMainWindow->RemoveChild( pChild );
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		}
	}
	/**/
}


void CWindowSimpleSharedState::UndoRemove( const string & szTypeName, const CDBID &rDBID, const string & szName )
{
	/**
	if ( string::npos != szName.find( "Children" ) )
	{
		CPtr<IManipulator> pTableManipulator = Singleton<IResourceManager>()->CreateTableManipulator();
		NI_ASSERT( pTableManipulator != 0, "CWindowSimpleSharedState::UndoRemove: pTableManipulator == 0" );

		InsertChildInstanceToUI( szTypeName, rDBID );

		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	}
	/**/
}



