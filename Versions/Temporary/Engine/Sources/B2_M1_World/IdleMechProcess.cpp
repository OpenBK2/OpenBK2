#include "stdafx.h"

#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "IdleMechProcess.h"
#include "SceneB2/Scene.h"
#include "Misc/Win32Random.h"

#include <zconf.h>

REGISTER_SAVELOAD_CLASS( 0x12118C80, CIdleMechProcess );

CIdleMechProcess::CIdleMechProcess( int nObjectID, const std::vector<std::string> &effectBones, const NDb::SComplexEffect *pComplexEffect ) :
	nID( nObjectID ), pEffect( pComplexEffect->GetSceneEffect() ), times( 0 )
{
	bones.resize( effectBones.size() );
	for ( int i = 0; i < effectBones.size(); ++i )
		bones[i] = effectBones[i];
}

bool CIdleMechProcess::Update( const NTimer::STime &time )
{
	if ( bones.size() == 0 )
		return false;
	if ( times.size() == 0 )
	{
		times.resize( bones.size() );
		for ( int i = 0; i < times.size(); ++i )
			times[i] = time + NWin32Random::Random( 10000 );
	}
	else
	{
		IScene *pScene = Scene();
		for ( int i = 0; i< times.size(); ++i )
		{
			if ( times[i] <= time )
			{
				pScene->AttachEffect( nID, ESSOT_WATER_DROPS, bones[i], pEffect, times[i], ESAT_NO_REPLACE );
				times[i] = time + 5000 + NWin32Random::Random( 10000 );
			}
		}
	}
	return true;
}


