#pragma once

#include "../MapEditorLib/Interface_EditorAI.h"

class  CEditorAI : public IEditorAI
{
	OBJECT_NOCOPY_METHODS( CEditorAI );
	//
public:
	// IEditorAI
	interface ITerraAIObserver* CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY );
};



