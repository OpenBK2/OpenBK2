// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "testtype.h"

namespace NDb
{



void SWeapon::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Weapon", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "AmmoPerBurst", (BYTE*)&nAmmoPerBurst - pThis, sizeof(nAmmoPerBurst), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWeapon::operator&( IXmlSaver &saver )
{
	saver.ReportCurrentObject( GetDBID() );
	saver.Add( "AmmoPerBurst", &nAmmoPerBurst );

	return 0;
}

int SWeapon::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nAmmoPerBurst );

	return 0;
}



void SHPObject::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Name", (BYTE*)&wszName - pThis, sizeof(wszName), NTypeDef::TYPE_TYPE_WSTRING );
	NMetaInfo::ReportMetaInfo( "HP", (BYTE*)&fHP - pThis, sizeof(fHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "HasPassability", (BYTE*)&bHasPassability - pThis, sizeof(bHasPassability), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Flags", (BYTE*)&flags - pThis, sizeof(flags), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "DesignerName", (BYTE*)&szDesignerName - pThis, sizeof(szDesignerName), NTypeDef::TYPE_TYPE_STRING );
}

int SHPObject::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &wszName );
	saver.Add( "HP", &fHP );
	saver.Add( "HasPassability", &bHasPassability );
	saver.Add( "Flags", &flags );
	saver.Add( "DesignerName", &szDesignerName );

	return 0;
}

int SHPObject::operator&( IBinSaver &saver )
{
	saver.Add( 2, &wszName );
	saver.Add( 3, &fHP );
	saver.Add( 4, &bHasPassability );
	saver.Add( 5, &flags );
	saver.Add( 6, &szDesignerName );

	return 0;
}


string EnumToString( NDb::SUnitBase::EUnitType eValue )
{
	switch ( eValue )
	{
	case NDb::SUnitBase::UNIT_TYPE_UNKNOWN:
		return "UNIT_TYPE_UNKNOWN";
	case NDb::SUnitBase::UNIT_TYPE_INFANTRY_SNIPER:
		return "UNIT_TYPE_INFANTRY_SNIPER";
	case NDb::SUnitBase::UNIT_TYPE_ARMOR_MEDIUM:
		return "UNIT_TYPE_ARMOR_MEDIUM";
	case NDb::SUnitBase::UNIT_TYPE_ARMOR_HEAVY:
		return "UNIT_TYPE_ARMOR_HEAVY";
	case NDb::SUnitBase::UNIT_TYPE_AVIA_FIGHTER:
		return "UNIT_TYPE_AVIA_FIGHTER";
	case NDb::SUnitBase::UNIT_TYPE_AUTO_ENGINEER:
		return "UNIT_TYPE_AUTO_ENGINEER";
	case NDb::SUnitBase::UNIT_TYPE_SPG_ASSAULT:
		return "UNIT_TYPE_SPG_ASSAULT";
	default:
		return "UNIT_TYPE_UNKNOWN";
	}
}

NDb::SUnitBase::EUnitType NDb::StringToEnum_NDb_SUnitBase_EUnitType( const string &szValue )
{
	if ( szValue == "UNIT_TYPE_UNKNOWN" )
		return NDb::SUnitBase::UNIT_TYPE_UNKNOWN;
	if ( szValue == "UNIT_TYPE_INFANTRY_SNIPER" )
		return NDb::SUnitBase::UNIT_TYPE_INFANTRY_SNIPER;
	if ( szValue == "UNIT_TYPE_ARMOR_MEDIUM" )
		return NDb::SUnitBase::UNIT_TYPE_ARMOR_MEDIUM;
	if ( szValue == "UNIT_TYPE_ARMOR_HEAVY" )
		return NDb::SUnitBase::UNIT_TYPE_ARMOR_HEAVY;
	if ( szValue == "UNIT_TYPE_AVIA_FIGHTER" )
		return NDb::SUnitBase::UNIT_TYPE_AVIA_FIGHTER;
	if ( szValue == "UNIT_TYPE_AUTO_ENGINEER" )
		return NDb::SUnitBase::UNIT_TYPE_AUTO_ENGINEER;
	if ( szValue == "UNIT_TYPE_SPG_ASSAULT" )
		return NDb::SUnitBase::UNIT_TYPE_SPG_ASSAULT;
	return NDb::SUnitBase::UNIT_TYPE_UNKNOWN;
}


