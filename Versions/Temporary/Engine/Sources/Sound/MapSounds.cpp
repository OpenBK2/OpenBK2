#include "StdAfx.h"
#include "mapsounds.h"
#include "SoundSceneInternal.h"
#include "SoundSceneConsts.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*															CMapSounds
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::CMapSoundCell::AddSound( const WORD wSoundID, const CVec3 &vPos, const CMapSounds::RegisteredSounds &registeredSounds, const WORD wInstanceID, const bool bLooped )
{
	if ( bLooped )
	{
		cellLoopedSounds[wSoundID].instanceIDs[wInstanceID] = vPos;
		++cellLoopedSounds[wSoundID].nCount;
	}
	else
	{
		cellSounds[wSoundID].instanceIDs[wInstanceID] = vPos;
		++cellSounds[wSoundID].nCount;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::CMapSoundCell::RemoveSound( const WORD wInstanceID, interface ISoundScene * pScene )
{
	// if removed sound is playing - remove it from sound scene
	if ( wInstanceID == playingLoopedSound.wInstanceID && 0 != playingLoopedSound.wSceneID)
	{
		pScene->RemoveSound( playingLoopedSound.wSceneID );
		playingLoopedSound.Clear();
	}
	// if currently playing looped sound mustn't play anymore, stop it.
	if ( wInstanceID == playingSound.wInstanceID && 0 != playingSound.wSceneID )
	{
		pScene->RemoveSound( playingSound.wSceneID );
		playingSound.Clear();
	}

	//if ( bLooped )
	RemoveSound( &cellLoopedSounds, wInstanceID );
	//else
	RemoveSound( &cellSounds, wInstanceID );

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::CMapSoundCell::RemoveSound( CMapSounds::CMapSoundCell::CellSounds *pCellSounds, const WORD wInstanceID )
{
	// remove this sound from types list
	for ( CellSounds::iterator it = pCellSounds->begin(); pCellSounds->end() != it ; ++it )
	{
		if ( it->second.nCount != 0 )
		{
			SMapSounds &snds = it->second;
			if ( snds.instanceIDs.find( wInstanceID ) != snds.instanceIDs.end() )
			{
				snds.instanceIDs.erase( wInstanceID );
				--snds.nCount;
				if ( 0 == snds.nCount )
					pCellSounds->erase( it );
				break;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::CMapSoundCell::Update( interface ISoundScene * pScene, const CMapSounds::RegisteredSounds &registeredSounds )
{
	if ( timeNextRun < CSoundScene2D::GetCurTime() )
	{
		// if sounds currently playing cannot be played amymore
		if ( 0 != playingLoopedSound.wSoundTypeID && cellSounds[playingLoopedSound.wSoundTypeID].nCount < SSoundSceneConsts::MIN_SOUND_COUNT_TO_PLAY_LOOPED )
		{
			pScene->RemoveSound( playingLoopedSound.wSceneID );
			playingLoopedSound.Clear();
		}

		// find new looped sound to play
		if ( 0 == playingLoopedSound.wSceneID )
		{
			// if there is no sounds plaing or looped run them ( if have to )
			SMaxCountPredicate pr;
			CellSounds::iterator maxElement = max_element( cellLoopedSounds.begin(), cellLoopedSounds.end(), pr );
			if ( maxElement != cellLoopedSounds.end() &&
				!maxElement->second.instanceIDs.empty() &&
				maxElement->second.nCount >= SSoundSceneConsts::MIN_SOUND_COUNT_TO_PLAY_LOOPED )
			{
				hash_map<WORD,CVec3>::iterator element = maxElement->second.instanceIDs.begin();
				playingLoopedSound.wInstanceID = element->first;
				playingLoopedSound.wSoundTypeID = maxElement->first;
				playingLoopedSound.wSceneID = pScene->AddSound( registeredSounds.ToT1(playingLoopedSound.wSoundTypeID),
					element->second,
					SFX_MIX_IF_TIME_EQUALS,
					SAM_LOOPED_NEED_ID, 0, 1.0f );
			}
		}		

		// run next non looped sound
		// remove current sound
		if ( 0 != playingSound.wSceneID && pScene->IsSoundFinished( playingSound.wSceneID ) )
		{
			pScene->RemoveSound( playingSound.wSceneID );
			playingSound.Clear();
			playingSound.wSceneID = 0;
		}
		if ( 0 == playingSound.wSceneID )
		{
			SMaxCountPredicate pr;
			CellSounds::iterator maxElement = max_element( cellSounds.begin(), cellSounds.end(), pr );

			if ( maxElement != cellSounds.end() && !maxElement->second.instanceIDs.empty() )
			{
				hash_map<WORD,CVec3>::iterator element = maxElement->second.instanceIDs.begin();
				playingSound.wInstanceID = element->first;
				playingSound.wSoundTypeID = maxElement->first;
				playingSound.wSceneID = pScene->AddSound( registeredSounds.ToT1( playingSound.wSoundTypeID ),
					element->second,
					SFX_MIX_IF_TIME_EQUALS,
					SAM_NEED_ID, 0, 1.0f );

			}
		}
		// set next run time
		timeNextRun = CSoundScene2D::GetCurTime() + SSoundSceneConsts::SS_MAP_SOUND_PERIOND + rand() * SSoundSceneConsts::SS_MAP_SOUND_PERIOND_RANDOM / RAND_MAX;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*															CMapSounds
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::SetSoundScene( struct ISoundScene *_pSoundScene )
{
	pSoundScene = _pSoundScene;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::InitSizes( const int nSizeX, const int nSizeY )
{
	// новое
	soundIDs.Clear();
	mapCells.SetSizes( nSizeX / SSoundSceneConsts::MAP_SOUND_CELL + 1, nSizeY / SSoundSceneConsts::MAP_SOUND_CELL + 1 );
	cells.clear();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::RemoveSound( const WORD wInstanceID )
{
	if ( 0 == wInstanceID ) return ;
	hash_map<WORD, SIntThree>::iterator pos = cells.find( wInstanceID );
	NI_ASSERT( pos != cells.end(), "wrong instace deleted" );
	if ( pos == cells.end() )
		return;
	const SIntThree & vPos = pos->second;;
	mapCells[vPos.y][vPos.x].RemoveSound( wInstanceID, pSoundScene );
	instanceIDs.Return( wInstanceID );
	cells.erase( wInstanceID );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WORD CMapSounds::AddSound( const CVec3 &vPos, const NDb::SComplexSoundDesc* pStats )
{
	// определить к какой клетке он относится.
	const SIntThree vCellPos( vPos.x / SSoundSceneConsts::MAP_SOUND_CELL, vPos.y / SSoundSceneConsts::MAP_SOUND_CELL, vPos.z / SSoundSceneConsts::MAP_SOUND_CELL );
	// check, can we add this sound, or it out of bounds
	if ( vCellPos.x < 0 || vCellPos.x >= mapCells.GetSizeX() )
		return 0;
	if ( vCellPos.y < 0 || vCellPos.y >= mapCells.GetSizeY() )
		return 0;
	// зарегистрировать звук
	if ( !registeredSounds.IsPresent( pStats ) )
	{
		const WORD wNewID = soundIDs.Get();
		registeredSounds.Add( pStats, wNewID );
	}
	// wSoundID - зарегистрированный звук.
	const WORD wSoundID = registeredSounds.ToT2( pStats );
	const WORD wInstanceID = instanceIDs.Get();
	// добавить его в эту клетку.
	mapCells[vCellPos.y][vCellPos.x].AddSound( wSoundID, vPos, registeredSounds, wInstanceID, pStats->bLooped );
	// добавить в карту клекта - ID звука.
	cells[wInstanceID] = vCellPos;
	return wInstanceID;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::Clear()
{
	soundIDs.Clear();
	mapCells.Clear();
	cells.clear();
	registeredSounds.Clear();
	instanceIDs.Clear();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CMapSounds::Update( const CVec3 &vListener, const float fViewRadius )
{
	if ( timeNextUpdate < CSoundScene2D::GetCurTime() )
	{
		const int nMinX = Max( 0, int(vListener.x - fViewRadius - SSoundSceneConsts::MAP_SOUND_CELL) ) / SSoundSceneConsts::MAP_SOUND_CELL;
		const int nMaxX = Min( mapCells.GetSizeX()-1, int(vListener.x + fViewRadius + SSoundSceneConsts::MAP_SOUND_CELL) / SSoundSceneConsts::MAP_SOUND_CELL );
		const int nMinY = Max( 0, int(vListener.y - fViewRadius - SSoundSceneConsts::MAP_SOUND_CELL) ) / SSoundSceneConsts::MAP_SOUND_CELL;
		const int nMaxY = Min( mapCells.GetSizeY()-1, int(vListener.y + fViewRadius + SSoundSceneConsts::MAP_SOUND_CELL) / SSoundSceneConsts::MAP_SOUND_CELL );

		//scan only through cells near screen
		for ( int x = nMinX; x <= nMaxX; ++x )
			for ( int y = nMinY; y <= nMaxY; ++y )
				mapCells[y][x].Update( pSoundScene, registeredSounds );
		timeNextUpdate = CSoundScene2D::GetCurTime() + SSoundSceneConsts::SS_MAP_SOUND_PERIOND;
	}
}
