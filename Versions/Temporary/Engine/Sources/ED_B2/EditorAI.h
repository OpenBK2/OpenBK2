#if !defined(__EDITOR_AI__)
#define __EDITOR_AI__
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

#endif // !defined(__EDITOR_AI__)