void SUnitBase::ReportMetaInfo() const
{
	SHPObject::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "UnitType", (BYTE*)&eUnitType - pThis, sizeof(eUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Sight", (BYTE*)&fSight - pThis, sizeof(fSight), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Speed", (BYTE*)&fSpeed - pThis, sizeof(fSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "BoundTileRadius", (BYTE*)&nBoundTileRadius - pThis, sizeof(nBoundTileRadius), NTypeDef::TYPE_TYPE_INT );
}

int SUnitBase::operator&( IXmlSaver &saver )
{
	saver.AddTypedSuper( (SHPObject*)(this) );
	saver.Add( "UnitType", &eUnitType );
	saver.Add( "Sight", &fSight );
	saver.Add( "Speed", &fSpeed );
	saver.Add( "BoundTileRadius", &nBoundTileRadius );

	return 0;
}

int SUnitBase::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SHPObject*)this );
	saver.Add( 2, &eUnitType );
	saver.Add( 3, &fSight );
	saver.Add( 4, &fSpeed );
	saver.Add( 5, &nBoundTileRadius );

	return 0;
}



void SMechUnit::SJogging::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Amplitude", (BYTE*)&fAmplitude - pThis, sizeof(fAmplitude), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Phase", (BYTE*)&fPhase - pThis, sizeof(fPhase), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Shift", (BYTE*)&fShift - pThis, sizeof(fShift), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Tremble", &vTremble, pThis ); 
}

int SMechUnit::SJogging::operator&( IXmlSaver &saver )
{
	saver.Add( "Amplitude", &fAmplitude );
	saver.Add( "Phase", &fPhase );
	saver.Add( "Shift", &fShift );
	saver.Add( "Tremble", &vTremble );

	return 0;
}

int SMechUnit::SJogging::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fAmplitude );
	saver.Add( 3, &fPhase );
	saver.Add( 4, &fShift );
	saver.Add( 5, &vTremble );

	return 0;
}



void SMechUnit::SStruct1::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "TypeInt", (BYTE*)&nTypeInt - pThis, sizeof(nTypeInt), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeFloat", (BYTE*)&fTypeFloat - pThis, sizeof(fTypeFloat), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeBool", (BYTE*)&bTypeBool - pThis, sizeof(bTypeBool), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeGUID", (BYTE*)&typeGUID - pThis, sizeof(typeGUID), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeString", (BYTE*)&szTypeString - pThis, sizeof(szTypeString), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeWString", (BYTE*)&wszTypeWString - pThis, sizeof(wszTypeWString), NTypeDef::TYPE_TYPE_WSTRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeEnumUnitType", (BYTE*)&eTypeEnumUnitType - pThis, sizeof(eTypeEnumUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "TypeBinaryFlags", (BYTE*)&typeBinaryFlags - pThis, sizeof(typeBinaryFlags), NTypeDef::TYPE_TYPE_BINARY );
}

int SMechUnit::SStruct1::operator&( IXmlSaver &saver )
{
	saver.Add( "TypeInt", &nTypeInt );
	saver.Add( "TypeFloat", &fTypeFloat );
	saver.Add( "TypeBool", &bTypeBool );
	saver.Add( "TypeGUID", &typeGUID );
	saver.Add( "TypeString", &szTypeString );
	saver.Add( "TypeWString", &wszTypeWString );
	saver.Add( "TypeEnumUnitType", &eTypeEnumUnitType );
	saver.Add( "TypeBinaryFlags", &typeBinaryFlags );

	return 0;
}

int SMechUnit::SStruct1::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nTypeInt );
	saver.Add( 3, &fTypeFloat );
	saver.Add( 4, &bTypeBool );
	saver.Add( 5, &typeGUID );
	saver.Add( 6, &szTypeString );
	saver.Add( 7, &wszTypeWString );
	saver.Add( 8, &eTypeEnumUnitType );
	saver.Add( 9, &typeBinaryFlags );

	return 0;
}



void SMechUnit::SStruct2::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Structs", &structs, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "guids", &guids, pThis );
}

int SMechUnit::SStruct2::operator&( IXmlSaver &saver )
{
	saver.Add( "Structs", &structs );
	saver.Add( "guids", &guids );

	return 0;
}

int SMechUnit::SStruct2::operator&( IBinSaver &saver )
{
	saver.Add( 2, &structs );
	saver.Add( 3, &guids );

	return 0;
}



