#include "stdafx.h"
#include "TerrainSounds.h"
#include "SoundSceneConsts.h"
#include "SoundSceneInternal.h"
#include "SoundManager.h"

//*******************************************************************
//*															CTerrainSound*
//*******************************************************************

void CTerrainSounds::CTerrainSound::AddCycledSound( const NDb::SComplexSoundDesc *pStats )
{
	const NDb::SComplexSoundDesc::SSoundStats *pRandomSound = pStats->GetRandomSound();

	if ( pRandomSound )
	{
		CPtr<ISound> pNewSound = CSoundManager::CreateSound2D( pRandomSound->pPathName, true );
		if ( pNewSound )
		{
			cycledSounds.push_back( SSoundInfo(pNewSound, pRandomSound->esoundType == NDb::PEACEFULL ) );
			bNeedUpdate = true;
		}
	}
}

void CTerrainSounds::CTerrainSound::StartCycledSounds( ISFX *pSFX, bool bNonPeacefulOnly )
{
	for ( CCycledSounds::iterator it = cycledSounds.begin(); it != cycledSounds.end(); ++it )
	{
		if ( bMustPlay && 
			( !bNonPeacefulOnly	|| !it->bPeaceful ) &&
			!pSFX->IsPlaying( it->pSound ) )
			pSFX->PlaySample( it->pSound, true );
	}
}

void CTerrainSounds::CTerrainSound::Update(	const struct SSoundTerrainInfo& info, 
																												const CVec3 &vListener, 
																												const float fViewSize, 
																												const float fRelativeVolume )
{
	//calc volume and pan and set new volume to ISounds 
	//if something changed set bNeedupdateFlag
	vSoundPos	= info.vPos;

	const CVec2 vNewOffset( info.vPos.x - vListener.x, info.vPos.y - vListener.y );
	if ( vNewOffset != vOffset )
	{
		vOffset = vNewOffset;
		//calc pan
		fPan = vOffset.x / fViewSize;

		//calc volume
		const float fOffset = fabs(vOffset);
		//dependence from distance
		const float fMinRadius = SSoundSceneConsts::TERRAIN_SOUND_RADIUS_MIN * fViewSize;
		float fVolumeDist;
		if ( fOffset < fMinRadius )
			fVolumeDist = 1.0f;
		else
			fVolumeDist = ( fOffset - fMinRadius ) / 
			( SSoundSceneConsts::TERRAIN_SOUND_RADIUS_MAX * fViewSize - fMinRadius );
		fVolumeDist = fVolumeDist < 0.0f ? 0.0f : fVolumeDist;

		//dependence from weight		
		float fVolumeWeight;
		if ( info.fWeight > SSoundSceneConsts::TERRAIN_CRITICAL_WEIGHT )
			fVolumeWeight = 1.0f;
		else
			fVolumeWeight = info.fWeight / SSoundSceneConsts::TERRAIN_CRITICAL_WEIGHT;

		fVolume = fVolumeDist * fVolumeWeight * fRelativeVolume ;
		bNeedUpdate = true;
	}
}

void CTerrainSounds::CTerrainSound::SetMustPlay( bool _bMustPlay ) 
{ 
	bNeedUpdate &= ( bMustPlay == _bMustPlay );
	bMustPlay = _bMustPlay; 
}

void CTerrainSounds::CTerrainSound::DoUpdate( ISFX * pSFX )
{
	if ( bMustPlay )
	{
		for ( CCycledSounds::iterator it = cycledSounds.begin(); it != cycledSounds.end(); ++it )
		{
			it->pSound->SetVolume( fVolume );
			it->pSound->SetPan( fPan );
			if ( pSFX->IsPlaying( it->pSound ) )
				pSFX->UpdateSample( it->pSound );
		}
	}
	else
		StopSounds( pSFX, false );

	bNeedUpdate = false;
}

void CTerrainSounds::CTerrainSound::SetSound( const NDb::SComplexSoundDesc *pStats, NTimer::STime timeWhenRestart, ISoundScene *pScene )
{
	if ( wSound )
	{
		if ( pScene->IsSoundFinished( wSound ) )
		{
			pScene->RemoveSound( wSound );
			wSound = 0;
		}
	}

	if ( !wSound )
		wSound = pScene->AddSound( pStats, vSoundPos, SFX_MIX_IF_TIME_EQUALS, SAM_NEED_ID, 0, 2 );
	timeRestart = timeWhenRestart;
}

