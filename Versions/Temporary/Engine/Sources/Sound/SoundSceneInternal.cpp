#include "stdafx.h"

#include "SoundSceneInternal.h"
#include "SoundSceneConsts.h"
#include "SoundCell.h"
#include "SubstSound.h"
#include "SoundManager.h"
#include "System/Commands.h"
#include "DBSoundDesc.h"


//#pragma comment(lib, "dsound.lib")

class CAckTuning 
{
	typedef std::unordered_map<std::string, int> CPlayedAcks;
	ZDATA
	CPlayedAcks playedAcks;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&playedAcks); return 0; }

	bool bInitted;
public:
	void SaveAsText( const std::string &szFile )
	{
		std::string szResultFile = szFile.c_str();
		FILE *pFile = fopen( szResultFile.c_str(), "w" );
		if ( pFile == 0 )
		{
			csSystem << "Can't open " << szResultFile << endl;
			return;
		}
		fprintf( pFile, "Name,Times\n" );

		for ( CPlayedAcks::iterator it = playedAcks.begin(); it != playedAcks.end(); ++it )
		{
			fprintf( pFile, StrFmt( "%s,%i\n", it->first.c_str(), it->second) );
		}
		fclose( pFile );
	}

	~CAckTuning()
	{
		if ( !playedAcks.empty() )
		{
			// write file
			CFileStream stream( "acklog.bin", CFileStream::WIN_CREATE );
			if ( !stream.IsOk() )
				return;

			CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			operator&( *pSaver.GetPtr() );
			SaveAsText( "acklog.txt" );
		}
	}
	CAckTuning() : bInitted( false )
	{
	}
	void Add( const std::string &szAck )
	{
		if ( !bInitted )
		{
			// read file with statistics
			CFileStream stream( "acklog.bin", CFileStream::WIN_READ_ONLY );
			if ( stream.IsOk() )
			{
				CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
				operator&( *pSaver.GetPtr() );
			}
			bInitted = true;
		}
		++playedAcks[szAck];
	}
};

CAckTuning theAckTuning;

REGISTER_SAVELOAD_CLASS( 0x1107C480, CSoundScene2D )
REGISTER_SAVELOAD_CLASS( 0x11190BC0, CSoundScene ) 
REGISTER_SAVELOAD_CLASS( 0x11190BC1, CSoundScene3D ) 

NTimer::STime CSoundScene2D::curTime;			// чтобы не передавать всюду
SIntThree CSoundScene2D::vLimit;						// размер в клетках всей звуковой сцены
int CSoundScene2D::nMinZ;										// minimum camera height
static bool s_bSound5_1 = false;

//*******************************************************************
//*															CSoundScene2D::CSoundsCollector::
//*******************************************************************

void CSoundScene2D::CSoundsCollector::operator()( int nRadius, const SIntThree & vCell )
{
	NI_ASSERT( cellsWSound.find(vCell) != cellsWSound.end(), StrFmt("Can't find cell at {%d : %d : %d}", vCell.x, vCell.y, vCell.z) );
	cellsWSound[vCell]->EnumHearableSounds( nRadius, *this );
}

void CSoundScene2D::CSoundsCollector::operator()( CSound * sound, bool bHearable )
{
	if ( bHearable )
	{
		CSoundSubstTable::iterator it = substTable.find( sound->GetDesc() );
		if ( substTable.end() == it )
		{
			// сам звук будет своей заменой
			sounds[sound->GetDesc()].push_back( sound );
		}
		else
			sounds[(*it).second].push_back( sound );
	}
	else
	{
		muteSounds.push_back( sound );
	}
}

//*******************************************************************
//*															CSoundScene2D
//*******************************************************************

void CSoundScene2D::InitConsts()
{
	pGameTimer = Singleton<IGameTimer>();
	SSoundSceneConsts::Load();
	Init();
}

CSoundScene2D::CSoundScene2D()
: pSFX( 0 ), pGameTimer( 0 ), bMapInitted( false )
{
	mapSounds.SetSoundScene( this );
	InitConsts();
}

CSoundScene2D::~CSoundScene2D()
{
//	Clear();
}

void CSoundScene2D::SetTerrain( struct ITerrainSounds * pTerrain )
{ 
	terrainSounds.Init( pTerrain ); 
}

void CSoundScene2D::Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE )
{
	bMapInitted = true;
	SSoundSceneConsts::SS_TILE_SIZE = _VIS_TILE_SIZE;
	InitConsts();
	//
	vLimit.x = _nMaxX / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE;
	if ( vLimit.x == 0 )
		vLimit.x = 1;

	vLimit.y = _nMaxY / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE;
	if ( vLimit.y == 0 )
		vLimit.y = 1;

	vLimit.z = _nMaxZ / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE;
	nMinZ =		 _nMinZ / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE;
	if ( nMinZ >= vLimit.z )
		vLimit.z = nMinZ + 1 ;

	soundCellsInBounds.resize( vLimit.z - nMinZ );
	for ( int z = 0; z < soundCellsInBounds.size(); ++z )
	{
		soundCellsInBounds[z].SetSizes( vLimit.x, vLimit.y );
		for ( int x = 0; x < vLimit.x; ++x )
			for ( int y = 0; y < vLimit.y; ++y )
				soundCellsInBounds[z][y][x] = new CSoundCell;
	}
	curTime = 0;
	vFormerCameraCell.x = -1;
	vFormerCameraCell.y = -1;
	vFormerCameraDir = VNULL2;
	timeLastUpdate = pGameTimer->GetAbsTime();

 	cellsPHS.Init( vLimit.x, vLimit.y, nMinZ, vLimit.z );
	mapSounds.InitSizes( _nMaxX, _nMaxY );

//	const float fSoundVol = NGlobal::GetVar( "sound_sfx_master_volume", 1.0f );
//	pSFX->SetSFXMasterVolume( fSoundVol );
	pSFX->Set3DMode( false );
}

