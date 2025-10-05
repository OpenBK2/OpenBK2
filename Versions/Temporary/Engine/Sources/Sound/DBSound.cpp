// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBSound.h"

namespace NDb
{


std::string EnumToString( NDb::ESoundType eValue )
{
	switch ( eValue )
	{
	case NDb::NORMAL:
		return "NORMAL";
	case NDb::PEACEFULL:
		return "PEACEFULL";
	case NDb::COMBAT:
		return "COMBAT";
	default:
		return "NORMAL";
	}
}

NDb::ESoundType NDb::StringToEnum_NDb_ESoundType( const std::string &szValue )
{
	if ( szValue == "NORMAL" )
		return NDb::NORMAL;
	if ( szValue == "PEACEFULL" )
		return NDb::PEACEFULL;
	if ( szValue == "COMBAT" )
		return NDb::COMBAT;
	return NDb::NORMAL;
}


void SComplexSoundDesc::SSoundStats::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "PathName", (BYTE*)&pPathName - pThis, sizeof(pPathName), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "MinDist", (BYTE*)&fMinDist - pThis, sizeof(fMinDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "MaxDist", (BYTE*)&fMaxDist - pThis, sizeof(fMaxDist), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Probability", (BYTE*)&fProbability - pThis, sizeof(fProbability), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "soundType", (BYTE*)&esoundType - pThis, sizeof(esoundType), NTypeDef::TYPE_TYPE_ENUM );
}

int SComplexSoundDesc::SSoundStats::operator&( IXmlSaver &saver )
{
	saver.Add( "PathName", &pPathName );
	saver.Add( "MinDist", &fMinDist );
	saver.Add( "MaxDist", &fMaxDist );
	saver.Add( "Probability", &fProbability );
	saver.Add( "soundType", &esoundType );

	return 0;
}

int SComplexSoundDesc::SSoundStats::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pPathName );
	saver.Add( 3, &fMinDist );
	saver.Add( 4, &fMaxDist );
	saver.Add( 5, &fProbability );
	saver.Add( 6, &esoundType );

	return 0;
}

DWORD SComplexSoundDesc::SSoundStats::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMinDist << fMaxDist << fProbability << esoundType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SComplexSoundDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ComplexSoundDesc", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "sounds", &sounds, pThis );
	NMetaInfo::ReportMetaInfo( "Looped", (BYTE*)&bLooped - pThis, sizeof(bLooped), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SComplexSoundDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "sounds", &sounds );
	saver.Add( "Looped", &bLooped );

	return 0;
}

int SComplexSoundDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &sounds );
	saver.Add( 3, &bLooped );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x11069BC3, SComplexSoundDesc ) 

