#pragma once

#include "misc/2darray.h"
#include "DebugTools/DebugInfoManager.h"
#include "Common_RTS_AI/TerraAIObserver.h"


class CAIMap;

class CTerraAIObserverInEditor : public CTerraAIObserver
{
	OBJECT_NOCOPY_METHODS( CTerraAIObserverInEditor )

	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

public:
	CTerraAIObserverInEditor() { NI_ASSERT( 0,  "Default constructor is not allowed!" ); }
	CTerraAIObserverInEditor( const int nAIMapSizeX, const int nAIMapSizeY );
	virtual ~CTerraAIObserverInEditor() {}
};


