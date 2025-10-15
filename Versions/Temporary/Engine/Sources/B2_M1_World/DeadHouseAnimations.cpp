#include "stdafx.h"

#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "DeadHouseAnimations.h"
#include "SceneB2/Scene.h"

#include "B2_M1_World_export.h"

#include <zconf.h>

#include <algorithm>

REGISTER_SAVELOAD_CLASS( B2_M1_WORLD, 0x3116C300, CDeadHouseAnimations );

void CDeadHouseAnimations::Add( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation )
{
	animations.push_back( SAnimationInfo( nObjectID, time + pAnimation->nLength ) );
}

void CDeadHouseAnimations::Sort()
{
	std::sort( animations.begin(), animations.end(), SSortAnimationsByTime() );
}

bool CDeadHouseAnimations::Update( const NTimer::STime &time )
{
	while ( !animations.empty() && time >= animations.back().nEndTime )
	{
		Scene()->RemoveObject( animations.back().nID );
		animations.pop_back();
	}
	return !animations.empty();
}


