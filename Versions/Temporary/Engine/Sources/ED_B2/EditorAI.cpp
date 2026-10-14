#include "stdafx.h"

#include "EditorAI.h"
#include "TerraAIObserver.h"

ITerraAIObserver *CEditorAI::CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY )
{
	return new CTerraAIObserverInEditor( nAIMapSizeX, nAIMapSizeY );
}

// basement storage
