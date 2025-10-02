#include "StdAfx.h"
#include "soundcell.h"
#include "SoundSceneInternal.h"
#include "SoundSceneConsts.h"

REGISTER_SAVELOAD_CLASS( 0x110793C2, CSoundCell );

//*******************************************************************
//*															CSoundCell::*
//*******************************************************************

CSoundCell::CSoundCell()
: nRadius( 0 ), timeLastCombatHear( 0 )
{
}

void CSoundCell::SetLastHearCombat( const NTimer::STime hearTime )
{
	timeLastCombatHear = hearTime;
}

void CSoundCell::Clear()
{
	sounds.clear();
	nRadius = 0;
}

bool CSoundCell::IsSoundHearable( const CSound *pSound, const int nRadius ) const
{
	return !pSound->IsMarkedFinished() &&
		pSound->GetRadiusMax() > nRadius &&
		( pSound->GetCombatType() == NDb::PEACEFULL ? !IsCombat() : true ) ;

}

bool CSoundCell::IsCombat() const
{
	return CSoundScene2D::GetCurTime() < timeLastCombatHear + SSoundSceneConsts::COMBAT_FEAR_TIME;
}

void CSoundCell::RecountForDelete()
{
	nRadius = 0;
	for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); ++it )
	{
		NI_ASSERT( 0 != (*it)->GetRadiusMax(), " 0 max radius" );
		nRadius = Max( nRadius, (*it)->GetRadiusMax() );
	}
}

void CSoundCell::AddSound( class CSound *pSound )
{
	NI_ASSERT( 0 != pSound->GetRadiusMax(), " 0 max radius" );
	sounds.push_back( pSound );
	nRadius = Max( nRadius, pSound->GetRadiusMax() );

}

void CSoundCell::RemoveSound( const WORD wID, ISFX * pSFX )
{
	for ( CSounds::iterator it = sounds.begin(); it != sounds.end();  )
	{
		if ( (*it)->GetID() == wID )
		{
			CSound * s = *it;
			if ( pSFX && !s->IsSubstituted() )
				pSFX->StopSample( s->GetSound() );
			it = sounds.erase( it );
			RecountForDelete();
			return;
		}
		++it;
	}
	NI_ASSERT( false, "not present" );
}

const CSound * CSoundCell::GetSound( const WORD wID ) const
{
	for ( CSounds::const_iterator it = sounds.begin(); it != sounds.end(); ++it )
	{
		if ( (*it)->GetID() == wID )
			return (*it);
	}
	return 0;
}

CSound * CSoundCell::GetSound( const WORD wID )
{
	for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); ++it )
	{
		if ( (*it)->GetID() == wID )
			return (*it);
	}
	return 0;
}

void CSoundCell::Update( ISFX * pSFX )
{
	bool bSomeSoundErased = false;
	for ( CSounds::iterator it = sounds.begin(); it != sounds.end(); )
	{
		CPtr<CSound> sound = (*it);

		if ( 0.0f == sound->GetVolume( CSoundScene2D::GetCurTime(), 0.0f ) )
		{
			if ( 0 == sound->GetID() )
			{
				pSFX->StopSample( sound->GetSound() );
				it = sounds.erase( it );
				bSomeSoundErased = true;
				continue;
			}
		}

		if ( !sound->IsMarkedFinished() && !sound->IsLooped() )
		{
			if( sound->IsTimeToFinish() )
			{
				if ( 0 == sound->GetID() ) // этим звуком управляет движок
				{
					it = sounds.erase( it );
					bSomeSoundErased = true;
					continue;
				}
				sound->MarkFinished();
			}
		}
		++it;
	}

	if ( bSomeSoundErased )
		RecountForDelete();
}

