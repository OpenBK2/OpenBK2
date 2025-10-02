// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "m1unittype.h"

namespace NDb
{


string EnumToString( NDb::EM1UnitBaseType eValue )
{
	switch ( eValue )
	{
	case NDb::M1_MECH:
		return "M1_MECH";
	case NDb::M1_SOLDIER:
		return "M1_SOLDIER";
	case NDb::M1_PLANE:
		return "M1_PLANE";
	case NDb::M1_HELICOPTER:
		return "M1_HELICOPTER";
	default:
		return "M1_MECH";
	}
}

NDb::EM1UnitBaseType NDb::StringToEnum_NDb_EM1UnitBaseType( const string &szValue )
{
	if ( szValue == "M1_MECH" )
		return NDb::M1_MECH;
	if ( szValue == "M1_SOLDIER" )
		return NDb::M1_SOLDIER;
	if ( szValue == "M1_PLANE" )
		return NDb::M1_PLANE;
	if ( szValue == "M1_HELICOPTER" )
		return NDb::M1_HELICOPTER;
	return NDb::M1_MECH;
}


void SM1UnitType::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitType", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "BaseType", (BYTE*)&eBaseType - pThis, sizeof(eBaseType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitType::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "BaseType", &eBaseType );

	return 0;
}

int SM1UnitType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eBaseType );

	return 0;
}

DWORD SM1UnitType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eBaseType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x33193B01, SM1UnitType ) 
