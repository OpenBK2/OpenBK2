#include "stdafx.h"

#include "misc/2darray.h"
#include "stats_b2_m1/iconsset.h"
#include "DeadHouseAnimations.h"
#include "SceneB2/Scene.h"

#include <zconf.h>

REGISTER_SAVELOAD_CLASS( 0x3116C300, CDeadHouseAnimations );

void CDeadHouseAnimations::Add( int nObjectID, const NTimer::STime &time, const NDb::SAnimB2 *pAnimation )
{
	animations.push_back( SAnimationInfo( nObjectID, time + pAnimation->nLength ) );
}

void CDeadHouseAnimations::Sort()
{
	sort( animations.begin(), animations.end(), SSortAnimationsByTime() );
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


