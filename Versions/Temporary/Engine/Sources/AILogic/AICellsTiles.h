#pragma once

#include "AIInternalConsts.h"

//*******************************************************************
//*				  Tiles and Cells enumeration/converting handling					*
//*******************************************************************

namespace AICellsTiles
{
	// cell coordinates by the point coordinates
	inline const SVector GetCell( const int x, const int y )
	{ 
		SVector res;
		if ( x < 0 )
			res.x = 0;
		else
			res.x = x / SConsts::CELL_SIZE;
		if ( y < 0 )
			res.y = 0;
		else
			res.y = y / SConsts::CELL_SIZE;

		return res;
	}
	// big cell coordinates by the point coordinates
	inline const SVector GetBigCell( const int x, const int y )
	{ 
		SVector res;
		if ( x < 0 )
			res.x = 0;
		else
			res.x = x / SConsts::BIG_CELL_SIZE;
		if ( y < 0 )
			res.y = 0;
		else
			res.y = y / SConsts::BIG_CELL_SIZE;
		
		return res;
	}

	inline const SVector GetBigCell( const SVector &bigCell )
	{
		return GetBigCell( bigCell.x, bigCell.y );
	}

	// for general cells
	inline const SVector GetGeneralCell( const SVector &vPos )
	{
		return SVector( vPos.x / SConsts::GENERAL_CELL_SIZE,
									vPos.y / SConsts::GENERAL_CELL_SIZE );
	}
	inline const CVec2 GetCenterOfGeneralCell( const SVector &vPos )
	{
		return 		CVec2( vPos.x * SConsts::GENERAL_CELL_SIZE + SConsts::GENERAL_CELL_SIZE / 2,
												vPos.y * SConsts::GENERAL_CELL_SIZE + SConsts::GENERAL_CELL_SIZE / 2 );
	}
};

