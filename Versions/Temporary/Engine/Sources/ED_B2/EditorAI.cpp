#include "stdafx.h"

#include "TerraAIObserver.h"
#include "EditorAI.h"


ITerraAIObserver* CEditorAI::CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY )
{
	return new CTerraAIObserverInEditor( nAIMapSizeX, nAIMapSizeY );
}

// basement storage  


