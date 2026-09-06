#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "CommandHandlerDefines.h"

#include "ED_B2_M1Dll.h"
#include "Misc/HPTimer.h"

#include "MapEditorLib/EditorFactory.h"

#include "ModelEditor.h"

#include "EditorScene.h"
#include "SceneB2/Camera.h"

#include "MapEditorLib/Interface_MainFrame.h"

#include <cstdint>

#include <zconf.h>

#include "ED_B2_M1_export.h"

REGISTER_EDITOR_IN_DLL( Model, CModelEditor )


ED_B2_M1_EXPORT const unsigned TOOLBAR_MODEL_ELEMENTS_ID[TOOLBAR_MODEL_ELEMENTS_COUNT] = 
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


CModelEditor::CModelEditor() : pModelState( 0 ), nModelToolbarID( 0xFFFFFFFF ), pwndTool( 0 ), bPreviousCameraHandleType( false ), fFOV( 26.0f )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MODEL_EDITOR, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MODEL_EDITOR, ID_MODEL_VIEW_TOOLBAR, ID_MODEL_VIEW_TOOL );
}


void CModelEditor::ResetGUI( bool bActive )
{
    const CModelEditorSettings defaults;
    editorSettings.bShowTool = defaults.bShowTool;
    editorSettings.bShowToolbar = defaults.bShowToolbar;
    SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
    // The model and unit-stats views share controls but have separate profiles.
    // Patch both profiles so switching objects cannot bring back old visibility.
    for ( const char *profile : { "Model", "UnitStats" } )
    {
        CModelEditorSettings saved;
        if ( std::string( profile ) == "UnitStats" )
            saved.bDrawAnimations = saved.bDrawTerrain = false;
        pUserData->SerializeSettings( saved, profile, SUserData::EDITOR_SETTINGS, SUserData::ST_LOAD );
        saved.bShowTool = defaults.bShowTool;
        saved.bShowToolbar = defaults.bShowToolbar;
        pUserData->SerializeSettings( saved, profile, SUserData::EDITOR_SETTINGS, SUserData::ST_SAVE );
    }
    if ( pwndTool )
        pwndTool->Show( bActive && editorSettings.bShowTool );
    if ( auto *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
        pToolbar->Show( bActive && editorSettings.bShowToolbar );
}


void CModelEditor::CreateControls()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	// создаем minimap docking window
	unsigned nID = ID_MODEL_EDITOR_DW;
	CString strPaneLabel;
	strPaneLabel.LoadString( theEDB2M1Instance, IDS_MODEL_TOOL_WINDOW_NAME  );
	if ( pwndTool = Singleton<IMainFrameContainer>()->Get()->CreateControlBar( &nID, strPaneLabel.GetString(), CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_RIGHT, 0.5f, 265 ) )
	{
		AfxSetResourceHandle( theEDB2M1Instance );
		modelWindow.Create( CModelWindow::IDD, ToCWnd( pwndTool ) );
		AfxSetResourceHandle( AfxGetInstanceHandle() );
		CWndWidget contentsWidget( &modelWindow );
		Singleton<IMainFrameContainer>()->Get()->SetControlBarWindowContents( pwndTool, &contentsWidget );
		pwndTool->ShowWithoutLayout( true );
		modelWindow.ShowWindow( SW_SHOW );
	}
	//
	AfxSetResourceHandle( theEDB2M1Instance );
	CString strToolbarName;
	strToolbarName.LoadString( IDS_TOOLBAR_MODEL );
	Singleton<IMainFrameContainer>()->Get()->AddToolBarResource( IDT_MODEL, IDT_MODEL );
	Singleton<IMainFrameContainer>()->Get()->CreateToolBar( &nModelToolbarID,
																													strToolbarName.GetString(),
																													TOOLBAR_MODEL_ELEMENTS_COUNT,
																													TOOLBAR_MODEL_ELEMENTS_ID,
 																													CBRS_ALIGN_ANY,
																													AFX_IDW_DOCKBAR_TOP,
																													true,
																													false,
																													false );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
}


void CModelEditor::PostCreateControls()
{
	if ( pwndTool != 0 )
	{
		pwndTool->Show( false );
	}
	if ( IToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
	{
		pToolbar->Show( false );
	}
}


void CModelEditor::PreDestroyControls()
{
	if ( pwndTool != 0 )
	{
		pwndTool->Show( false );
	}
	if ( IToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
	{
		pToolbar->Show( false );
	}

	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MODEL_EDITOR );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MODEL_EDITOR );
	Destroy();
}


void CModelEditor::DestroyControls()
{
	// разрушаем shortcut docking window 
	if ( pwndTool != 0 )
	{
		if ( pwndTool->IsAlive() )
		{
			pwndTool->Destroy();
		}
		delete pwndTool;
		pwndTool = 0;
	}
	modelWindow.DestroyWindow();
}


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


void CModelEditor::Destroy()
{
	if ( Singleton<IMainFrameContainer>() &&
			 Singleton<IMainFrameContainer>()->Get() &&
			 MainFrameWnd() )
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


bool CModelEditor::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	switch( nCommandID ) 
	{
		case ID_MODEL_VIEW_TOOLBAR:
		{
			if ( IToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
			{
				pToolbar->Show( !pToolbar->IsVisible() );
			}
			return true;
		}
		case ID_MODEL_VIEW_TOOL:
		{
			if ( pwndTool != 0 ) 
			{
				pwndTool->Show( !pwndTool->IsVisible() );
			}
		}
		default:
			return false;
	} 
	return false;
}

bool CModelEditor::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CModelEditor::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CModelEditor::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
		case ID_MODEL_VIEW_TOOLBAR:
		{
			if ( IToolBar *pToolbar = Singleton<IMainFrameContainer>()->Get()->GetToolBar( nModelToolbarID ) )
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


// basement storage  