void CSoundScene2D::To2DSoundPos( const CVec3 &vPos, CVec3 *v2DPos )
{
	v2DPos->x = vPos.x - vPos.z * FP_SQRT_2;
	v2DPos->y = vPos.y + vPos.z * FP_SQRT_2;
	v2DPos->z = 0;
}

void CSoundScene2D::MuteTerrain( const bool bMute )
{
	terrainSounds.Mute( bMute );
}

WORD CSoundScene2D::AddSound( const NDb::SComplexSoundDesc *pStats, const CVec3 &vPos,
												    const enum ESoundMixType eMixMode, const enum ESoundAddMode eAddMode,
														const unsigned int nTimeAfterStart,
														int nVolumeType )
{
	if ( !pSFX->IsSFXEnabled() || 0 == pStats ) 
		return 0;

	const NDb::SComplexSoundDesc::SSoundStats *pRandomSound = pStats->GetRandomSound();

	if ( !pRandomSound )
		return 0;

	if ( !pRandomSound->pPathName ) // empty sound rolled, don't add it to SoundScene
		return 0;

	const bool bLooped = eAddMode == SAM_LOOPED_NEED_ID;
	const bool bNeedID = eAddMode == SAM_NEED_ID || eAddMode == SAM_LOOPED_NEED_ID;

	CPtr<ISound> pSound = CSoundManager::CreateSound2D( pRandomSound->pPathName, bLooped );


	if ( pSound )
	{
		const WORD wID = bNeedID ? freeIDs.Get() : 0;
		const bool bToCell = eMixMode != SFX_INTERFACE;
#ifndef _FINALRELEASE
		if ( nVolumeType == 1 )
			theAckTuning.Add( pRandomSound->pPathName->szSoundPath );
#endif
		pSound->SetVolumeType( nVolumeType );
		CPtr<CSound> pSnd = new CSound( wID, pRandomSound->pPathName, pSound, eMixMode, 
																		bToCell ? vPos : VNULL3, bLooped, 
																		pRandomSound->esoundType,
																	  pRandomSound->fMinDist * SSoundSceneConsts::SS_TILE_SIZE,
																	  pRandomSound->fMaxDist * SSoundSceneConsts::SS_TILE_SIZE );
		pSnd->SetBeginTime( GetCurTime() - nTimeAfterStart );
		if ( !bToCell )
		{
			interfaceSounds[pSnd->GetDesc()].push_back( pSnd );
		}
		else if ( ESSM_INGAME == eSoundSceneMode )
		{
			const SIntThree vCell( vPos / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE );
			AddSound( vCell, pSnd );	
		}
		else 
		{
			if ( bNeedID )
				freeIDs.Return( wID );
			return 0;
		}	
		return wID;
	}
	return 0;
}

const NTimer::STime & CSoundScene2D::GetCurTime()
{
	return curTime;
}

void CSoundScene2D::ClearSounds()
{
	for ( CHearableSounds::iterator itList = interfaceSounds.begin(); itList != interfaceSounds.end(); ++itList )
	{
		for ( CSoundsList::iterator itSound = itList->second.begin(); itSound != itList->second.end(); ++itSound )
		{
			if ( (*itSound) )
			{
				(*itSound)->UnSubstitute();
				pSFX->StopSample( (*itSound)->GetSound() );
			}
		}
	}
	interfaceSounds.clear();
	finishedInterfaceSounds.clear();
	deletedInterfaceSounds.clear();
	//
	bMapInitted = false;
	mapSounds.Clear();
	freeIDs.Clear();
	substTable.clear();
	soundCellsInBounds.clear();
	soundCellsWithSound.clear();
	soundCellsOutOfBounds.clear();
	terrainSounds.Clear();
	mapSounds.Clear();
	cellsPHS.Clear();
	updatedCells.clear();
	soundIDs.clear();
	vLimit = SIntThree( -1, -1, -1 );
}

void CSoundScene2D::SetSoundSceneMode( const enum ESoundSceneMode _eSoundSceneMode )
{
	if ( eSoundSceneMode != _eSoundSceneMode)
	{
		if ( ESSM_CLEAR_SOUNDS == _eSoundSceneMode )
		{
			ClearSounds();
			return;
		}
		eSoundSceneMode = _eSoundSceneMode;
		// clear all game sounds
		ClearSounds();
		switch( eSoundSceneMode )
		{
		case ESSM_INGAME:
			break;
		case ESSM_INTERMISSION_INTERFACE:
			break;
		}
	}
}

bool CSoundScene2D::IsInBounds( const int x, const int y, const int z )
{ 
	return x >= 0 && x < vLimit.x &&
				 y >= 0 && y < vLimit.y &&
				 z >= nMinZ && z < vLimit.z; 
}

// helper function
CSoundCell * CSoundScene2D::GetSoundCell( const SIntThree &vCell )
{
	if ( IsInBounds( vCell.x, vCell.y, vCell.z ) )
	{
		if ( !soundCellsInBounds[vCell.z-nMinZ][vCell.y][vCell.x] )
			soundCellsInBounds[vCell.z-nMinZ][vCell.y][vCell.x] = new CSoundCell;
		return
			soundCellsInBounds[vCell.z-nMinZ][vCell.y][vCell.x];
	}
	else
	{
		if ( !soundCellsOutOfBounds[vCell] )
			soundCellsOutOfBounds[vCell] = new CSoundCell;
		return
			soundCellsOutOfBounds[vCell];
	}
}

