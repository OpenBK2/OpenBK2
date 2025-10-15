#include "stdafx.h"

#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "DeadHouseAnimation.h"
#include "SceneB2/Scene.h"

#include "B2_M1_World_export.h"

#include <zconf.h>

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x12118C81, CDeadHouseAnimation );

void CDeadHouseAnimation::Init( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation )
{
	nID = nObjectID;
	nEndTime = time + pAnimation->nLength;
}

bool CDeadHouseAnimation::Update( const NTimer::STime &time )
{
	if ( time >= nEndTime )
	{
		if ( nID != -1 )
		{
			Scene()->RemoveObject( nID );
			nID = -1;
		}
		return false;
	}
	return true;
}


