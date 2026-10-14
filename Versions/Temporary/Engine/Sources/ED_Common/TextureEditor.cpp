#include "stdafx.h"

#include "MapEditorLib/EditorFactory.h"
#include "TextureEditor.h"

REGISTER_EDITOR_IN_DLL( Texture, CTextureEditor )

CTextureEditor::CTextureEditor()
	: pState( 0 )
{
}

CTextureEditor::~CTextureEditor()
{
	Destroy();
}

void CTextureEditor::Create()
{
	ASSERT( pState == 0 );
	pState = new CTextureState( this );
}

void CTextureEditor::Destroy()
{
	delete pState;
	pState = 0;
}
