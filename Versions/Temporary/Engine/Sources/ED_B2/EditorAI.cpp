#include "StdAfx.h"

#include "terraaiobserver.h"
#include "EditorAI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ITerraAIObserver* CEditorAI::CreateTerraAIObserver( const int nAIMapSizeX, const int nAIMapSizeY )
{
	return new CTerraAIObserverInEditor( nAIMapSizeX, nAIMapSizeY );
}

// basement storage  