void CSoundScene2D::AddSound( const SIntThree &vCell, CSound *pSound )
{
	CSoundCell *pCell = GetSoundCell( vCell );
	NI_ASSERT( pCell != 0, StrFmt( "Can't get cell at {%d : %d : %d}", vCell.x, vCell.y, vCell.z ) );
	soundCellsWithSound[vCell] = pCell;
	const float fFormerRadius = pCell->GetRadius();
	pCell->AddSound( pSound );
	const float fNewRadius = pCell->GetRadius();
	UpdatePHSMap( vCell, fFormerRadius, fNewRadius );
	UpdateCombatMap( vCell, pSound );
	if ( 0 != pSound->GetID() )
		soundIDs[pSound->GetID()] = vCell;
}

void CSoundScene2D::UpdateCombatMap( const SIntThree &vCell, CSound *pSound )
{
	if ( pSound->GetCombatType() == NDb::COMBAT )
	{
		const int nRadius = SSoundSceneConsts::COMBAT_SOUND_FEAR_RADIUS;

		const SIntThree vMinPos( (std::max)(vCell.x - nRadius, 0),
														 (std::max)(vCell.y - nRadius, 0),
														 (std::max)(vCell.z - nRadius, nMinZ ) );

		const SIntThree vMaxPos( (std::min)(vCell.x + nRadius, vLimit.x - 1),
														 (std::min)(vCell.y + nRadius, vLimit.y - 1),
														 (std::min)(vCell.z + nRadius, vLimit.z - 1) );

		SIntThree vCur;
		for ( vCur.z = vMinPos.z; vCur.z < vMaxPos.z; ++vCur.z )
		{
			for ( vCur.x = vMinPos.x; vCur.x < vMaxPos.x; ++vCur.x  )
			{
				for ( vCur.y = vMinPos.y; vCur.y < vMaxPos.y; ++vCur.y )
				{
					soundCellsInBounds[vCur.z-nMinZ][vCur.y][vCur.x]->SetLastHearCombat( CSoundScene2D::GetCurTime() );
				}
			}
		}
	}
}

void CSoundScene2D::UpdatePHSMap( const SIntThree &vCell, const int nFormerRadius, const int nNewRadius )
{
	if ( nNewRadius != nFormerRadius )
	{
		cellsPHS.RemoveHearCell( vCell, nFormerRadius );
		cellsPHS.AddHearCell( vCell, nNewRadius );
	}
}

void CSoundScene2D::RemoveSound( const WORD wID )
{
	if ( !pSFX->IsSFXEnabled() || 0 == wID ) 
		return;

	if ( soundIDs.find( wID ) == soundIDs.end() )
	{
		if ( finishedInterfaceSounds.find( wID ) == finishedInterfaceSounds.end() )
			deletedInterfaceSounds.insert( wID );
		else
			finishedInterfaceSounds.erase( wID );
		return;
	}
	if ( soundCellsWithSound.find( soundIDs[wID] ) == soundCellsWithSound.end() ) 
		return ;

	const SIntThree vCell = soundIDs[wID];
	CSoundCell *pCell = GetSoundCell( vCell );

	CSound * pSound = pCell->GetSound( wID );
	if ( pSound )
	{
		const WORD wID = pSound->GetID();

		if ( pSound->IsLooped() )
			pSound->MarkToDim( GetCurTime() );
		else
		{
			const int nFormerRadius = pCell->GetRadius();
			pCell->RemoveSound( wID, pSFX );
			const int nNewRadius = pCell->GetRadius();
			UpdatePHSMap( vCell, nFormerRadius, nNewRadius );
		}
		
		if ( wID )
		{
			soundIDs.erase( wID );
			freeIDs.Return( wID );
		}
	}

}

void CSoundScene2D::SetSoundPos( const WORD wID, const CVec3 &vPos )
{
	if (	!pSFX->IsSFXEnabled() ||
			0 == wID ||
			soundIDs.find( wID ) == soundIDs.end() ||
			soundCellsWithSound.find( soundIDs[wID] ) == soundCellsWithSound.end() )
		return;

	const SIntThree &vFormerCell= soundIDs[wID];
	CSoundCell *pFormerCell = GetSoundCell( vFormerCell );
	CSound *pSound = pFormerCell->GetSound( wID );

	if ( !pSound ) 
		return;
	NI_ASSERT( pSound != 0, "sound doesn't exist" );

	const SIntThree vNewCell( vPos / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE );
	
	// звук переместился в другую клетку
	if ( vFormerCell != vNewCell )
	{
		CSoundCell *pNewCell = GetSoundCell( vNewCell );
		
		// add sound to new cell
		NI_ASSERT( pNewCell != 0, StrFmt( "cannot create cell at (%d, %d )", vNewCell.x, vNewCell.y ) );
		const int nFormerRadiusNewCell = pNewCell->GetRadius();
		pNewCell->AddSound( pSound );
		const int nNewRadiusNewCell = pNewCell->GetRadius();


		// remove sound form old cell
		const int nFormerRadiusOldCell = pFormerCell->GetRadius();
		pFormerCell->RemoveSound( wID );
		const int nNewRadiusOldCell = pFormerCell->GetRadius();
		
		// update PHS map
		UpdatePHSMap( vNewCell, nFormerRadiusNewCell, nNewRadiusNewCell );
		UpdatePHSMap( vFormerCell, nFormerRadiusOldCell, nNewRadiusOldCell );

		soundCellsWithSound[vNewCell] = pNewCell;
		soundIDs[wID] = vNewCell;
	}
	pSound->SetPos( vPos );
}

