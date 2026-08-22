// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "M1UnitSpecific.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{



void SM1UnitSpecific::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
}

int SM1UnitSpecific::operator&( IXmlSaver &saver )
{

	return 0;
}

int SM1UnitSpecific::operator&( IBinSaver &saver )
{

	return 0;
}

uint32_t SM1UnitSpecific::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitHelicopter::SHelicopterAxis::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Scaled", (uint8_t*)&pScaled - pThis, sizeof(pScaled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Dynamic", (uint8_t*)&pDynamic - pThis, sizeof(pDynamic), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LocatorName", (uint8_t*)&szLocatorName - pThis, sizeof(szLocatorName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "StartScaleSpeed", (uint8_t*)&fStartScaleSpeed - pThis, sizeof(fStartScaleSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HideStaticSpeed", (uint8_t*)&fHideStaticSpeed - pThis, sizeof(fHideStaticSpeed), NTypeDef::TYPE_TYPE_FLOAT );
}

int SM1UnitHelicopter::SHelicopterAxis::operator&( IXmlSaver &saver )
{
	saver.Add( "Scaled", &pScaled );
	saver.Add( "Dynamic", &pDynamic );
	saver.Add( "LocatorName", &szLocatorName );
	saver.Add( "StartScaleSpeed", &fStartScaleSpeed );
	saver.Add( "HideStaticSpeed", &fHideStaticSpeed );

	return 0;
}

int SM1UnitHelicopter::SHelicopterAxis::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pScaled );
	saver.Add( 3, &pDynamic );
	saver.Add( 4, &szLocatorName );
	saver.Add( 5, &fStartScaleSpeed );
	saver.Add( 6, &fHideStaticSpeed );

	return 0;
}

uint32_t SM1UnitHelicopter::SHelicopterAxis::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szLocatorName << fStartScaleSpeed << fHideStaticSpeed;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitHelicopter::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitHelicopter", typeID, sizeof(*this) );
	SM1UnitSpecific::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Axes", &axes, pThis );
	NMetaInfo::ReportMetaInfo( "FlyingHeight", (uint8_t*)&fFlyingHeight - pThis, sizeof(fFlyingHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxSpeed", (uint8_t*)&fMaxSpeed - pThis, sizeof(fMaxSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxAcceleration", (uint8_t*)&fMaxAcceleration - pThis, sizeof(fMaxAcceleration), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxTilt", (uint8_t*)&fMaxTilt - pThis, sizeof(fMaxTilt), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RotationSpeed", (uint8_t*)&fRotationSpeed - pThis, sizeof(fRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FullSpinTime", (uint8_t*)&fFullSpinTime - pThis, sizeof(fFullSpinTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitHelicopter::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SM1UnitSpecific*)(this) );
	saver.Add( "Axes", &axes );
	saver.Add( "FlyingHeight", &fFlyingHeight );
	saver.Add( "MaxSpeed", &fMaxSpeed );
	saver.Add( "MaxAcceleration", &fMaxAcceleration );
	saver.Add( "MaxTilt", &fMaxTilt );
	saver.Add( "RotationSpeed", &fRotationSpeed );
	saver.Add( "FullSpinTime", &fFullSpinTime );

	return 0;
}

int SM1UnitHelicopter::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SM1UnitSpecific*)this );
	saver.Add( 2, &axes );
	saver.Add( 3, &fFlyingHeight );
	saver.Add( 4, &fMaxSpeed );
	saver.Add( 5, &fMaxAcceleration );
	saver.Add( 6, &fMaxTilt );
	saver.Add( 7, &fRotationSpeed );
	saver.Add( 8, &fFullSpinTime );

	return 0;
}

uint32_t SM1UnitHelicopter::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SM1UnitSpecific::CalcCheckSum() << axes << fFlyingHeight << fMaxSpeed << fMaxAcceleration << fMaxTilt << fRotationSpeed << fFullSpinTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
BASIC_REGISTER_DATABASE_CLASS( STATS_B2_M1, SM1UnitSpecific )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x31197340, SM1UnitHelicopter )

