#pragma once

#include "Misc/2Darray.h"

#include <cstdint>

struct STerrainAIInfo
{
	CArray2D<float> heights;
	CArray2D<uint8_t> terrTypes;
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &heights );
		saver.Add( 2, &terrTypes );
		return 0;
	}
};


