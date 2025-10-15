// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBVisObj.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{



void SVisObj::SSingleObj::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Model", (uint8_t*)&pModel - pThis, sizeof(pModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LowLevelModel", (uint8_t*)&pLowLevelModel - pThis, sizeof(pLowLevelModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (uint8_t*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
}

int SVisObj::SSingleObj::operator&( IXmlSaver &saver )
{
	saver.Add( "Model", &pModel );
	saver.Add( "LowLevelModel", &pLowLevelModel );
	saver.Add( "Season", &eSeason );

	return 0;
}

int SVisObj::SSingleObj::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pModel );
	saver.Add( 3, &pLowLevelModel );
	saver.Add( 4, &eSeason );

	return 0;
}

uint32_t SVisObj::SSingleObj::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVisObj::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "VisObj", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "ForceAnimated", (uint8_t*)&bForceAnimated - pThis, sizeof(bForceAnimated), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructArrayMetaInfo( "Models", &models, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SVisObj::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "ForceAnimated", &bForceAnimated );
	saver.Add( "Models", &models );

	return 0;
}

int SVisObj::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bForceAnimated );
	saver.Add( 3, &models );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x11073C40, SVisObj )

