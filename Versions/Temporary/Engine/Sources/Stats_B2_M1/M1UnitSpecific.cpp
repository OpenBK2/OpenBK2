// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "m1unitspecific.h"

namespace NDb
{



void SM1UnitSpecific::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
}

int SM1UnitSpecific::operator&( IXmlSaver &saver )
{

	return 0;
}

int SM1UnitSpecific::operator&( IBinSaver &saver )
{

	return 0;
}

DWORD SM1UnitSpecific::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitHelicopter::SHelicopterAxis::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Scaled", (BYTE*)&pScaled - pThis, sizeof(pScaled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Dynamic", (BYTE*)&pDynamic - pThis, sizeof(pDynamic), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LocatorName", (BYTE*)&szLocatorName - pThis, sizeof(szLocatorName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "StartScaleSpeed", (BYTE*)&fStartScaleSpeed - pThis, sizeof(fStartScaleSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HideStaticSpeed", (BYTE*)&fHideStaticSpeed - pThis, sizeof(fHideStaticSpeed), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SM1UnitHelicopter::SHelicopterAxis::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Axes", &axes, pThis );
	NMetaInfo::ReportMetaInfo( "FlyingHeight", (BYTE*)&fFlyingHeight - pThis, sizeof(fFlyingHeight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxSpeed", (BYTE*)&fMaxSpeed - pThis, sizeof(fMaxSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxAcceleration", (BYTE*)&fMaxAcceleration - pThis, sizeof(fMaxAcceleration), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "MaxTilt", (BYTE*)&fMaxTilt - pThis, sizeof(fMaxTilt), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RotationSpeed", (BYTE*)&fRotationSpeed - pThis, sizeof(fRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FullSpinTime", (BYTE*)&fFullSpinTime - pThis, sizeof(fFullSpinTime), NTypeDef::TYPE_TYPE_FLOAT );
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

DWORD SM1UnitHelicopter::CalcCheckSum() const
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
BASIC_REGISTER_DATABASE_CLASS( SM1UnitSpecific )
REGISTER_DATABASE_CLASS( 0x31197340, SM1UnitHelicopter ) 

