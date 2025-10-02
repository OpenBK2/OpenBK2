#include "StdAfx.h"
#include "TextureEditor.h"

#include "../MapEditorLib/EditorFactory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EDITOR_IN_DLL( Texture, CTextureEditor )


CTextureEditor::CTextureEditor() : pState( 0 )
{
}


CTextureEditor::~CTextureEditor()
{
	Destroy();
}


void CTextureEditor::Create()
{
	ASSERT ( pState == 0 );
	pState = new CTextureState( this );
}


void CTextureEditor::Destroy()
{
	delete pState;
	pState = 0;
}



