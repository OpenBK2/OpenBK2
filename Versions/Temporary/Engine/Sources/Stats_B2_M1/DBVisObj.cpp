// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbvisobj.h"

namespace NDb
{



void SVisObj::SSingleObj::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Model", (BYTE*)&pModel - pThis, sizeof(pModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "LowLevelModel", (BYTE*)&pLowLevelModel - pThis, sizeof(pLowLevelModel), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (BYTE*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
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

DWORD SVisObj::SSingleObj::CalcCheckSum() const
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

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "ForceAnimated", (BYTE*)&bForceAnimated - pThis, sizeof(bForceAnimated), NTypeDef::TYPE_TYPE_BOOL );
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
REGISTER_DATABASE_CLASS( 0x11073C40, SVisObj ) 

