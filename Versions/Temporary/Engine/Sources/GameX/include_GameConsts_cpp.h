// Included into the generated DBConsts.cpp by the [cppExternal] include in
// DBConsts.cll: the definitions of the SGameConsts methods declared in
// include_GameConsts.h, which the generator has no way to write itself.
#pragma once

#include "AILogic/SimpleChecksumCalc.h"
#include "DBMPConsts.h"
#include "Main/DBNetConsts.h"
#include "Main/MODs.h"
#include "AILogic/DBAIConsts.h"
#include "port/unicode.h"
#include "Stats_B2_M1/DBMapInfo.h"

#include <filesystem>

namespace NDb
{

uint32_t SGameConsts::GetMPDataVersionChecksum() const
{
	if ( !bMPDataVersionChecksumValid )
		return RecalculateMPDataVersionChecksum();
	return nMPDataVersionChecksum;
}

uint32_t SGameConsts::RecalculateMPDataVersionChecksum() const
{
	// Invalidate the map snapshot too: a new attempt must inspect its data again.
	bMPDataVersionChecksumValid = false;
	bMPMapChecksumValid = false;
	pMPChecksumMap = nullptr;
	uint32_t ret = 123321;

	// game.exe version
	auto gameVersionStr = WideToUTF8( NGlobal::GetVar( "code_version_number", "0.0.0.0" ).GetString() );
	int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
	sscanf( gameVersionStr.c_str(), "%d.%d.%d.%d", &v1, &v2, &v3, &v4 );
	ret = CalculateChecksum( ret, v1 * v1 + v1 + 1, v2 * v2 + v2 + 2, v3 * v3 + v3 + 3, v4 * v4 + v4 + 4 );

	// AI Consts checksum
	uint32_t ai_chksum = pAI->CalcCheckSum();
	ret = CalculateChecksum( ret, ai_chksum );

	// Net Consts checksum, port param discluded
	ret = CalculateChecksum( ret,
	                         pNet->nMaxLatency,
	                         pNet->nTimeToStartLagByNoSegmentData,
	                         pNet->nTimeToAllowDropByLag,
	                         pNet->nTimeOutTime,
	                         pNet->nTimeBWTimeOuts );

	// MP Consts checksum
	uint32_t mp_chksum = pMultiplayer->CalcCheckSum();
	ret = CalculateChecksum( ret, mp_chksum );

	// loaded mod folder path checksum
	NMOD::SMOD modDesc;
	NMOD::GetAttachedMOD( &modDesc );

	const char *loadedMod = modDesc.szRelativePath.c_str();
	std::filesystem::path relPath = loadedMod;
	auto mod_folder_name = relPath.string();
	if ( !mod_folder_name.empty() )
	{
		int gummy = 0x123456AF;
		for ( size_t i = 0; i < mod_folder_name.size(); i++ )
			gummy += mod_folder_name[i] * gummy + i * gummy + 0xFED321;
		ret = CalculateChecksum( ret, gummy );
	}

	nMPDataVersionChecksum = ret;
	bMPDataVersionChecksumValid = true;
	return nMPDataVersionChecksum;
}

uint32_t SGameConsts::GetMPDataVersionChecksumWithMap( CDBPtr<NDb::SMultiplayerMap> map ) const
{
	const uint32_t base = GetMPDataVersionChecksum();
	// Join-condition checks can repeat while peers connect. Hash this map once
	// per attempt; selecting another map also requires a fresh map checksum.
	if ( !bMPMapChecksumValid || pMPChecksumMap != map )
	{
		nMPDataVersionChecksumWithMap = CalculateChecksum( base, map->CalcCheckSum() );
		pMPChecksumMap = map;
		bMPMapChecksumValid = true;
	}
	return nMPDataVersionChecksumWithMap;
}

}
