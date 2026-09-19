#include "stdafx.h"

#include "HeightViewV3.h"
#include "ObjectProperties.h"
#include "ED_B2_M1Dll.h"

#include "MapEditorLib/Interface_Controller.h"
#include "MapEditorLib/Tools_HashSet.h"

// The part of the terrain height palette (HeightViewV3Wx.cpp) that is not
// drawing.

namespace NHeightViewV3
{
	void ShowTileProperties( const std::string &rszTileName )
	{
		// A set of one. The showing itself is NObjectProperties::Show, because
		// the map object and VSO palettes want the same thing and the block it
		// replaces was already written out three times.
		SObjectSet objectSet;
		objectSet.szObjectTypeName = TILE_TYPE_NAME;
		InsertHashSetElement( &( objectSet.objectNameSet ), rszTileName );
		NObjectProperties::Show( objectSet );
	}
}
