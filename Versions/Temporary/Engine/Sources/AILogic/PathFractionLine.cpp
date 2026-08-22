#include "stdafx.h"
#include "PathFractionLine.h"

#include "AILogic_export.h"

REGISTER_SAVELOAD_CLASS(AILOGIC, 0x11095C00, CPathFractionLine )



void CPathFractionLine::Init( const CVec2 &_x0, const CVec2 &_x1, const float _fZ )
{
	Init( CVec3(_x0,_fZ), CVec3(_x1,_fZ) );
}

void CPathFractionLine::Init( const CVec3 &_x0, const CVec3 &_x1 )
{
	x0 = _x0;
	x1 = _x1;
	v0 = x1 - x0;
	Normalize( &v0 );
	fLength = fabs( x0 - x1 );
}