bool CSoundScene2D::IsSoundFinished( const WORD wID ) const
{
	if (	!pSFX->IsSFXEnabled() || 0 == wID ) 
		return true;

	CSoundIDs::const_iterator posID = soundIDs.find( wID );

	if ( posID == soundIDs.end() ) // sound is from interface.
		return finishedInterfaceSounds.find( wID ) != finishedInterfaceSounds.end();

	CSoundCellsWithSound::const_iterator pos = soundCellsWithSound.find( posID->second );
	if ( pos == soundCellsWithSound.end() ) 
		return true;

	const CSound *pSound = pos->second->GetSound( wID );
	return !pSound || pSound->IsTimeToFinish();
}

void CSoundScene2D::UpdateSound( const CVec3 &_vListener, const CVec3 &_vCameraDir, const float fViewRadius )
{
	if ( !pSFX || !pGameTimer )
		return;
	pSFX->Update( _vListener, _vCameraDir, GetCurTime() - timeLastUpdate ); // ВЫЗЫВАТЬ РАНЬШЕ ВСЯКОЙ РАБОТЫ СО ЗВУКОМ!

	const CVec3 vListener( _vListener.x, _vListener.y, 0 );
	CVec2 vCameraDir( _vCameraDir.x, _vCameraDir.y );
	Normalize( &vCameraDir );

	MixInterfaceSounds(); // звуки от интерфейса должны играть под паузой
	
	if ( bMapInitted ) 
	{
		if ( pSFX->IsSFXEnabled() && !pSFX->IsPaused() ) 
		{
			SIntThree vListenerCell( vListener / SSoundSceneConsts::SS_SOUND_CELL_SIZE / SSoundSceneConsts::SS_TILE_SIZE );
 			// NI_ASSERT( IsInBounds( vListenerCell.x, vListenerCell.y, vListenerCell.z ), "camera is out of scene bounds" );
			vListenerCell.x  = Clamp( vListenerCell.x, 0, vLimit.x - 1);
			vListenerCell.y  = Clamp( vListenerCell.y, 0, vLimit.y - 1);
			vListenerCell.z  = Clamp( vListenerCell.z, 0, vLimit.z - 1);

			terrainSounds.Update( vListener, fViewRadius, soundCellsInBounds[vListenerCell.z-nMinZ][vListenerCell.y][vListenerCell.x]->IsCombat(), this );
			mapSounds.Update( vListener, fViewRadius );
			CHearableSounds sounds;
			CSoundsList			muteSounds;
			CSoundsCollector collector( substTable, sounds, soundCellsWithSound, muteSounds );
			
			if ( CSoundScene2D::GetCurTime() > timeLastUpdate + SSoundSceneConsts::SS_UPDATE_PERIOD ||
					((vListenerCell.x == -1 || vListenerCell.y != -1) && vFormerCameraCell != vListenerCell) || 
					vCameraDir != vFormerCameraDir ) 
			{// камера сместилась - полный пересчет
				vFormerCameraDir = vCameraDir;

				// ------------- удалить все доигравшие звуки
				for ( CSoundCellsWithSound::iterator it = soundCellsWithSound.begin(); it != soundCellsWithSound.end(); ++it )
				{
					NI_ASSERT( IsValid( it->second ), StrFmt("Invalid cell at ( delete finished sounds ){%d : %d}", it->first.x, it->first.y) );
					CSoundCell * pCell = (*it).second;
					const int nFormerRadius = pCell->GetRadius();
					pCell->Update( pSFX );
					const int nNewRadius = pCell->GetRadius();
					if ( nFormerRadius != nNewRadius ) // collect updated
						updatedCells.push_back( SUpdatedCell( (*it).first, nFormerRadius, nNewRadius ) );
				}

				for ( CUpdatedCells::iterator it = updatedCells.begin(); it != updatedCells.end();  )
				{
					UpdatePHSMap( (*it).vCell, (*it).nFormerRadius, (*it).nNewRadius );
					it = updatedCells.erase( it );
				}
				
				// -- взять все звуки, которые слышны в центральной клетке
				for ( CSoundCellsWithSound::iterator it = soundCellsWithSound.begin(); it != soundCellsWithSound.end(); ++it )
				{
					NI_ASSERT( IsValid( it->second ), StrFmt("Invalid cell at ( enum all sounds ){%d : %d}", it->first.x, it->first.y) );
					const int nRadius = abs( vListenerCell.x - (*it).first.x ) + abs( vListenerCell.y - (*it).first.y );
					(*it).second->EnumAllSounds( collector, nRadius );
				}
				vFormerCameraCell = vListenerCell;
				timeLastUpdate = CSoundScene2D::GetCurTime();
			}
			else
				cellsPHS.EnumHearableCells( collector, vListenerCell );
	
			MuteSounds( &muteSounds );
			MixMixedAlways( sounds, vListener );
			MixMixedWithDelta( sounds, vListener );
			MixSingle( sounds, vListener );
		}
	}
	curTime = pGameTimer->GetAbsTime() + 1000000;
}

void CSoundScene2D::MixInterfaceSounds()
{
	for ( CHearableSounds::iterator it = interfaceSounds.begin(); it != interfaceSounds.end(); ++it )
	{
		// clear finished
		for ( CSoundsList::iterator soundIter = (*it).second.begin(); soundIter != (*it).second.end(); )
		{
			CSound * pSound = (*soundIter);
			if ( pSound->IsTimeToFinish() ||
					( pSound->IsMarkedStarted() && !pSFX->IsPlaying( pSound->GetSound() ) ) )
			{
				pSound->UnSubstitute();
				const WORD wID = pSound->GetID();

				if ( wID && deletedInterfaceSounds.end() == deletedInterfaceSounds.find( wID ) )
					finishedInterfaceSounds.insert( pSound->GetID() );
				soundIter = (*it).second.erase( soundIter );
			}
			else
				++soundIter;
		}

		// mix not started

		//отсортировать их по времени старта.
		CSoundStartTimePredicate pr;
		(*it).second.sort( pr );

		// найти порции звуков, у которых разница во времени меньше Delta 
		// и скормить их Mix()
		CSoundsList::iterator beginIterator = (*it).second.begin();
		CSoundsList::iterator endIterator = (*it).second.begin();
		while( (*it).second.end() != beginIterator )
		{
			CSound *s = (*beginIterator);
			const NDb::SSoundDesc *pSubst = s->GetDesc();
			CSoundsWithinDeltaPredicate pr( s->GetBeginTime(), SSoundSceneConsts::SS_MIX_DELTA );
			endIterator = find_if( beginIterator, (*it).second.end(), pr );
			Mix( (*it).second, beginIterator, endIterator, pSubst, VNULL3, SFX_INTERFACE, 1, false );
			beginIterator = endIterator;
		}
	}
}

