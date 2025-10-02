#include "StdAfx.h"
#include "playtime.h"

#include "DBMusicSystem.h"
#include "../Misc/Win32Random.h"
#include "../vendor/fmod/api/inc/fmod.h"
#include "MusicSystem.hpp"
#include "../System/VFSOperations.h"

namespace NMusicSystem
{


//	CPlayTime


CPlayTime::CPlayTime( const NDb::SPlayTime *_pPlayTime, const NDb::SMusicTrack *pTrack )
: nPlayTime( 0 )
{
	if ( !pTrack || !_pPlayTime )
		return;
	const float fRandom ( NWin32Random::Random( 0.0f, 1.0f ) );
	if ( _pPlayTime->nNumer != 0 )
	{	
		CFileStream trackStream( NVFS::GetMainVFS(), pTrack->szMusicFileName );
		FSOUND_STREAM * pStreamingSound = OpenTrack( &trackStream );
//		NI_ASSERT( pStreamingSound != 0, StrFmt( "cannot open stream file %s", pTrack->szMusicFileName ) );
		if ( pStreamingSound )
		{
			nPlayTime = FSOUND_Stream_GetLengthMs( pStreamingSound ) * (_pPlayTime->nNumer + int( fRandom * _pPlayTime->nNumberRandom ) );
			FSOUND_Stream_Close( pStreamingSound );
		}
	}
	else if ( _pPlayTime->nPlayTime != 0 )
		nPlayTime = _pPlayTime->nPlayTime + int( _pPlayTime->nPlayTimeRandom * fRandom );
}

NTimer::STime CPlayTime::GetPlayTime(  ) const
{
	return nPlayTime;
}
}

