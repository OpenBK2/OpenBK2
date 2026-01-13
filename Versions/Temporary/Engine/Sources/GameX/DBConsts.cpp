// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBConsts.h"
#include "AILogic/SimpleChecksumCalc.h"
#include "DBMPConsts.h"
#include "Main/DBNetConsts.h"
#include "AILogic/DBAIConsts.h"
#include "Misc/StringConversions.h"
#include "Stats_B2_M1/DBMapInfo.h"

namespace NDb
{



void SGameConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "GameConsts", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "AI", (BYTE*)&pAI - pThis, sizeof(pAI), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Net", (BYTE*)&pNet - pThis, sizeof(pNet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Client", (BYTE*)&pClient - pThis, sizeof(pClient), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "UI", (BYTE*)&pUI - pThis, sizeof(pUI), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Scene", (BYTE*)&pScene - pThis, sizeof(pScene), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Multiplayer", (BYTE*)&pMultiplayer - pThis, sizeof(pMultiplayer), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SGameConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "AI", &pAI );
	saver.Add( "Net", &pNet );
	saver.Add( "Client", &pClient );
	saver.Add( "UI", &pUI );
	saver.Add( "Scene", &pScene );
	saver.Add( "Multiplayer", &pMultiplayer );

	return 0;
}

int SGameConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pAI );
	saver.Add( 3, &pNet );
	saver.Add( 4, &pClient );
	saver.Add( 5, &pUI );
	saver.Add( 6, &pScene );
	saver.Add( 7, &pMultiplayer );

	return 0;
}

DWORD SGameConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pAI << pClient << pUI << pMultiplayer;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

DWORD SGameConsts::GetMPDataVersionChecksum() const
{
	DWORD ret = 123321;

	// AI Consts checksum
	ret = CalculateChecksum(ret, pAI->CalcCheckSum());

	// Net Consts checksum, port param discluded
	ret = CalculateChecksum(ret, pNet->nMaxLatency, pNet->nTimeToStartLagByNoSegmentData, pNet->nTimeToAllowDropByLag, pNet->nTimeOutTime, pNet->nTimeBWTimeOuts);

	// MP Consts checksum
	ret = CalculateChecksum(ret, pMultiplayer->CalcCheckSum());

	// game.exe version
	auto gameVersionStr = string_conversion::wstring_to_utf8(NGlobal::GetVar("code_version_number", "0.0.0.0").GetString());
	int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
	sscanf(gameVersionStr.c_str(), "%d.%d.%d.%d", &v1, &v2, &v3, &v4);
	ret = CalculateChecksum(ret, v1, v2, v3, v4);

	return ret;
}

DWORD SGameConsts::GetMPDataVersionChecksumWithMap(CDBPtr<NDb::SMultiplayerMap> map) const
{
	DWORD ret = GetMPDataVersionChecksum();
	
	ret = CalculateChecksum(ret, map->CalcCheckSum());

	return ret;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x11074CC1, SGameConsts ) 