void CSoundScene2D::MixSingle( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vListener )
{
	//оставшиеся запустить на проигрышь без смешивания.
	for ( CHearableSounds::iterator substIter = sounds.begin(); substIter != sounds.end(); ++substIter )
	{
		const NDb::SSoundDesc *pSubst = (*substIter).first;
		CSoundsList & curSounds = (*substIter).second;

		CSoundsList::iterator beginIterator = curSounds.begin();
		CSoundsList::iterator endIterator = curSounds.begin();
		while( curSounds.end() != beginIterator )
		{
			CSoundsList::iterator temp = beginIterator;
			endIterator = ++temp;
			Mix( curSounds, beginIterator, endIterator, pSubst, vListener, SFX_MIX_ALL, 0 );
			beginIterator = endIterator;
		}
	}
}

void CSoundScene2D::MixMixedWithDelta( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vListener )
{
	// 2) выбрать звуки, которые можно объединять если начало разнесено во времени.
				//сделать замену.
	for ( CHearableSounds::iterator substIter = sounds.begin(); substIter != sounds.end(); ++substIter )
	{
		const NDb::SSoundDesc *pSubst = (*substIter).first;

		//отсортировать их по времени старта.
		CSoundsList & curSounds = (*substIter).second;
		CSoundStartTimePredicate pr;
		curSounds.sort( pr );

		// найти порции звуков, у которых разница во времени меньше Delta 
		// и скормить их Mix()
		CSoundsList::iterator beginIterator = curSounds.begin();
		CSoundsList::iterator endIterator = curSounds.begin();
		while( curSounds.end() != beginIterator )
		{
			CSound * s = (*beginIterator);
			CSoundsWithinDeltaPredicate pr( s->GetBeginTime(), SSoundSceneConsts::SS_MIX_DELTA );
			endIterator = find_if( beginIterator, curSounds.end(), pr );
			Mix( curSounds, beginIterator, endIterator, pSubst, vListener, SFX_MIX_IF_TIME_EQUALS, 2 );
			beginIterator = endIterator;
		}
	}
}

void CSoundScene2D::MixMixedAlways( CSoundScene2D::CHearableSounds & sounds, const CVec3 & vListener )
{
	// 1) выбрать звуки, которые играть как один всегда, просчитать им координаты.
				//запустить на проигрыш.
	for ( CHearableSounds::iterator substIter = sounds.begin(); substIter != sounds.end(); ++substIter )
	{
		CSoundsList & curSounds = (*substIter).second;
		const NDb::SSoundDesc *pSubst = (*substIter).first;
		Mix( curSounds, curSounds.begin(), curSounds.end(), pSubst, vListener, SFX_MIX_ALWAYS, 1 );
	}
}

void CSoundScene2D::PlaySubstSound( ISound *pSubstSound, unsigned int nStartSample, unsigned int nSoundLenght, bool bLooped )
{
	if ( nSoundLenght )
	{	
		if ( bLooped )
		{
			nStartSample = nStartSample % nSoundLenght ;
			if ( nStartSample > nSoundLenght * 8 / 10 )
				nStartSample = 0;
		}

		if ( !pSFX->IsPlaying( pSubstSound ) && ( nStartSample < nSoundLenght ) )
		{
			// возобновить проигрыш звука 
			const int nChannel = pSFX->PlaySample( pSubstSound, bLooped, nStartSample );
		}
		else
			pSFX->UpdateSample( pSubstSound );
	}
}

