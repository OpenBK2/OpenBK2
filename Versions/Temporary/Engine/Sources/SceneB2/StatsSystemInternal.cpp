#include "stdafx.h"

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "StatsSystemInternal.h"
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

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x110AA3C0, CStatsSystem );


