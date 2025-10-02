#include "StdAfx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "DeadHouseAnimation.h"
#include "..\SceneB2\Scene.h"

REGISTER_SAVELOAD_CLASS( 0x12118C81, CDeadHouseAnimation );

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

