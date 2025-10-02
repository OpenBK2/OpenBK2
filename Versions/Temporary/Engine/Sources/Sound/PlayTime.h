#pragma once

namespace NDb
{
	struct SPlayTime;
	struct SMusicTrack;
}

namespace NMusicSystem
{

class CPlayTime
{
	ZDATA
	int nPlayTime;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nPlayTime); return 0; }
public:
	CPlayTime() {  }
	CPlayTime( const NDb::SPlayTime *_PlayTime, const NDb::SMusicTrack *pTrack );
	NTimer::STime GetPlayTime() const;
};
}

