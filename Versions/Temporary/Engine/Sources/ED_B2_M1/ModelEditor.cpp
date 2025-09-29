#include "StdAfx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "CommandHandlerDefines.h"

#include "ED_B2_M1Dll.h"
#include "../Misc/HPtimer.h"

#include "../MapEditorLib/EditorFactory.h"

#include "ModelEditor.h"

#include "EditorScene.h"
#include "../SceneB2/Camera.h"

#include "../MapEditorLib/Interface_MainFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_EDITOR_IN_DLL( Model, CModelEditor )

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const UINT TOOLBAR_MODEL_ELEMENTS_ID[TOOLBAR_MODEL_ELEMENTS_COUNT] = 
{
	ID_MODEL_RELOAD_EDITOR,
	ID_SEPARATOR,
	ID_MODEL_DRAW_TERRAIN,
	ID_MODEL_DRAW_ANIMATIONS,
	ID_MODEL_DRAW_AI_GEOMETRY,
	ID_MODEL_SET_LIGHT,
	ID_SEPARATOR,
	ID_MODEL_CENTER_CAMERA,
	ID_MODEL_SAVE_CAMERA,
	ID_MODEL_RESET_CAMERA,
	ID_SEPARATOR,
	ID_MODEL_SPEED_DOWN,
	ID_MODEL_SPEED_UP,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CModelEditor::CModelEditor() : pModelState( 0 ), nModelToolbarID( 0xFFFFFFFF ), pwndTool( 0 ), bPreviousCameraHandleType( false ), fFOV( 26.0f )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MODEL_EDITOR, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MODEL_EDITOR, ID_MODEL_VIEW_TOOLBAR, ID_MODEL_VIEW_TOOL );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::CreateControls()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	// создаем minimap docking window
	UINT nID = ID_MODEL_EDITOR_DW;
	CString strPaneLabel;
	strPaneLabel.LoadString( theEDB2M1Instance, IDS_MODEL_TOOL_WINDOW_NAME  );
	if ( pwndTool = Singleton<IMainFrameContainer>()->Get()->CreateControlBar( &nID, strPaneLabel, CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_RIGHT, 0.5f, 265 ) )
	{
		AfxSetResourceHandle( theEDB2M1Instance );
		modelWindow.Create( CModelWindow::IDD, pwndTool );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndTool, &modelWindow );
		pwndTool->ShowWindow( SW_SHOW );
		modelWindow.ShowWindow( SW_SHOW );
	}
	//
	AfxSetResourceHandle( theEDB2M1Instance );
	CString strToolbarName;
	strToolbarName.LoadString( IDS_TOOLBAR_MODEL );
	Singleton<IMainFrameContainer>()->Get()->AddToolBarResource( IDT_MODEL, IDT_MODEL );
	Singleton<IMainFrameContainer>()->Get()->CreateToolBar( &nModelToolbarID,
																													strToolbarName,
																													TOOLBAR_MODEL_ELEMENTS_COUNT,
																													TOOLBAR_MODEL_ELEMENTS_ID,
 																													CBRS_ALIGN_ANY,
																													AFX_IDW_DOCKBAR_TOP,
																													true,
																													false,
																													false );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::PostCreateControls()
{
	if ( pwndTool != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndTool, false, true );
	}
	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::PreDestroyControls()
{
	if ( pwndTool != 0 )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndTool, false, true );
	}
	if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
	{
		Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, false, true );
	}

	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MODEL_EDITOR );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MODEL_EDITOR );
	Destroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::DestroyControls()
{
	// разрушаем shortcut docking window 
	if ( pwndTool != 0 )
	{
		if ( ::IsWindow( pwndTool->m_hWnd ) )
		{
			pwndTool->DestroyWindow();
		}
		delete pwndTool;
		pwndTool = 0;
	}
	modelWindow.DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::Create()
{
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CModelState::Enter(): pScene == 0" );

	bPreviousCameraHandleType = Singleton<ICamera>()->GetHandleType();
	fFOV = Singleton<ICamera>()->GetFOV();
	//
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	Singleton<ICamera>()->SetHandleType( true );
	if ( pModelState == 0 )
	{
		pModelState = new CModelState( this );
	}
	AfxSetResourceHandle( theEDB2M1Instance );
	Singleton<IMainFrameContainer>()->Get()->ShowMenu( IDM_MODEL );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	//
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CModelEditor::Destroy()
{
	if ( Singleton<IMainFrameContainer>() &&
			 Singleton<IMainFrameContainer>()->Get() &&
			 Singleton<IMainFrameContainer>()->GetSECWorkbook() )
	{
		AfxSetResourceHandle( theEDB2M1Instance );
		Singleton<IMainFrameContainer>()->Get()->ShowMenu( IDM_MAIN );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		//
		Singleton<ICamera>()->SetHandleType( bPreviousCameraHandleType );
		Singleton<ICamera>()->SetFOV( fFOV );

		if ( pModelState != 0 )
		{
			pModelState->UpdateTime( true );
			pModelState->UpdateSceneColor( true );
			delete pModelState;
			pModelState = 0;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CModelEditor::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID ) 
	{
		case ID_MODEL_VIEW_TOOLBAR:
		{
			if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pToolbar, !pToolbar->IsVisible(), true );
			}
			return true;
		}
		case ID_MODEL_VIEW_TOOL:
		{
			if ( pwndTool != 0 ) 
			{
				Singleton<IMainFrameContainer>()->GetSECWorkbook()->ShowControlBar( pwndTool, !pwndTool->IsVisible(), true );
			}
		}
		default:
			return false;
	} 
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CModelEditor::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CModelEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CModelEditor::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
		case ID_MODEL_VIEW_TOOLBAR:
		{
			if ( SECCustomToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
			{
				( *pbEnable ) = true;
				( *pbCheck ) = pToolbar->IsVisible();
			}
			else
			{
				( *pbEnable ) = false;
				( *pbCheck ) = false;
			}
			return true;
		}
		case ID_MODEL_VIEW_TOOL:
		{
			if ( pwndTool != 0 ) 
			{
				( *pbEnable ) = true;
				( *pbCheck ) = pwndTool->IsVisible();
			}
			else
			{
				( *pbEnable ) = false;
				( *pbCheck ) = false;
			}
			return true;
		}
		//
		default:
			return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