void CSoundScene2D::Mix(	CSoundsList & curSounds,
												const CSoundsList::iterator begin_iter,
												const CSoundsList::iterator end_iter,
												const NDb::SSoundDesc *pSubst,
												const CVec3 &vListener,
												const ESoundMixType eMixType,
												const int nMixMinimum,
												bool bDelete ) 
{
	CVec3 vSoundCoord( VNULL3 );					// координата звука относительно камеры
	
	float fPan = 0;
	float fVolume = 0;
	float fMaxVolume = 0;
	float fMaxHearRadius = 0;

	CPtr<CSubstSound> pSubstStruct;
	CSound *pSound = 0;										// первый же играющий звук
	int nSounds = 0;
	NTimer::STime nStartTime = 0;
	bool bLooped = false;

	for ( CSoundsList::iterator soundsIter = begin_iter; soundsIter != end_iter; ++soundsIter )
	{
		CSound &sound = *(*soundsIter);
		if ( SFX_MIX_ALL == eMixType || sound.GetMixType() == eMixType )
		{
			const CVec3 v ( sound.GetPos() - vListener );
			const float fVol = sound.GetVolume( CSoundScene2D::GetCurTime(), fabs( v ) );
			fMaxVolume += fVol;
			vSoundCoord += v * fVol;
			
			++nSounds;
			bLooped |= sound.IsLooped(); // замена зациклена если хоть один звучок зациклен
			if ( !pSound ) pSound = &sound;

			if ( !pSubstStruct && sound.IsSubstituted() && sound.IsMarkedStarted() ) 
			{	// звук - уже заменени и он уже играет
				pSubstStruct = sound.GetSubst();
				nStartTime = sound.GetBeginTime();
			}
		}
	}

	if ( nSounds != 0 && nSounds >= nMixMinimum )
	{
		vSoundCoord /= nSounds;
		
		if ( !pSubstStruct )
		{
			// замену создать заново и запустить на проигрыш
			CPtr<ISound> pSubstituteSound = CSoundManager::CreateSound2D( pSubst, pSound->IsLooped() );
			if ( pSubstituteSound == 0 ) // значит звук замены не задан
				pSubstituteSound = pSound->GetSound();
			pSubstStruct = new CSubstSound( pSubstituteSound );
			pSubstituteSound->SetVolumeType( pSound->GetSound()->GetVolumeType() );
		}

		// СДЕЛАТЬ ЗАМЕНУ ЗВУКАМ И УДАЛИТЬ ИХ ИЗ ВРЕМЕННОГО СПИСКА ЗВУКОВ
		unsigned int nStartSample = 0; // время для возобновления звука
		NTimer::STime nStartTime = 0;
		// вычислиить время старта ( в самплах )
		for ( CSoundsList::iterator soundsIter = begin_iter; soundsIter != end_iter; ++soundsIter)
		{
			CSound & sound = *(*soundsIter);
			if ( SFX_MIX_ALL == eMixType || sound.GetMixType() == eMixType )
			{
				nStartSample = (std::max)( nStartSample, sound.GetSamplesPassed() );
				const NTimer::STime tmp = sound.GetBeginTime();
				if ( tmp < nStartTime || nStartTime == 0 )
					nStartTime = tmp; 
			}
		}

		// все заменить
		for ( CSoundsList::iterator soundsIter = begin_iter; soundsIter != end_iter; )
		{
			CSound & sound = *(*soundsIter);
			if ( SFX_MIX_ALL == eMixType || sound.GetMixType() == eMixType )
			{
				if ( !sound.IsSubstituted() )
				{
					pSFX->StopSample( sound.GetSound() );
					sound.Substitute( pSubstStruct, nStartTime );
				}
				sound.MarkStarted();
				if ( bDelete )
					soundsIter = curSounds.erase( soundsIter );
				else
					++soundsIter;
			}
			else
				++soundsIter;
		}

		// проапдейтить звук в соответствии с тем, что намикшировали
		fMaxHearRadius = 1.0f * pSound->GetRadiusMax() * SSoundSceneConsts::SS_SOUND_CELL_SIZE * SSoundSceneConsts::SS_TILE_SIZE;
		CalcVolNPan( &fVolume, &fPan, vSoundCoord, fMaxHearRadius );

		fVolume *= fMaxVolume / nSounds;
		ISound * pSubstSound = pSubstStruct->GetSound();

		pSubstSound->SetPan( fPan );
		pSubstSound->SetVolume( fVolume );
		
		PlaySubstSound( pSubstSound, nStartSample, pSubstSound->GetLenght(), bLooped );
	}
}

void CSoundScene2D::MuteSounds( CSoundsList	* muteSounds )
{
// замолчать все неслышные звуки.
	// ---------------------------------------------
	for ( CSoundsList::iterator it = muteSounds->begin(); it != muteSounds->end(); ++it )
	{
		CSound *sound = (*it);
		if ( !sound->IsMarkedFinished() )
		{
			sound->UnSubstitute();
			pSFX->StopSample( sound->GetSound() );
		}
	}
}

void CSoundScene2D::CalcVolNPan( float *fVolume, float *fPan, const CVec3 &_vSound, const float fMaxHear )
{
	*fVolume = 1.0f - fabs( _vSound ) / fMaxHear;
	*fPan =  ( _vSound.x * vFormerCameraDir.y - _vSound.y * vFormerCameraDir.x ) / fMaxHear;
	if ( *fVolume < 0.0f )
	{
		*fVolume = 0.0f;
		*fPan = 0.0f;
	}
}

WORD CSoundScene2D::AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos )
{
	return mapSounds.AddSound( vPos, pDesc );
}

void CSoundScene2D::RemoveSoundFromMap( const WORD	wInstanceID )
{
	mapSounds.RemoveSound( wInstanceID );
}

ISoundScene* CreateSoundScene()
{
	return new CSoundScene();
}

void CSoundScene2D::Init() 
{ 
	pSFX = Singleton<ISFX>();
}


// CSoundScene3D


void CSoundScene3D::Init()
{
	pGameTimer = Singleton<IGameTimer>();
	pSFX = Singleton<ISFX>();
	timeLastPosUpdate = pGameTimer->GetAbsTime();

	pSFX->Set3DMode( true );
}

void CSoundScene3D::Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE )
{
	SSoundSceneConsts::SS_TILE_SIZE = _VIS_TILE_SIZE;
	Init();
	currentUnderUpdate = forgottenSounds.end();
}

