#include "StdAfx.h"

#include "SoundManager.h"
#include "DBSoundDesc.h"
#include "Sound2D.h"

//CBasicShare<int, CSound2D> shareSound2D(120);

ISound* CSoundManager::CreateSound2D( const NDb::SSoundDesc *pDesc, const bool bLooped )
{
	if ( !pDesc || pDesc->szSoundPath.empty() ) return 0;

	CSound2D *pS = new CSound2D( pDesc, bLooped );
	return pS;
}

ISound* CSoundManager::CreateSound3D( const NDb::SSoundDesc *pDesc, const bool bLooped )
{
	if ( !pDesc || pDesc->szSoundPath.empty() ) return 0;

	CSound3D *pS = new CSound3D( pDesc, bLooped );
	return pS;
}

