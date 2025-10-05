#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "UI/UI.h"
#include "Misc/StrProc.h"
#include "DebugStatsSystem.h"

namespace NDebugInfo
{

void UpdateEntry( const std::string &szName, const std::string &szValue, const DWORD dwColor )
{
	IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
	if ( pDebug )
	{
		IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
			if ( pStatsSystemWindow )
				pStatsSystemWindow->UpdateEntry( NStr::ToUnicode( StrFmt( "%s", szName.c_str() ) ), NStr::ToUnicode( StrFmt( "%s", szValue.c_str() ) ), dwColor );
	}
}

}


