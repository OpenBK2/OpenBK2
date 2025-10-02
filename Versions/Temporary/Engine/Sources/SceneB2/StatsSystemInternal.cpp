#include "StdAfx.h"

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "./statssysteminternal.h"
#include "../UI/UI.h"
#include "../Misc/StrProc.h"

void CStatsSystem::UpdateEntry( const string &szName, const string &szEntry, const DWORD dwColor )
{
	IStatsSystemWindow *pStats = Singleton<IDebugSingleton>()->GetStatsWindow();
	if ( pStats )
		pStats->UpdateEntry( NStr::ToUnicode( szName ), NStr::ToUnicode( szEntry ), dwColor );
}

int CStatsSystem::operator&( IBinSaver &saver )
{
	return 0;
}

IStatSystem *CreateStatSystem()
{
	return new CStatsSystem;
}

REGISTER_SAVELOAD_CLASS( 0x110AA3C0, CStatsSystem );

