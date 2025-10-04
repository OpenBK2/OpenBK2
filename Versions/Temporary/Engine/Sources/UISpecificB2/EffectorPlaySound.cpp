#include "stdafx.h"
#include "System/Dg.h"
#include "effectorplaysound.h"
#include "Sound/SoundScene.h"
#include "DBUISpecificB2.h"

REGISTER_SAVELOAD_CLASS(0x11075C02,CEffectorPlaySound)

int CEffectorPlaySound::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pSound );
	saver.Add( 2, &bForward );
	saver.Add( 3, &bFinished );
	return 0;
}

bool CEffectorPlaySound::IsFinished() const 
{ 
	return bFinished;
}

void CEffectorPlaySound::Configure( const NDb::SUIStateBase *_pCmd, struct IScreen *pScreen, SWindowContext *pContext, const string &szAnimatedWindow )
{ 
	const NDb::SUISPlaySound *pCmd( checked_cast<const NDb::SUISPlaySound*>( _pCmd ) );
	CParam<CDBPtr<NDb::SComplexSoundDesc> > pS( pCmd->pSoundToPlay );
	if ( pContext )
		pS.Merge( pContext->pSoundToPlay );
	NI_ASSERT( pS.IsValid(), "SOUND to play is invalid" );
	pSound = pS.Get();
}

const int CEffectorPlaySound::Segment( const int timeDiff, struct IScreen *pScreen, const bool bFastForward )
{ 
	Singleton<ISoundScene>()->AddSound( pSound, VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET, bFastForward ? timeDiff : 0, 2 );
	bFinished = true;
	return 0;
}

void CEffectorPlaySound::Reverse()
{
	bForward = !bForward;
	bFinished = false;
}