void SMechUnit::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MechUnit", typeID, sizeof(*this) );
	SUnitBase::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "Jx", &jx, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Jy", &jy, pThis ); 
	NMetaInfo::ReportMetaInfo( "guid", (BYTE*)&guid - pThis, sizeof(guid), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayInt", &simpleArrayInt, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayFloat", &simpleArrayFloat, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayGUID", &simpleArrayGUID, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayBinaryFlags", &simpleArrayBinaryFlags, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayEnumUnitType", &simpleArrayEnumUnitType, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayString", &simpleArrayString, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "SimpleArrayWString", &simpleArrayWString, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ComplexArrayStruct1", &complexArrayStruct1, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ComplexArrayStruct2", &complexArrayStruct2, pThis );
	NMetaInfo::ReportMetaInfo( "Weapon", (BYTE*)&pWeapon - pThis, sizeof(pWeapon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "Weapons", &weapons, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SMechUnit::operator&( IXmlSaver &saver )
{
	saver.ReportCurrentObject( GetDBID() );
	saver.AddTypedSuper( (SUnitBase*)(this) );
	saver.Add( "Jx", &jx );
	saver.Add( "Jy", &jy );
	saver.Add( "guid", &guid );
	saver.Add( "SimpleArrayInt", &simpleArrayInt );
	saver.Add( "SimpleArrayFloat", &simpleArrayFloat );
	saver.Add( "SimpleArrayGUID", &simpleArrayGUID );
	saver.Add( "SimpleArrayBinaryFlags", &simpleArrayBinaryFlags );
	saver.Add( "SimpleArrayEnumUnitType", &simpleArrayEnumUnitType );
	saver.Add( "SimpleArrayString", &simpleArrayString );
	saver.Add( "SimpleArrayWString", &simpleArrayWString );
	saver.Add( "ComplexArrayStruct1", &complexArrayStruct1 );
	saver.Add( "ComplexArrayStruct2", &complexArrayStruct2 );
	saver.Add( "Weapon", &pWeapon );
	saver.Add( "Weapons", &weapons );

	return 0;
}

int SMechUnit::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUnitBase*)this );
	saver.Add( 0, &simpleArrayInt );
	saver.Add( 0, &complexArrayStruct1 );
	saver.Add( 1, &complexArrayStruct2 );
	saver.Add( 1, &simpleArrayFloat );
	saver.Add( 2, &pWeapon );
	saver.Add( 2, &simpleArrayGUID );
	saver.Add( 2, &jx );
	saver.Add( 3, &jy );
	saver.Add( 3, &simpleArrayBinaryFlags );
	saver.Add( 3, &weapons );
	saver.Add( 4, &simpleArrayEnumUnitType );
	saver.Add( 4, &guid );
	saver.Add( 5, &simpleArrayString );
	saver.Add( 6, &simpleArrayWString );

	return 0;
}



void SMapInfo2::SMapObject::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "HP", (BYTE*)&fHP - pThis, sizeof(fHP), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Rot", &qRot, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "LinkID", (BYTE*)&linkID - pThis, sizeof(linkID), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportMetaInfo( szAddName + "LinkWith", (BYTE*)&linkWith - pThis, sizeof(linkWith), NTypeDef::TYPE_TYPE_GUID );
	NMetaInfo::ReportMetaInfo( szAddName + "Object", (BYTE*)&pObject - pThis, sizeof(pObject), NTypeDef::TYPE_TYPE_REF );
}

int SMapInfo2::SMapObject::operator&( IXmlSaver &saver )
{
	saver.Add( "HP", &fHP );
	saver.Add( "Pos", &vPos );
	saver.Add( "Rot", &qRot );
	saver.Add( "LinkID", &linkID );
	saver.Add( "LinkWith", &linkWith );
	saver.Add( "Object", &pObject );

	return 0;
}

int SMapInfo2::SMapObject::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fHP );
	saver.Add( 3, &vPos );
	saver.Add( 4, &qRot );
	saver.Add( 5, &linkID );
	saver.Add( 6, &linkWith );
	saver.Add( 7, &pObject );

	return 0;
}



void SMapInfo2::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MapInfo2", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Objects", &objects, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SMapInfo2::operator&( IXmlSaver &saver )
{
	saver.ReportCurrentObject( GetDBID() );
	saver.Add( "Objects", &objects );

	return 0;
}

int SMapInfo2::operator&( IBinSaver &saver )
{
	saver.Add( 2, &objects );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x1019230D, SWeapon ) 
BASIC_REGISTER_DATABASE_CLASS( SHPObject )
BASIC_REGISTER_DATABASE_CLASS( SUnitBase )
REGISTER_DATABASE_CLASS( 0x1019230E, SMechUnit ) 
REGISTER_DATABASE_CLASS( 0x101A6C80, SMapInfo2 ) 

