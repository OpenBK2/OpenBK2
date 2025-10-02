#include "StdAfx.h"

#include "SoundSceneInternal.h"
#include "SubstSound.h"
#include "SoundCell.h"

int CSoundScene2D::operator&( IBinSaver &saver  )
{
	if ( saver.IsReading() )
	{
		InitConsts();
	}
	saver.Add( 2, &freeIDs );
	saver.Add( 5, &curTime );
	saver.Add( 6, &timeLastUpdate );
	saver.Add( 7, &vFormerCameraCell );
	saver.Add( 8, &substTable );
	saver.Add( 9, &vLimit );
	saver.Add( 10, &soundIDs );
	// а теперь самое сложное - звуки
	saver.Add( 12, &interfaceSounds );
	saver.Add( 13, &soundCellsInBounds  );
	// saver.Add( 14, &streamingSounds );
	saver.Add( 15, &terrainSounds );
	
	saver.Add( 16, &mapSounds );
	if ( saver.IsReading() )
		mapSounds.SetSoundScene( this );
	saver.Add( 18, &soundCellsWithSound );
	saver.Add( 19, &soundCellsOutOfBounds );
	saver.Add( 20, &eSoundSceneMode );
	saver.Add( 21, &finishedInterfaceSounds );
	saver.Add( 22, &deletedInterfaceSounds );

	saver.Add( 23, &cellsPHS );
	saver.Add( 24, &nMinZ );
	
	saver.Add( 25, &pSFX );
	saver.Add( 26, &pGameTimer );
	saver.Add( 27, &bMapInitted );
	saver.Add( 28, &vFormerCameraDir );
	return 0;
}

int CMapSounds::CMapSoundCell::SMapSounds::operator&( IBinSaver &saver  )
{
	saver.Add( 2, &instanceIDs ); // это не сериализовать
	saver.Add( 3, &nCount );
	return 0;
}

int CMapSounds::operator&( IBinSaver &saver  )
{
	saver.Add( 4, &soundIDs );
	saver.Add( 16, &mapCells );
	saver.Add( 17, &cells );
	saver.Add( 8, &instanceIDs );
	saver.Add( 9, &timeNextUpdate );
	saver.Add( 18, &registeredSounds );
	return 0;
}

int CMapSounds::CMapSoundCell::operator&( IBinSaver &saver )
{
	
	saver.Add( 1, &playingLoopedSound );
	saver.Add( 2, &playingSound );
	saver.Add( 3, &cellSounds );
	saver.Add( 4, &timeNextRun );
	saver.Add( 5, &cellLoopedSounds );
	return 0;

}

int CSubstSound::operator&( IBinSaver &saver  )
{
	saver.Add( 2, &pSFX );
	saver.Add( 3, &pSample );
	return 0;
}


int CSound::operator&( IBinSaver &saver  )
{
	saver.Add( 2, &wID );														// 
	saver.Add( 3, &timeBegin );
	saver.Add( 4, &timeToPlay );
	saver.Add( 5, &timeBeginDim );
	saver.Add( 6, &pDesc );
	saver.Add( 7, &eMixType );
	saver.Add( 8, &vPos );
	saver.Add( 9, &bLooped );
	saver.Add( 10, &bStartedMark );
	saver.Add( 11, &bFinishedMark );
	saver.Add( 12, &bDimMark );
	saver.Add( 13, &pSubstitute );

	saver.Add( 14, &pSample );
	saver.Add( 15, &nMaxRadius );
	saver.Add( 16, &nMinRadius );
	
	saver.Add( 17, &eCombatType );
	saver.Add( 18, &vSpeed );
	saver.Add( 19, &timeLastPosUpdate );
	return 0;
}

int CSoundCell::operator&( IBinSaver &saver  )
{
	saver.Add( 2, &nRadius );
	saver.Add( 4, &sounds );
	saver.Add( 5, &timeLastCombatHear );
	return 0;
}

int CTerrainSounds::operator&( IBinSaver &saver  )
{
	saver.Add( 1, &terrainSounds );
	saver.Add( 2, &vListener );
	saver.Add( 3, &pTerrain );
	saver.Add( 4, &lastUpdateTime	);
	saver.Add( 6, &bMuteAll );
	saver.Add( 7, &pSFX );
	return 0;
}

int CTerrainSounds::CTerrainSound::operator&( IBinSaver &saver  )
{
			
	saver.Add( 1, &timeRestart );
	saver.Add( 2, &bMustPlay );
	saver.Add( 6, &fVolume );
	saver.Add( 7, &fPan );
	saver.Add( 8, &vOffset );
	saver.Add( 9, &bNeedUpdate );
	saver.Add( 10, &cycledSounds );
	saver.Add( 11, &vSoundPos );
	saver.Add( 12, &wSound );

	return 0;
}

int CTerrainSounds::CTerrainSound::SSoundInfo::operator&( IBinSaver &saver  )
{
			
	saver.Add( 1, &pSound );
	saver.Add( 2, &bPeaceful );

	return 0;
}

int CCellsConglomerateContainer::operator&( IBinSaver &saver  )
{
	saver.Add( 1, &conglomeratesHeight );
	saver.Add( 2, &nMaxRank );
	saver.Add( 3, &bInitted );
	return 0;
}

