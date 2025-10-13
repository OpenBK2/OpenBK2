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

const unsigned long SCheats::MakeCheckSum( const std::string &_szPassword )
{
/*
//	string szPassword = "123456";
	string szPassword = "654321";

	vector<uint8_t> checksum;
	checksum.reserve( 100 );
	checksum.insert( checksum.end(), szPassword.begin(), szPassword.end() );
	checksum.insert( checksum.end(), s_cKey2, s_cKey2 + s_nKey2Length );
	const uLong uCheckSum = crc32( 0L, &(checksum[0]), checksum.size() );

	return uCheckSum;
*/
	return 0;
}

void SCheats::CheckPassword( const std::string &szPassword )
{
	std::vector<uint8_t> checksum;
	checksum.reserve( 100 );
	checksum.insert( checksum.end(), szPassword.begin(), szPassword.end() );
	checksum.insert( checksum.end(), s_cKey2, s_cKey2 + s_nKey2Length );
	const uLong uCheckSum = crc32( 0L, &(checksum[0]), checksum.size() );

	bPasswordOK = ( uCheckSum == ulPass );
	if ( bPasswordOK )
		NGlobal::SetVar( "VVP", 1 );
}


