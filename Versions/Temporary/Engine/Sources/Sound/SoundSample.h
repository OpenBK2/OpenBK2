#pragma once
#include "../vendor/fmod/api/inc/fmod.h"
#include "..\System\GResource.h"


class CSoundSample : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CSoundSample );
	static bool b3DSoundShare;
	//
	FSOUND_SAMPLE *sample;								// FMOD sound sample
	CDBID dbidSound;
	//
	void Close();
	void SetSample( FSOUND_SAMPLE *_sample );
public:
	static void Set3DMode( bool b3DMode ) { b3DSoundShare = b3DMode; }
	CSoundSample();
	~CSoundSample();
	int operator&( IBinSaver &saver );
#if !defined(_FINALRELEASE)
	string GetName() const { return dbidSound.ToString(); }
#endif
	//
	FSOUND_SAMPLE* GetInternalContainer();
	void SetLoop( bool bEnable );
	void SetKey( const CDBID &dbid );
};

