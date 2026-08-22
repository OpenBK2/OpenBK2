#include "stdafx.h"
#include "SupportAAGun.h"
#include "UnitsIterators.h"
#include "Aviation.h"
#include "ShootEstimatorInternal.h"

#include "AILogic_export.h"

extern NTimer::STime curTime;

CSupportAAGun::CSupportAAGun( CAIUnit *_pUnit )
: timeNextTargetSearch( curTime ), predictedFire( _pUnit, 0 )
{
	pShootEstimator = new CShootEstimatorSupportAAGun( _pUnit );
}

void CSupportAAGun::AddGunNumber( const int nGun )
{
	predictedFire.AddGunNumber( nGun );
	dwAllowed |= (1<<nGun);
}

void CSupportAAGun::Segment()
{
	if ( !predictedFire.IsFinishedTask() )
		predictedFire.Segment();
	else if ( curTime >= timeNextTargetSearch )
	{
		// if there is no state - search for target
		const int nPlayer = predictedFire.GetUnit()->GetPlayer();
		float fBestRating = 0;
		CAviation *pBestTarget = 0;

		pShootEstimator->Reset( 0, false, ~dwAllowed );

		for ( CPlanesIter iter; !iter.IsFinished(); iter.Iterate() )
		{
			if ( theDipl.GetDiplStatus( nPlayer, (*iter)->GetPlayer() ) == EDI_ENEMY )
				pShootEstimator->AddUnit( *iter );		
		}
		predictedFire.SetTarget( static_cast<CAviation*>( pShootEstimator->GetBestUnit() ) );
	}
}


REGISTER_SAVELOAD_CLASS( AILOGIC, 0x11143C40, CSupportAAGun )

