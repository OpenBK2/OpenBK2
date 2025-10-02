// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbpassprofile.h"

namespace NDb
{



void SPolygon2D::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "verts", &verts, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "Fake", (BYTE*)&nFake - pThis, sizeof(nFake), NTypeDef::TYPE_TYPE_INT );
}

int SPolygon2D::operator&( IXmlSaver &saver )
{
	saver.Add( "verts", &verts );
	saver.Add( "Fake", &nFake );

	return 0;
}

int SPolygon2D::operator&( IBinSaver &saver )
{
	saver.Add( 2, &verts );
	saver.Add( 3, &nFake );

	return 0;
}

DWORD SPolygon2D::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << verts << nFake;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPassProfile::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "polygons", &polygons, pThis );
}

int SPassProfile::operator&( IXmlSaver &saver )
{
	saver.Add( "polygons", &polygons );

	return 0;
}

int SPassProfile::operator&( IBinSaver &saver )
{
	saver.Add( 2, &polygons );

	return 0;
}

DWORD SPassProfile::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << polygons;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
