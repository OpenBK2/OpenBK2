#include "stdafx.h"

#include "DeadHouseAnimations.h"
#include "Misc/2Darray.h"
#include "SceneB2/Scene.h"
#include "Stats_B2_M1/IconsSet.h"

#include "B2_M1_World_export.h"

#include <algorithm>

#include <zconf.h>

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
