#include "stdafx.h"

#include "Cheats.h"
#include "Diplomacy.h"
#include "System/Commands.h"
#include "Misc/StrProc.h"

#include <zlib.h>

#include <cstdint>

SCheats theCheats;
extern CDiplomacy theDipl;
bool g_bDontShowWarFog;

void CheatsPassword( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
		return;
	//
	theCheats.CheckPassword( NStr::ToMBCS(paramsSet[0]) );
}

START_REGISTER(Cheats)

REGISTER_VAR_EX( "dont_show_warfog", NGlobal::VarBoolHandler, &g_bDontShowWarFog, false, STORAGE_NONE );
REGISTER_CMD( "password", CheatsPassword );

FINISH_REGISTER

void SCheats::Init()
{
	immortals.clear();
	immortals.resize( SAIConsts::MAX_NUM_OF_PLAYERS + 1, 0 );
	firstShoot.clear();
	firstShoot.resize( SAIConsts::MAX_NUM_OF_PLAYERS + 1, 0 );

	bWarFog = true;
	bLoadObjects = true;
	nPartyForWarFog = 0;
	bTurnOffWarFog = false;
	bHistoryPlaying = false;

	bPasswordOK = false;

#ifdef _FASTDEBUG
	bPasswordOK = true;
	NGlobal::SetVar( "VVP", 1 );
#endif
}

SCheats::SCheats()
{
	Init();
}

void SCheats::SetWarFog( bool _bWarFog )
{
	if ( !theDipl.IsNetGame() && bPasswordOK )
		bWarFog = _bWarFog;
}

void SCheats::SetNPartyForWarFog( const int _nPartyForWarFog, bool bUnconditionly )
{
	if ( !theDipl.IsNetGame() && bPasswordOK || bUnconditionly )
		nPartyForWarFog = _nPartyForWarFog;
}

void SCheats::SetLoadObjects( bool _bLoadObjects )
{
	if ( !theDipl.IsNetGame() && bPasswordOK )
		bLoadObjects = _bLoadObjects;
}

void SCheats::SetTurnOffWarFog( bool _bTurnOffWarFog )
{
	if ( !theDipl.IsNetGame() && bPasswordOK )
		bTurnOffWarFog = _bTurnOffWarFog;
}

bool SCheats::GetTurnOffWarFog() const
{ 
#ifdef _FINALRELEASE
	return bTurnOffWarFog;
#else
	return bTurnOffWarFog || g_bDontShowWarFog;
#endif
}

void SCheats::SetImmortals( const int nPlayer, const uint8_t cValue )
{
	if ( !theDipl.IsNetGame() && bPasswordOK )
		immortals[nPlayer] = cValue;
}

void SCheats::SetFirstShoot( const int nPlayer, const uint8_t cValue )
{
	if ( !theDipl.IsNetGame() && bPasswordOK )
		firstShoot[nPlayer] = cValue;
}

int SCheats::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &bWarFog );
		saver.Add( 2, &nPartyForWarFog );
		saver.Add( 3, &bLoadObjects );
		saver.Add( 4, &immortals );
		saver.Add( 5, &firstShoot );
		saver.Add( 6, &bTurnOffWarFog );
		saver.Add( 7, &bHistoryPlaying );
		saver.Add( 8, &bPasswordOK );
	}

	return 0;
}

const int s_nKey2Length = 20;
uint8_t s_cKey2[s_nKey2Length] = { /* [REMOVED_SECRET_KEY] */ };
//uLong ulPass = 3702409162;
uLong ulPass = 0; /* [REMOVED_SECRET_PASSWORD_HASH] */

// The single-player cheat password. Both halves of the check above were scrubbed
// before this source was released: s_cKey2 is 20 zero bytes and ulPass is 0. crc32
// of a non-empty buffer is never 0, so the comparison in CheckPassword could not
// succeed for any input at all, bPasswordOK stayed false for the whole run, and
// every cheat setter guarded by it - SetImmortals, SetFirstShoot, SetWarFog,
// SetLoadObjects, SetTurnOffWarFog - accepted its call and silently did nothing.
// The commented-out 3702409162 is the retail hash, but it was computed over the
// salt that is gone, so it cannot be matched any more. The plaintext password is
// known, so the expected checksum is rebuilt from it below, over whatever salt this
// build actually carries. Should a genuine s_cKey2/ulPass pair ever be restored,
// ulPass is non-zero again and takes precedence, so this stays a fallback.
static const char s_szCheatPassword[] = "Barbarossa";

// Checksum of a candidate password, exactly as the original check computed it:
// crc32 over the password bytes followed by the salt.
static uLong CalcPasswordCheckSum( const std::string &szPassword )
{
	std::vector<uint8_t> checksum;
	checksum.reserve( 100 );
	checksum.insert( checksum.end(), szPassword.begin(), szPassword.end() );
	checksum.insert( checksum.end(), s_cKey2, s_cKey2 + s_nKey2Length );

	return crc32( 0L, &( checksum[0] ), checksum.size() );
}

const unsigned long SCheats::MakeCheckSum( const std::string &_szPassword )
{
	// Was a commented-out stub that hashed a hardcoded "654321" and returned 0. It is
	// a developer helper for printing the hash of a candidate password, so it now
	// hashes the password it was handed.
	return CalcPasswordCheckSum( _szPassword );
}

void SCheats::CheckPassword( const std::string &szPassword )
{
	const uLong uCheckSum = CalcPasswordCheckSum( szPassword );
	// ulPass == 0 means this build carries no usable hash, see the comment above.
	const uLong uExpected = ( ulPass != 0 ) ? ulPass : CalcPasswordCheckSum( s_szCheatPassword );

	bPasswordOK = ( uCheckSum == uExpected );
	if ( bPasswordOK )
	{
		NGlobal::SetVar( "VVP", 1 );
	}
}


