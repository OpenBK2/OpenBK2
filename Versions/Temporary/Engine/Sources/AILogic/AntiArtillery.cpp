#include "stdafx.h"

#include "AntiArtillery.h"
#include "NewUpdater.h"
#include "Randomize.h"
#include "UnitsIterators2.h"
#include "AntiArtilleryManager.h"
#include "AIUnit.h"
#include "Cheats.h"
#include "FeedbackSystem.h"

REGISTER_SAVELOAD_CLASS( 0x1108D4A1, CAntiArtillery );
REGISTER_SAVELOAD_CLASS( 0x191442C0, CRevealCircle );

extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern CAntiArtilleryManager theAAManager;
extern SCheats theCheats;
extern CFeedBackSystem theFeedBackSystem;

CAntiArtillery::CAntiArtillery( CAIUnit *_pOwner )
: pOwner( _pOwner ), fMaxRadius( 0.0f ), nParty( -1 ), lastScan( 0 ), bIsAA( false )
{
	SetUniqueIdForObjects();
}

void CAntiArtillery::Init( const float _fMaxRadius, const int _nParty )
{
	bIsAA = false;
	Mem2UniqueIdObjs();
	
	fMaxRadius = _fMaxRadius;
	if ( fMaxRadius == 0.0f )
		fMaxRadius = ( GetAIMap()->GetSizeX() + GetAIMap()->GetSizeY() ) * SAIConsts::TILE_SIZE;

	nParty = _nParty;

	lastScan = 0;

	// 3 - всего три стороны
	closestEnemyDist2.resize( 3, 0.0f );
	lastHeardPos.resize( 3, VNULL2 );
	nHeardShots.resize( 3, 0 );
	lastRevealCenter.resize( 3, VNULL2 );
	
	lastShotTime.resize( 3, 0 );
	lastRevealCircleTime.resize( 3, 0 );
}

