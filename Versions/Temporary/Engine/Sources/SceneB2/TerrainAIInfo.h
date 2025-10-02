#pragma once

#include "Misc/2DArray.h"

struct STerrainAIInfo
{
	CArray2D<float> heights;
	CArray2D<BYTE> terrTypes;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &heights );
		saver.Add( 2, &terrTypes );
		return 0;
	}
};


