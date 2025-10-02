// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbplanemanuvers.h"

namespace NDb
{


string EnumToString( NDb::ESpeedRelation eValue )
{
	switch ( eValue )
	{
	case NDb::ESR_NEAR_STALL:
		return "ESR_NEAR_STALL";
	case NDb::ESR_SMALL:
		return "ESR_SMALL";
	case NDb::ESR_NORMAL:
		return "ESR_NORMAL";
	case NDb::ESR_MAXIMUM:
		return "ESR_MAXIMUM";
	case NDb::_ESR_COUNT:
		return "_ESR_COUNT";
	default:
		return "ESR_NEAR_STALL";
	}
}

NDb::ESpeedRelation NDb::StringToEnum_NDb_ESpeedRelation( const string &szValue )
{
	if ( szValue == "ESR_NEAR_STALL" )
		return NDb::ESR_NEAR_STALL;
	if ( szValue == "ESR_SMALL" )
		return NDb::ESR_SMALL;
	if ( szValue == "ESR_NORMAL" )
		return NDb::ESR_NORMAL;
	if ( szValue == "ESR_MAXIMUM" )
		return NDb::ESR_MAXIMUM;
	if ( szValue == "_ESR_COUNT" )
		return NDb::_ESR_COUNT;
	return NDb::ESR_NEAR_STALL;
}

string EnumToString( NDb::EPlanesAttitude eValue )
{
	switch ( eValue )
	{
	case NDb::EPA_ATTACK:
		return "EPA_ATTACK";
	case NDb::EPA_RETREAT:
		return "EPA_RETREAT";
	default:
		return "EPA_ATTACK";
	}
}

NDb::EPlanesAttitude NDb::StringToEnum_NDb_EPlanesAttitude( const string &szValue )
{
	if ( szValue == "EPA_ATTACK" )
		return NDb::EPA_ATTACK;
	if ( szValue == "EPA_RETREAT" )
		return NDb::EPA_RETREAT;
	return NDb::EPA_ATTACK;
}

string EnumToString( NDb::EManuverDestination eValue )
{
	switch ( eValue )
	{
	case NDb::EMD_PREDICTED_POINT:
		return "EMD_PREDICTED_POINT";
	case NDb::EMD_MANUVER_DEPENDENT:
		return "EMD_MANUVER_DEPENDENT";
	default:
		return "EMD_PREDICTED_POINT";
	}
}

NDb::EManuverDestination NDb::StringToEnum_NDb_EManuverDestination( const string &szValue )
{
	if ( szValue == "EMD_PREDICTED_POINT" )
		return NDb::EMD_PREDICTED_POINT;
	if ( szValue == "EMD_MANUVER_DEPENDENT" )
		return NDb::EMD_MANUVER_DEPENDENT;
	return NDb::EMD_PREDICTED_POINT;
}

string EnumToString( NDb::EManuverID eValue )
{
	switch ( eValue )
	{
	case NDb::DB_EMID_GENERIC:
		return "DB_EMID_GENERIC";
	case NDb::DB_EMID_STEEP_CLIMB:
		return "DB_EMID_STEEP_CLIMB";
	default:
		return "DB_EMID_GENERIC";
	}
}

NDb::EManuverID NDb::StringToEnum_NDb_EManuverID( const string &szValue )
{
	if ( szValue == "DB_EMID_GENERIC" )
		return NDb::DB_EMID_GENERIC;
	if ( szValue == "DB_EMID_STEEP_CLIMB" )
		return NDb::DB_EMID_STEEP_CLIMB;
	return NDb::DB_EMID_GENERIC;
}


void SDirectionRange::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DirectionRange", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SDirectionRange::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SDirectionRange::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SDirectionRange::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SSpeedRange::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "SpeedRange", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SSpeedRange::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SSpeedRange::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SSpeedRange::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SDistanceRange::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "DistanceRange", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SDistanceRange::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SDistanceRange::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SDistanceRange::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SHeightRange::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "HeightRange", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SHeightRange::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );

	return 0;
}

int SHeightRange::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );

	return 0;
}

DWORD SHeightRange::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SManuverConditions::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "EnemyDirection", (BYTE*)&pEnemyDirection - pThis, sizeof(pEnemyDirection), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SelfDirection", (BYTE*)&pSelfDirection - pThis, sizeof(pSelfDirection), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SpeedAngle", (BYTE*)&pSpeedAngle - pThis, sizeof(pSpeedAngle), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Distance", (BYTE*)&pDistance - pThis, sizeof(pDistance), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SelfHeight", (BYTE*)&pSelfHeight - pThis, sizeof(pSelfHeight), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "HeightDifference", (BYTE*)&pHeightDifference - pThis, sizeof(pHeightDifference), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "SelfSpeed", (BYTE*)&pSelfSpeed - pThis, sizeof(pSelfSpeed), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "EnemySpeed", (BYTE*)&pEnemySpeed - pThis, sizeof(pEnemySpeed), NTypeDef::TYPE_TYPE_REF );
}

int SManuverConditions::operator&( IXmlSaver &saver )
{
	saver.Add( "EnemyDirection", &pEnemyDirection );
	saver.Add( "SelfDirection", &pSelfDirection );
	saver.Add( "SpeedAngle", &pSpeedAngle );
	saver.Add( "Distance", &pDistance );
	saver.Add( "SelfHeight", &pSelfHeight );
	saver.Add( "HeightDifference", &pHeightDifference );
	saver.Add( "SelfSpeed", &pSelfSpeed );
	saver.Add( "EnemySpeed", &pEnemySpeed );

	return 0;
}

int SManuverConditions::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pEnemyDirection );
	saver.Add( 3, &pSelfDirection );
	saver.Add( 4, &pSpeedAngle );
	saver.Add( 5, &pDistance );
	saver.Add( 6, &pSelfHeight );
	saver.Add( 7, &pHeightDifference );
	saver.Add( 8, &pSelfSpeed );
	saver.Add( 9, &pEnemySpeed );

	return 0;
}

DWORD SManuverConditions::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pEnemyDirection << pSelfDirection << pSpeedAngle << pDistance << pSelfHeight << pHeightDifference << pSelfSpeed << pEnemySpeed;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SManuverDescriptor::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ManuverDescriptor", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Conditions", &conditions, pThis ); 
	NMetaInfo::ReportMetaInfo( "ManuverID", (BYTE*)&eManuverID - pThis, sizeof(eManuverID), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Destination", (BYTE*)&eDestination - pThis, sizeof(eDestination), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Attitude", (BYTE*)&eAttitude - pThis, sizeof(eAttitude), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SManuverDescriptor::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Conditions", &conditions );
	saver.Add( "ManuverID", &eManuverID );
	saver.Add( "Destination", &eDestination );
	saver.Add( "Attitude", &eAttitude );

	return 0;
}

int SManuverDescriptor::operator&( IBinSaver &saver )
{
	saver.Add( 2, &conditions );
	saver.Add( 3, &eManuverID );
	saver.Add( 4, &eDestination );
	saver.Add( 5, &eAttitude );

	return 0;
}

DWORD SManuverDescriptor::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << conditions << eManuverID << eDestination << eAttitude;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x1108EB80, SDirectionRange ) 
REGISTER_DATABASE_CLASS( 0x1108EB81, SSpeedRange ) 
REGISTER_DATABASE_CLASS( 0x1108EB82, SDistanceRange ) 
REGISTER_DATABASE_CLASS( 0x1108EB83, SHeightRange ) 
REGISTER_DATABASE_CLASS( 0x1108EB84, SManuverDescriptor ) 

