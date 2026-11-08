#include "stdafx.h"

#include "DBMPConsts.h"
#include "Misc/StrProc.h"
#include "MPConnectionSettings.h"
#include "System/GlobalVars.h"

namespace NGameX
{
int GetMPGameVersion( const NDb::SMultiplayerConsts *pConsts )
{
	// Zero is a valid explicit protocol identifier; only negative values mean unset.
	if ( pConsts && pConsts->nNetGameVersion >= 0 )
	{
		return pConsts->nNetGameVersion;
	}
	return NGlobal::GetVar( "NetGameVersion", 1 );
}

SMPConnectionSettings GetMPConnectionSettings( const NDb::SMultiplayerConsts *pConsts )
{
	const std::string szAddress = pConsts && !pConsts->szNivalNetIP.empty()
	                                  ? pConsts->szNivalNetIP
	                                  : NStr::ToMBCS( NGlobal::GetVar( "NivalNetIP", "localhost" ) );
	const int nPort = pConsts && pConsts->nNivalNetPort > 0 ? pConsts->nNivalNetPort
	                                                        : static_cast<int>( NGlobal::GetVar( "NivalNetPort", 4200 ) );
	return { szAddress, nPort, GetMPGameVersion( pConsts ) };
}
} // namespace NGameX
