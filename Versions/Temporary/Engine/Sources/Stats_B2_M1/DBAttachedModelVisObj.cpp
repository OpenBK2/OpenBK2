// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbattachedmodelvisobj.h"

#include <cstdint>

namespace NDb
{



void SAttachedModelVisObj::SSDamageLevel::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (uint8_t*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
}

int SAttachedModelVisObj::SSDamageLevel::operator&( IXmlSaver &saver )
{
	saver.Add( "VisObj", &pVisObj );

	return 0;
}

int SAttachedModelVisObj::SSDamageLevel::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pVisObj );

	return 0;
}

uint32_t SAttachedModelVisObj::SSDamageLevel::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAttachedModelVisObj::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AttachedModelVisObj", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "visualObject", (uint8_t*)&pvisualObject - pThis, sizeof(pvisualObject), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "DamageLevels", &damageLevels, pThis );
	NMetaInfo::ReportMetaInfo( "AnimableModel", (uint8_t*)&pAnimableModel - pThis, sizeof(pAnimableModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TransportableModel", (uint8_t*)&pTransportableModel - pThis, sizeof(pTransportableModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "animdescs", &animdescs, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SAttachedModelVisObj::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "visualObject", &pvisualObject );
	saver.Add( "DamageLevels", &damageLevels );
	saver.Add( "AnimableModel", &pAnimableModel );
	saver.Add( "TransportableModel", &pTransportableModel );
	saver.Add( "animdescs", &animdescs );

	return 0;
}

int SAttachedModelVisObj::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pvisualObject );
	saver.Add( 3, &damageLevels );
	saver.Add( 4, &pAnimableModel );
	saver.Add( 5, &pTransportableModel );
	saver.Add( 6, &animdescs );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x3013FC00, SAttachedModelVisObj ) 