void CAntiArtillery::Scan( const CVec2 &center )
{
	memset( &(closestEnemyDist2[0]), 0, closestEnemyDist2.size() );
	
	for ( CUnitsIter<0,3> iter( nParty, EDI_ENEMY, center, fMaxRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		if ( closestEnemyDist2[(*iter)->GetParty()] == 0 ) 
		{
			const float fR2 = fabs2( center - (*iter)->GetCenterPlain() );
			if ( fR2 <= sqr( fMaxRadius ) )
				closestEnemyDist2[ (*iter)->GetParty() ] = fR2;
		}
	}

	lastScan = curTime;
}

float GetRadius( const float nShots, const float fRevealRadius )
{
	float fMax = fRevealRadius * ( 1.0f - nShots / SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS );
	fMax = fMax < SConsts::MIN_ANTI_ARTILLERY_RADIUS ? SConsts::MIN_ANTI_ARTILLERY_RADIUS : fMax;
	fMax = fMax > SConsts::MAX_ANTI_ARTILLERY_RADIUS ? SConsts::MAX_ANTI_ARTILLERY_RADIUS : fMax;
	return fMax;
	// I don't understand what is written below...
	/*float fMax = SConsts::MAX_ANTI_ARTILLERY_RADIUS *
								SConsts::ARTILLERY_REVEAL_COEEFICIENT / fRevealRadius;
	fMax = fMax < SConsts::MIN_ANTI_ARTILLERY_RADIUS ? SConsts::MIN_ANTI_ARTILLERY_RADIUS : fMax;
	
	return fMax - (fMax - SConsts::MIN_ANTI_ARTILLERY_RADIUS)/ SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS * nShots;*/
	// Another (older) variant
	/*return SConsts::ARTILLERY_REVEAL_COEEFICIENT/fRevealRadius * 
	(SConsts::MAX_ANTI_ARTILLERY_RADIUS - ( SConsts::MAX_ANTI_ARTILLERY_RADIUS - SConsts::MIN_ANTI_ARTILLERY_RADIUS ) / SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS * nShots); */
}

void CAntiArtillery::Fired( const float _fGunRadius, const CVec2 &center )
{
	if ( curTime - lastScan >= SConsts::ANTI_ARTILLERY_SCAN_TIME )
		Scan( center );
	theAAManager.AddAA( this );

	float fGunRadius = abs( _fGunRadius );
	bIsAA = ( _fGunRadius < 0.0f );
	if ( bIsAA )
	{
		fGunRadius = fMaxRadius;					// If reveal radius is negative, take max radius and consider it an AA gun
	}

	const float fGunRadius2 = sqr( fGunRadius );
	for ( int i = 0; i < 2; ++i )
	{
		if ( i != nParty && closestEnemyDist2[i] <= fGunRadius2 && closestEnemyDist2[i] != 0.0f )
		{
			if ( fabs2( center - lastHeardPos[i] ) > sqr( SConsts::RELOCATION_RADIUS ) || i == 0 && curTime - lastShotTime[i] > SConsts::AUDIBILITY_TIME )
				nHeardShots[i] = 0;

			CVec2 newCenter;
			const float fCurRadius = GetRadius( Min( (float)nHeardShots[i], (float)SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS ), fGunRadius );

			if ( nHeardShots[i] > SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS )
				newCenter = lastRevealCenter[i];
			else
			{
				const float fOldRadius = GetRadius( nHeardShots[i] - 1, fGunRadius );

				RandQuadrInCircle( fCurRadius, &newCenter );
				newCenter += center;
			}

			newCenter.x = Clamp( newCenter.x, 0.0f, (float)GetAIMap()->GetSizeX() * SConsts::TILE_SIZE );
			newCenter.y = Clamp( newCenter.y, 0.0f, (float)GetAIMap()->GetSizeY() * SConsts::TILE_SIZE );

			lastHeardPos[i] = center;
			lastRevealCenter[i] = newCenter;

			if ( nHeardShots[i] <= SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS )
				++nHeardShots[i];

			lastShotTime[i] = curTime;
		}
	}
}

const CCircle CAntiArtillery::GetRevealCircle( const int nParty ) const
{
	return CCircle( lastRevealCenter[nParty], 
									GetRadius( Min( (float)nHeardShots[nParty], (float)SConsts::SHOTS_TO_MINIMIZE_LOCATION_RADIUS ), fMaxRadius ) );
}

const NTimer::STime CAntiArtillery::GetLastHeardTime( const int nParty ) const
{
	return lastShotTime[nParty];
}

void CAntiArtillery::Segment( bool bOwnerVisible )
{
	const int nMyParty = theDipl.GetMyParty();	
	for ( int nIterParty = 0; nIterParty < 2; ++nIterParty )
	{
		// если player - враг, слышал выстрел, не слишком давно, и пора рисовать круг
		const bool bEnemy = theDipl.GetDiplStatusForParties( nParty, nIterParty ) == EDI_ENEMY;
		
		const bool bVisibleCircle = lastShotTime[nIterParty] != 0 && curTime - lastShotTime[nIterParty] <= SConsts::AUDIBILITY_TIME;
		const bool bHaveToSendCircle = bVisibleCircle && curTime - lastRevealCircleTime[nIterParty] >= SConsts::REVEAL_CIRCLE_PERIOD;

		if ( bEnemy && nMyParty == nIterParty && !bIsAA )
		{
			if ( bVisibleCircle )
				theFeedBackSystem.AddFeedback( pOwner->GetUniqueId(), pOwner->GetCenterPlain(), EFB_HOWITZER_GUN_FIRED, -1 );
			else
				theFeedBackSystem.RemoveFeedback( pOwner->GetUniqueId(), EFB_HOWITZER_GUN_FIRED );
		}

		if ( bEnemy && bHaveToSendCircle )
		{
			lastRevealCircleTime[nIterParty] = curTime;

			// обязательно нужно создать, чтобы общая нумерация объектов не зависела от клиента
			CPtr<CRevealCircle> pCircle = new CRevealCircle( GetRevealCircle( 1 - nParty ) );
			
			// этон наша сторона и мы для него не видны, 
			if ( nMyParty == nIterParty && !bOwnerVisible )
			{
				if ( bIsAA )
					updater.AddUpdate( 0, ACTION_NOTIFY_REVEAL_ARTILLERY, pCircle, 1 );
				else
					updater.AddUpdate( 0, ACTION_NOTIFY_REVEAL_ARTILLERY, pCircle, -1 );
			}
		}
	}
}