WORD CSoundScene3D::AddSound( 	const NDb::SComplexSoundDesc *pStats,
							const CVec3 &vPos,
							const enum ESoundMixType eMixMode,
							const enum ESoundAddMode eAddMode,
							const unsigned int nTimeAfterStart,
							int nVolumeType )
{
	if ( !pSFX->IsSFXEnabled() || 0 == pStats ) 
		return 0;

	const NDb::SComplexSoundDesc::SSoundStats *pRandomSound = pStats->GetRandomSound();

	if ( !pRandomSound )
		return 0;

	if ( !pRandomSound->pPathName ) // empty sound rolled, don't add it to SoundScene
		return 0;

	const bool bLooped = eAddMode == SAM_LOOPED_NEED_ID;
	const bool bNeedID = eAddMode == SAM_NEED_ID || eAddMode == SAM_LOOPED_NEED_ID;

	WORD wID = 0;
	CPtr<ISound> pSound;
	const NTimer::STime curTime = pGameTimer->GetAbsTime();
	if ( eMixMode == SFX_INTERFACE )
	{
		pSound = CSoundManager::CreateSound2D( pRandomSound->pPathName, bLooped );
#ifndef _FINALRELEASE
		if ( nVolumeType == 1 )
			theAckTuning.Add( pRandomSound->pPathName->szSoundPath );
#endif

		if ( pSound )
		{
			pSound->SetPan( 0.0f );
			pSound->SetVolumeType( nVolumeType );
			pSound->SetVolume( 1.0f );
			if ( bNeedID )
			{
				wID = freeIDs.Get();
				interfaceSounds[wID].first = pSound;
				interfaceSounds[wID].second = curTime;
			}
			else
			{
				forgottenSounds.push_back( CSoundWithStartTime(pSound, curTime) );
			}
		}
	}
	else
	{
		pSound = CSoundManager::CreateSound3D( pRandomSound->pPathName, bLooped );

		if (!pSound)
			return 0;

#ifndef _FINALRELEASE
		if ( nVolumeType == 1 )
			theAckTuning.Add( pRandomSound->pPathName->szSoundPath );
#endif
		pSound->SetPos( vPos );
		pSound->SetVolumeType( nVolumeType );
		pSound->SetMinMax( pRandomSound->fMinDist * SSoundSceneConsts::SS_TILE_SIZE, pRandomSound->fMaxDist * SSoundSceneConsts::SS_TILE_SIZE);
		if ( pSound )
		{
			if ( bNeedID )
			{
				wID = freeIDs.Get();
				movingSounds[wID].first = pSound;
				movingSounds[wID].second = curTime;
			}
			else
				forgottenSounds.push_back( CSoundWithStartTime( pSound, curTime ) );
		}
	}
	if ( pSound )
		pSFX->PlaySample( pSound, bLooped, nTimeAfterStart );

	return wID;
}

void CSoundScene3D::RemoveSound( const WORD wID )
{
	{
	CInterfaceSounds::iterator pos = interfaceSounds.find( wID );

	if ( pos != interfaceSounds.end() )
	{
		CPtr<ISound> pSound = pos->second.first;
		interfaceSounds.erase( wID );
		freeIDs.Return( wID );
		pSFX->StopSample( pSound );
		return;
	}
	}
	{
	CMovingSounds::iterator pos = movingSounds.find( wID );
	if ( pos != movingSounds.end() )
	{
		CPtr<ISound> pSound = pos->second.first;
		movingSounds.erase( wID );
		freeIDs.Return( wID );
		pSFX->StopSample( pSound );
		return;
	}
	}
}

void CSoundScene3D::SetSoundPos( const WORD wID, const class CVec3 &_vPos )
{
	CMovingSounds::iterator pos = movingSounds.find( wID );
	if ( pos != movingSounds.end() )
	{
		CPtr<ISound> pSound = pos->second.first;

		const NTimer::STime curTime = pGameTimer->GetAbsTime();

		int nTime = curTime - timeLastPosUpdate;
		if ( nTime != 0 )
			pSound->SetSpeed( ( _vPos - pSound->GetPos() ) / nTime );
		pSound->SetPos( _vPos );
		pSFX->UpdateSample( pSound );
	}
}

bool CSoundScene3D::IsSoundFinished( const WORD wID ) const
{
	{
		CInterfaceSounds::const_iterator pos = interfaceSounds.find( wID );

		if ( pos != interfaceSounds.end() )
		{
			CPtr<ISound> pSound = pos->second.first;
			if ( pSFX->IsPlaying( pSound ) )
				return false;
		}
	}
	{
		CMovingSounds::const_iterator  pos = movingSounds.find( wID );
		if ( pos != movingSounds.end() )
		{
			CPtr<ISound> pSound = pos->second.first;
			if ( pSFX->IsPlaying( pSound ) )
				return false;
		}
	}
	return true;
}

void CSoundScene3D::UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius )
{
	const NTimer::STime curTime = CSoundScene2D::GetCurTime();
	pSFX->Update( vListener, vCameraDir, timeLastPosUpdate - curTime );
	if ( bLaunchAfterLoad && !pSFX->IsPaused() )
	{
		LaunchAfterLoad();
		bLaunchAfterLoad = false;
	}
	// look at forgotten sounds, some may be finished
	for ( int nUpdated = 0; currentUnderUpdate != forgottenSounds.end() && nUpdated < 10; ++nUpdated )
	{
		++nUpdated;

		if ( !pSFX->IsPlaying( currentUnderUpdate->first ) )
			currentUnderUpdate = forgottenSounds.erase( currentUnderUpdate );
		else
			++currentUnderUpdate;
	}
	if ( currentUnderUpdate == forgottenSounds.end() )
		currentUnderUpdate = forgottenSounds.begin();

	timeLastPosUpdate = curTime;
}

void CSoundScene3D::SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode )
{
}

WORD CSoundScene3D::AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos )
{
	return 0;
}

void CSoundScene3D::RemoveSoundFromMap( const WORD	wInstanceID )
{
}

void CSoundScene3D::ClearSounds()
{
	freeIDs.Clear();

	if ( IsValid( pSFX  ) )
	{
		for ( CMovingSounds::iterator it = movingSounds.begin(); it != movingSounds.end(); ++it )
		{
			if ( IsValid( it->second.first ) )
				pSFX->StopSample( it->second.first );
		}

		for ( CInterfaceSounds::iterator it = interfaceSounds.begin(); it != interfaceSounds.end(); ++it )
		{
			if ( IsValid( it->second.first ) )
				pSFX->StopSample( it->second.first );
		}

		for ( CForgottenSounds::iterator it = forgottenSounds.begin(); it != forgottenSounds.end(); ++it )
		{
			if ( IsValid( it->first ) )
				pSFX->StopSample( it->first );
		}
	}

	movingSounds.clear();
	interfaceSounds.clear();
	forgottenSounds.clear();
	currentUnderUpdate = forgottenSounds.end();
}

