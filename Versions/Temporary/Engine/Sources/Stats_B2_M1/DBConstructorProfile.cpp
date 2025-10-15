// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbconstructorprofile.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{



void SDBGunsProfile::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Attached", (uint8_t*)&bAttached - pThis, sizeof(bAttached), NTypeDef::TYPE_TYPE_BOOL );
}

int SDBGunsProfile::operator&( IXmlSaver &saver )
{
	saver.Add( "Attached", &bAttached );

	return 0;
}

int SDBGunsProfile::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bAttached );

	return 0;
}

uint32_t SDBGunsProfile::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << bAttached;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDBPlatformsProfile::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Attached", (uint8_t*)&bAttached - pThis, sizeof(bAttached), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Guns", &guns, pThis );
}

int SDBPlatformsProfile::operator&( IXmlSaver &saver )
{
	saver.Add( "Attached", &bAttached );
	saver.Add( "Guns", &guns );

	return 0;
}

int SDBPlatformsProfile::operator&( IBinSaver &saver )
{
	saver.Add( 2, &bAttached );
	saver.Add( 3, &guns );

	return 0;
}

uint32_t SDBPlatformsProfile::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << bAttached << guns;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDBConstructorProfile::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DBConstructorProfile", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Platforms", &platforms, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Slots", &slots, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SDBConstructorProfile::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Platforms", &platforms );
	saver.Add( "Slots", &slots );

	return 0;
}

int SDBConstructorProfile::operator&( IBinSaver &saver )
{
	saver.Add( 2, &platforms );
	saver.Add( 3, &slots );

	return 0;
}

uint32_t SDBConstructorProfile::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << platforms << slots;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x3013ECC0, SDBConstructorProfile )