void CTerrainSounds::CTerrainSound::StopSounds( ISFX * pSFX, bool bOnlyPeaceful )
{
	for ( CCycledSounds::iterator it = cycledSounds.begin(); it != cycledSounds.end(); ++it )
	{
		if ( !bOnlyPeaceful || it->bPeaceful )
			pSFX->StopSample( it->pSound );
	}
}


//*******************************************************************
//*															CTerrainSounds*
//*******************************************************************

void CTerrainSounds::Init( struct ITerrainSounds *_pTerrain )
{
	Clear();
	pTerrain = _pTerrain;
	pSFX = Singleton<ISFX>();
}

void CTerrainSounds::Clear()
{
	for ( CSounds::iterator it = terrainSounds.begin(); it != terrainSounds.end();  ++it )
		(*it).second.StopSounds( pSFX, false );

	terrainSounds.clear();
	vListener = CVec3(-1,-1,-1);
	lastUpdateTime = 0;
	bMuteAll = false;
}

void CTerrainSounds::Mute( const bool bMute )
{
	bMuteAll = bMute;
	if ( bMuteAll )
	{
		for ( CSounds::iterator it = terrainSounds.begin(); it != terrainSounds.end();  ++it )
		{
			CTerrainSound &sound = (*it).second;
			sound.StopSounds( pSFX, false );
		}
	}
}

void CTerrainSounds::Update( const CVec3 &vNewListener, const float fViewSize, const bool bCombat, struct ISoundScene *pScene )
{
	if ( !pTerrain || bMuteAll ) return ;

	// если камера сместилась - получить информацию о звуках заново,
	// все их запустить
	if ( vListener != vNewListener || CSoundScene2D::GetCurTime() - lastUpdateTime > SSoundSceneConsts::SS_UPDATE_PERIOD )
	{
		lastUpdateTime = CSoundScene2D::GetCurTime();
		vListener = vNewListener;
		std::vector<SSoundTerrainInfo> pInfo;
		pTerrain->GetTerrainMassData( &pInfo, SSoundSceneConsts::SS_AMBIENT_TERRAIN_SOUNDS );
		const int nSize = pInfo.size();

		if ( 0 != nSize )
		{
			// mark sounds that must play (for one pass, so code may be ugly)
			const int nMaxTerrain = pInfo[nSize-1].nTerrainType;
			int nCurSizeIndex = 0;
			for ( int i = 0; i <= nMaxTerrain; ++i )
			{
				if ( i == pInfo[nCurSizeIndex].nTerrainType )
				{
					terrainSounds[i].Update( pInfo[nCurSizeIndex], vListener, fViewSize, pTerrain->GetSoundVolume( i ) );
					terrainSounds[i].SetMustPlay( true );
					if ( nCurSizeIndex + 1 < nSize ) 
						++nCurSizeIndex;
				}
				else
					terrainSounds[i].SetMustPlay( false );
			}
		}
	}

	// обработать all sounds 
	for ( CSounds::iterator it = terrainSounds.begin(); it != terrainSounds.end();  ++it )
	{
		CTerrainSound &sound = (*it).second;
		//if sound is to be changed and it is finished
		// not cycle sound
		if ( sound.GetRestartTime() <= CSoundScene2D::GetCurTime() && sound.IsMustPlay() )
		{
			//Update sound, run it again
			const NDb::SComplexSoundDesc *pStats = pTerrain->GetTerrainSound( (*it).first );

			sound.SetSound( pStats, CSoundScene2D::GetCurTime() + SSoundSceneConsts::SS_AMBIENT_SOUND_CHANGE + 
				SSoundSceneConsts::SS_AMBIENT_SOUND_CHANGE_RANDOM * rand() / RAND_MAX, pScene );

		}

		// create cycle sound
		if ( !sound.HasCycleSound() ) //sound isn't started yet
		{
			const NDb::SComplexSoundDesc *pStats = pTerrain->GetTerrainCycleSound( (*it).first );
			if ( pStats )
				sound.AddCycledSound( pStats );
			sound.StartCycledSounds( pSFX, bCombat );
		}

		// if in combat situation mute peaceful sounds or restart muted in noncombat situation
		if ( bCombat )
			sound.StopSounds( pSFX, true );
		else
			sound.StartCycledSounds( pSFX, false );

		//update sounds
		if ( sound.IsNeedUpdate() )
			sound.DoUpdate( pSFX );
	}
}