void LaunchSound( ISFX *pSFX, ISound *pSound, int nTimePassed, bool bLooped )
{
	pSFX->PlaySample( pSound, bLooped, nTimePassed * pSound->GetSampleRate() / 1000 );
}

void CSoundScene3D::LaunchAfterLoad()
{
	const NTimer::STime curTime = pGameTimer->GetAbsTime();

	for ( CMovingSounds::iterator it = movingSounds.begin(); it != movingSounds.end(); ++it )
		LaunchSound( pSFX, it->second.first, curTime - it->second.second, true );

	for ( CInterfaceSounds::iterator it = interfaceSounds.begin(); it != interfaceSounds.end(); ++it )
		LaunchSound( pSFX, it->second.first, curTime - it->second.second, false );

	for ( CForgottenSounds::iterator it = forgottenSounds.begin(); it != forgottenSounds.end(); ++it )
	{
		int nPlayedSoFar = curTime - it->second;
		if ( nPlayedSoFar * it->first->GetSampleRate() / 1000 < it->first->GetLenght() )
			LaunchSound( pSFX, it->first, nPlayedSoFar, false );
	}
}


// CSoundScene


bool CSoundScene::b3DSound = false;

void CSoundScene::Init()
{
	pScene = new CSoundScene2D;
	pScene->Init();
}

void CSoundScene::Init( const int _nMaxX, const int _nMaxY, const int _nMinZ, const int _nMaxZ, const int _VIS_TILE_SIZE )
{
	//b3DSound = s_bSound5_1;//NGlobal::GetVar( "sound_5.1", false );
	b3DSound = false;	// force disable 3D sound! - it's buggy!
	if ( b3DSound )
	{
		pScene = new CSoundScene3D;
	}
	else
		pScene = new CSoundScene2D;
	pScene->Init();
	pScene->SetSoundSceneMode( eMode );
	pScene->Init( _nMaxX, _nMaxY, _nMinZ, _nMaxZ, _VIS_TILE_SIZE );
}

WORD CSoundScene::AddSound( 	const NDb::SComplexSoundDesc *pStats,
							const CVec3 &vPos,
							const enum ESoundMixType eMixMode,
							const enum ESoundAddMode eAddMode,
							const unsigned int nTimeAfterStart,
							int nVolumeType )
{
	return pScene->AddSound( pStats, vPos, eMixMode, eAddMode, nTimeAfterStart, nVolumeType );
}

WORD CSoundScene::AddSound( 	const NDb::SComplexSoundDesc *pStats,
							CFuncBase<CVec3> *pPos, // AI pixels
							const enum ESoundMixType eMixMode,
							const enum ESoundAddMode eAddMode,
							const unsigned int nTimeAfterStart,
							int nVolumeType )
{
	ASSERT( eAddMode == SAM_NEED_ID || eAddMode == SAM_LOOPED_NEED_ID );
	CDGPtr<CFuncBase<CVec3> > pSoundPos = pPos;
	pSoundPos.Refresh();
	WORD wSoundID = pScene->AddSound( pStats, pSoundPos->GetValue(), eMixMode, eAddMode, nTimeAfterStart, nVolumeType );
	dynamicSounds[wSoundID] = pPos;
	return wSoundID;
}

void CSoundScene::RemoveSound( const WORD wID )
{
	if ( !dynamicSounds.empty() )
		dynamicSounds.erase( wID );
	pScene->RemoveSound( wID );
}

void CSoundScene::SetSoundPos( const WORD wID, const class CVec3 &vPos )
{
	pScene->SetSoundPos( wID, vPos );
}

bool CSoundScene::IsSoundFinished( const WORD wID ) const
{
	return pScene->IsSoundFinished( wID );
}

void CSoundScene::UpdateSound( const CVec3 &vListener, const CVec3 &vCameraDir, const float fViewRadius )
{
//	DebugTrace( "vListener = %f, %f, %f", vListener.x, vListener.y, vListener.z );
	for ( CDynamicSoundsMap::iterator it = dynamicSounds.begin(); it != dynamicSounds.end(); )
	{
		if ( IsValid( it->second ) )
		{
			it->second.Refresh();
			SetSoundPos( it->first, it->second->GetValue() );
			++it;
		}
		else
			dynamicSounds.erase( it++ );
	}
	pScene->UpdateSound( vListener, vCameraDir, fViewRadius );
}

void CSoundScene::SetSoundSceneMode( const enum ESoundSceneMode eSoundSceneMode )
{
	eMode = eSoundSceneMode;
	pScene->SetSoundSceneMode( eSoundSceneMode );
}

WORD CSoundScene::AddSoundToMap( const NDb::SComplexSoundDesc* pDesc, const CVec3 &vPos )
{
	return pScene->AddSoundToMap( pDesc, vPos );
}

void CSoundScene::RemoveSoundFromMap( const WORD	wInstanceID )
{
	pScene->RemoveSoundFromMap( wInstanceID );
}

void CSoundScene::ClearSounds()
{
	pScene->ClearSounds();
}

bool CSoundScene::Is3D()
{
	return b3DSound;
}

START_REGISTER(SoundSceneInternalCommands)
REGISTER_VAR_EX( "Sound.Use3DSound", NGlobal::VarBoolHandler, &s_bSound5_1, false, STORAGE_USER );

FINISH_REGISTER

