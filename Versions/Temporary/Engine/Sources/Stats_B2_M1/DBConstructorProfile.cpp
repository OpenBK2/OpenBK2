// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbconstructorprofile.h"

namespace NDb
{



void SDBGunsProfile::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Attached", (BYTE*)&bAttached - pThis, sizeof(bAttached), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SDBGunsProfile::CalcCheckSum() const
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



void SDBPlatformsProfile::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Attached", (BYTE*)&bAttached - pThis, sizeof(bAttached), NTypeDef::TYPE_TYPE_BOOL );
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

DWORD SDBPlatformsProfile::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
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

DWORD SDBConstructorProfile::CalcCheckSum() const
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
REGISTER_DATABASE_CLASS( 0x3013ECC0, SDBConstructorProfile ) 

