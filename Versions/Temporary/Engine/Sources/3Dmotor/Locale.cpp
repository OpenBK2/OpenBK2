#include "stdafx.h"
#include "Locale.h"
#include "GLocale.h"

namespace NGScene
{

CTextLocaleInfo *GetTextLocaleInfo()
{
	const int N_SINGLETON_ID = 0x2016EB40;
	CTextLocaleInfo *pRes = checked_cast<CTextLocaleInfo*>( NSingleton::Singleton( N_SINGLETON_ID ) );
	if ( pRes == 0 )
	{
		pRes = new CTextLocaleInfo;
		NSingleton::RegisterSingleton( pRes, N_SINGLETON_ID );
	}
	return pRes;
}


}

