#include "StdAfx.h"
#include "..\mapeditorlib\interface_commandhandler.h"
#include "..\ui\commandparam.h"
#include "..\MapEditorLib\ResourceDefines.h"
#include "..\MapEditorLib\CommandHandlerDefines.h"

//#include "EditorMethods.h"
#include "TextureEditor.h"
#include "TextureInterface.h"
#include "UIScene.h"
#include "UIWindow.h"

#include "..\MapEditorLib\CommonEditorMethods.h"
#include "..\3Dmotor\DBScene.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CTextureState::CTextureState( CTextureEditor *_pEditor ) 
: pEditor( _pEditor )
{
}


CTextureState::~CTextureState()
{
}


void CTextureState::Enter()
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_RUN_MODE, 15 );
	Singleton<IUIScene>()->Create();

	// get pointer to texture desc
	CDBPtr<NDb::STexture> pTexture = NDb::Get<NDb::STexture>( pEditor->GetObjectSet().objectNameSet.begin()->first );

	const int x = 0;
	const int y = 0;
	const int w = pTexture->nWidth;
	const int h = pTexture->nHeight;
	CUIWindow * pWindow = CreateUIWindow( x, y, w, h, 0xffffffff, pTexture );
	Singleton<IUIScene>()->AddWindow( pWindow );

	// setup UI
	//Singleton<IUIInitialization>()->SetOrigin( 0, 0 );
	//Singleton<IUIInitialization>()->SetResolution( 1024, 768 );

	// create run mode interface
	CTextureInterface * pInterface = new CTextureInterface();
	NMainLoop::Command( new CTexturelIC( pInterface ) );

	CDefaultInputState::Enter();
}


void CTextureState::Leave()
{
	CDefaultInputState::Leave();

	CObjectBase::Clear();

	Singleton<IUIScene>()->Clear();
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_RUN_MODE, 0 );
}


