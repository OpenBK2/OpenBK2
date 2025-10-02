#include "StdAfx.h"

#include "..\MapEditorLib\ResourceDefines.h"
#include "..\MapEditorLib\CommandHandlerDefines.h"
#include "..\MapEditorLib\CommonEditorMethods.h"

#include "WindowSimpleSharedEditor.h"
#include "UIRunModeState.h"
#include "UIRunModeInterface.h"
#include "UIScene.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CUIRunModeState::CUIRunModeState( CWindowSimpleSharedEditor *_pEditor, const string &rszTypeName, const CDBID &rDBID ) 
	: pEditor( _pEditor ), szTypeName( rszTypeName ), dbid( rDBID )
{
}


CUIRunModeState::~CUIRunModeState() 
{
}


void CUIRunModeState::Enter()
{
	DebugTrace( "CUIRunModeState::Enter()" );

	CWaitCursor wc;

	// clear the scene
	Singleton<IUIScene>()->Create();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_RUN_MODE, 15 );
	CUIRunModeInterface * pInterface = new CUIRunModeInterface( szTypeName,
																															dbid,
																															pEditor->editorSettings.templateWindowDBID,
																															pEditor->editorSettings.templateScreenDBID );
	NMainLoop::Command( new CUIRunModeIC( pInterface ) );

	CDefaultInputState::Enter();
}


void CUIRunModeState::Leave()
{
	DebugTrace( "CUIRunModeState::Leave()" );
	CDefaultInputState::Leave();

	// clear the scene
	Singleton<IUIScene>()->Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_RUN_MODE, 0 );
}


void CUIRunModeState::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch( nChar )
	{
	case VK_TAB:
		pEditor->PopRunModeState();
		break;
	default:
		break;
	}
	CDefaultInputState::OnKeyDown( nChar, nRepCnt, nFlags );
}


