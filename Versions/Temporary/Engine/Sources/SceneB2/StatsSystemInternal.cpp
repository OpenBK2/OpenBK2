#include "stdafx.h"

#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "./statssysteminternal.h"
#include "UI/UI.h"
#include "Misc/StrProc.h"

#include <cstdint>

void CStatsSystem::UpdateEntry( const std::string &szName, const std::string &szEntry, const uint32_t dwColor )
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


