#pragma once
#include "SFX.h"

// после использования звука заканчивает его проигрыш. для циклических
// звуков - они должны заканчиваться только когда их никто не играет
class CSubstSound : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CSubstSound);
	CPtr<ISFX> pSFX;
	CPtr<ISound> pSample;
public:
	CSubstSound() {  }
	CSubstSound( ISound *pSound )
		:pSample( pSound )
	{
		pSFX = Singleton<ISFX>();
	}
	virtual ~CSubstSound()
	{
		if ( pSFX )
			pSFX->StopSample( pSample );
	}
	ISound * GetSound()
	{
		return pSample;
	}
	int operator&( IBinSaver &saver ); 
};


