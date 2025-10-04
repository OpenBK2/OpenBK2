#include "stdafx.h"

#include "MapEditorLib/EditorFactory.h"
#include "TerrainEditor.h"

//REGISTER_EDITOR_IN_DLL( TerrainDesc, CTerrainEditor )


CTerrainEditor::CTerrainEditor() : pTerrainState( 0 )
{
}


void CTerrainEditor::Create()
{
	if ( pTerrainState == 0 )
	{
		pTerrainState = new CTerrainState( this );
	}
}


void CTerrainEditor::Destroy()
{
	if ( pTerrainState )
	{
		delete pTerrainState;
		pTerrainState = 0;
	}
}

// basement storage  


