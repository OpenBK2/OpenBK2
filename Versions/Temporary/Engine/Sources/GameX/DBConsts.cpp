// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBConsts.h"
#include "include_GameConsts_cpp.h"

#include "GameX_export.h"

#include <cstdint>

namespace NDb
{



void SGameConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "GameConsts", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "AI", (uint8_t*)&pAI - pThis, sizeof(pAI), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Net", (uint8_t*)&pNet - pThis, sizeof(pNet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Client", (uint8_t*)&pClient - pThis, sizeof(pClient), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "UI", (uint8_t*)&pUI - pThis, sizeof(pUI), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Scene", (uint8_t*)&pScene - pThis, sizeof(pScene), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Multiplayer", (uint8_t*)&pMultiplayer - pThis, sizeof(pMultiplayer), NTypeDef::TYPE_TYPE_REF );
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

uint32_t SGameConsts::CalcCheckSum() const
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

}
using namespace NDb;
REGISTER_DATABASE_CLASS( GAMEX, 0x11074CC1, SGameConsts )
