#include "StdAfx.h"
#include "TypeDef.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace NDb::NTypeDef;
namespace NTest
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef hash_map<string, CObj<STypeDef> > CSimpleTypesMap;
static CSimpleTypesMap simpleTypes;
struct SSimpleTypesAutoMagic
{
	SSimpleTypesAutoMagic()
	{
		simpleTypes["int"] = new STypeInt();
		simpleTypes["float"] = new STypeFloat();
		simpleTypes["bool"] = new STypeBool();
		simpleTypes["string"] = new STypeString();
		simpleTypes["wstring"] = new STypeWString();
		simpleTypes["GUID"] = new STypeGUID();
		//
		STypeStruct *pVec3 = new STypeStruct( "Vec3" );
		simpleTypes["Vec3"] = pVec3;
		pVec3->AddField( simpleTypes["float"], "x", 1, L"", 0, 0 );
		pVec3->AddField( simpleTypes["float"], "y", 2, L"", 0, 0 );
		pVec3->AddField( simpleTypes["float"], "z", 3, L"", 0, 0 );

		//
		STypeStruct *pQuat = new STypeStruct( "Quat" );
		simpleTypes["Quat"] = pQuat;
		pQuat->AddField( simpleTypes["float"], "x", 1, L"", 0, 0 );
		pQuat->AddField( simpleTypes["float"], "y", 2, L"", 0, 0 );
		pQuat->AddField( simpleTypes["float"], "z", 3, L"", 0, 0 );
		pQuat->AddField( simpleTypes["float"], "w", 4, L"", 0, 0 );
	}
};
static SSimpleTypesAutoMagic aSSimpleTypesAutoMagic;
STypeDef* GetSimpleType( const string &szName )
{
	CSimpleTypesMap::iterator pos = simpleTypes.find( szName );
	NI_ASSERT( pos != simpleTypes.end(), StrFmt("Unknown type \"%s\"", szName.c_str()) );
	return pos != simpleTypes.end() ? pos->second : 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateTestTypes( vector< CObj<NDb::NTypeDef::STypeDef> > *pTopLevelTypes )
{
	//
	// Weapon
	//
	STypeClass *pWeapon = new STypeClass( "Weapon" );
	pWeapon->nClassTypeID = 0x1019230D;
	pTopLevelTypes->push_back( pWeapon );
	pWeapon->AddField( GetSimpleType("int"), "AmmoPerBurst", 1, L"How many bullets are shooted during one burst", 0, 0 );
	//
	// HPObject
	//
	CPtr<STypeClass> pHPObject = new STypeClass( "HPObject" );
	// Name
	pHPObject->AddField( GetSimpleType("wstring"), "Name", 3, L"Object's internal name", 0, 0 );
	// HP
	pHPObject->AddField( GetSimpleType("float"), "HP", 2, L"Object's hit points (health)", new SConstraintsMinMaxInt(0, 10000), 0 );
	// has passability
	pHPObject->AddField( GetSimpleType("bool"), "HasPassability", 4, L"Does this object have passability?",  0, 0 );
	// flags
	STypeBinary *pBinaryFlags = new STypeBinary( "Flags" );
	pBinaryFlags->nBinaryObjectSize = 8;
	pHPObject->AddField( pBinaryFlags, "Flags", 5, L"General-purpose flags",  0, 0 );
	// DesignerName
	pHPObject->AddField( GetSimpleType("string"), "DesignerName", 6, L"Designer, who creates this object", 0, 0 );
	//
	// UnitBase
	//
	CPtr<STypeClass> pUnitBase = new STypeClass( "UnitBase" );
	pUnitBase->pBaseType = pHPObject;
	// nested type: enum UnitType
	STypeEnum *pEnumUnitType = new STypeEnum( "UnitType" );
	pUnitBase->nestedTypes.push_back( pEnumUnitType );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_UNKNOWN", 0 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_INFANTRY_SNIPER", 1 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_ARMOR_MEDIUM", 2 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_ARMOR_HEAVY", 3 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_AVIA_FIGHTER", 4 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_AUTO_ENGINEER", 5 ) );
	pEnumUnitType->entries.push_back( STypeEnum::SEnumEntry( "UNIT_TYPE_SPG_ASSAULT", 6 ) );
	// UnitType
	pUnitBase->AddField( pEnumUnitType, "UnitType", 2, L"Unit type (for designers)", 0, 0 );
	// Sight
	pUnitBase->AddField( GetSimpleType("float"), "Sight", 3, L"(meters) Sight radius", 
		                   new SConstraintsMinMax<float>(0, 50), 0 );
	// Speed
	pUnitBase->AddField( GetSimpleType("float"), "Speed", 4, L"(meters per second) Maximum speed", 
                       new SConstraintsMinMax<float>(0, 80), 0 );
	// BoundTileRadius
	pUnitBase->AddField( GetSimpleType("int"), "BoundTileRadius", 5, L"(meters) Radius for passability calculations", 
		                   new SConstraintsMinMax<int>(1, 5), 0 );
	//
	// MechUnit
	//
	STypeClass *pMechUnit = new STypeClass( "MechUnit" );
	pMechUnit->pBaseType = pUnitBase;
	pMechUnit->nClassTypeID = 0x1019230E;
	pTopLevelTypes->push_back( pMechUnit );
	pMechUnit->RegisterTerminalType();
	// nested type: jogging
	STypeStruct *pJogging = new STypeStruct( "Jogging" );
	pMechUnit->nestedTypes.push_back( pJogging );
	pJogging->AddField( GetSimpleType("float"), "Amplitude", 1, L"Cosine amplitude", 0, 0 );
	pJogging->AddField( GetSimpleType("float"), "Phase", 2, L"Cosine phase", 0, 0 );
	pJogging->AddField( GetSimpleType("float"), "Shift", 3, L"Cosine phase shift", 0, 0 );
	pJogging->AddField( GetSimpleType("Vec3"), "Tremble", 4, L"Uniform tremble along all 3 axises", 0, 0 );
	// nested type: Struct1
	//
	NDb::NTypeDef::SAttributes *pAttributeHidden = new NDb::NTypeDef::SAttributes();
	pAttributeHidden->attributes["hidden"] = "true";
	STypeStruct *pStruct1 = new STypeStruct( "Struct1" );
	//
	pMechUnit->nestedTypes.push_back( pStruct1 );
	pStruct1->AddField( GetSimpleType("int"), "TypeInt", 1, L"just a test int value", 0, 0 );
	pStruct1->AddField( GetSimpleType("float"), "TypeFloat", 2, L"just a test float value", 0, pAttributeHidden );
	pStruct1->AddField( GetSimpleType("bool"), "TypeBool", 3, L"just a test bool value", 0, 0 );
	pStruct1->AddField( GetSimpleType("GUID"), "TypeGUID", 4, L"just a test GUID value", 0, pAttributeHidden );
	pStruct1->AddField( GetSimpleType("string"), "TypeString", 5, L"just a test string value", 0, pAttributeHidden );
	pStruct1->AddField( GetSimpleType("wstring"), "TypeWString", 6, L"just a test wstring value", 0, 0, L"Очень клёвое default value" );
	pStruct1->AddField( pEnumUnitType, "TypeEnumUnitType", 7, L"just a test enum (UnitType) value", 0, 0 );
	pStruct1->AddField( pBinaryFlags, "TypeBinaryFlags", 8, L"just a test binary (Flags) value", 0, 0 );
	// nested type: Struct2
	STypeStruct *pStruct2 = new STypeStruct( "Struct2" );
	pMechUnit->nestedTypes.push_back( pStruct2 );
	pStruct2->AddField( new STypeArray(pStruct1), "Structs", 1, L"array of complex struct (for array-in-array testing)", 0, 0 );
	pStruct2->AddField( new STypeArray(GetSimpleType("GUID")), "guids", 2, L"array of GUIDs (for array-in-array testing)", 0, 0 );
//	// nested type: struct Platform
//	STypeStruct *pPlatform = new STypeStruct( "Platform" );
//	pMechUnit->nestedTypes.push_back( pPlatform );
//	STypeStruct *pPlatformConstraint = new STypeStruct( "Constraint" );
//	pPlatform->nestedTypes.push_back( pPlatformConstraint );
//	pPlatformConstraint->AddField( GetSimpleType("float"), "Min", 1, L"", 0, 0 );
//	pPlatformConstraint->AddField( GetSimpleType("float"), "Max", 2, L"", 0, 0 );
//	pPlatform->AddField( pPlatformConstraint, "HConstraints", 1, L"(degrees) Horizontal rotation constraints", 0, 0 );
//	pPlatform->AddField( pPlatformConstraint, "VConstraints", 2, L"(degrees) Vertical rotation constraints", 0, 0 );
//	// nested type: platform::gun
//	STypeStruct *pPlatformGun = new STypeStruct( "Gun" );
//	pPlatform->nestedTypes.push_back( pPlatformGun );
//	pPlatformGun->AddField( GetSimpleType("int"), "MaxAmmo", 1, L"Max ammo this gun can have", 0, 0 );
//	pPlatformGun->AddField( new STypeRef(pWeapon), "Weapon", 2, L"Mounted weapon", 0, 0 );
//	pPlatform->AddField( new STypeArray(pPlatformGun), "Guns", 3, L"All unit guns", 0, 0 );
	//
	pMechUnit->AddField( pJogging, "Jx", 2, L"Jogging along X-axis", 0, 0 );
	pMechUnit->AddField( pJogging, "Jy", 3, L"Jogging along Y-axis", 0, 0 );
	pMechUnit->AddField( GetSimpleType("GUID"), "guid", 4, L"Object's unique identifier", 0, 0 );
//	pMechUnit->AddField( new STypeArray(pPlatform), "Platforms", 3, L"All unit platforms", 0, 0 );
	// add nocode fields
	pMechUnit->AddField( GetSimpleType("int"), "nocodeInt", -1, L"Just a nocode integer field", 0, 0 );
	pMechUnit->AddField( GetSimpleType("float"), "nocodeFloat", -1, L"Just a nocode floating-point field", 0, 0 );
	pMechUnit->AddField( GetSimpleType("bool"), "nocodeBool", -1, L"Just a nocode boolean field", 0, 0 );
	pMechUnit->AddField( GetSimpleType("string"), "nocodeString", -1, L"Just a nocode string field", 0, 0 );
	pMechUnit->AddField( GetSimpleType("wstring"), "nocodeWString", -1, L"Just a nocode wide string field", 0, 0 );
	pMechUnit->AddField( GetSimpleType("GUID"), "nocodeGUID", -1, L"Just a nocode GUID field", 0, 0 );
	pMechUnit->AddField( pEnumUnitType, "nocodeEnumUnitType", -1, L"Just a nocode enum UnitType field", 0, 0 );
	pMechUnit->AddField( pBinaryFlags, "nocodeBinaryFlags", -1, L"Just a nocode binary Flags field", 0, 0 );
	// simple arrays
	{
		STypeDef *pSimpleArrayInt = new STypeArray( GetSimpleType("int") );
		pMechUnit->AddField( pSimpleArrayInt, "SimpleArrayInt", 5, L" SimpleArrayInt", 0, 0 );
		STypeDef *pSimpleArrayFloat = new STypeArray( GetSimpleType("float") );
		pMechUnit->AddField( pSimpleArrayFloat, "SimpleArrayFloat", 6, L" SimpleArrayFloat", 0, 0 );
		STypeDef *pSimpleArrayGUID = new STypeArray( GetSimpleType("GUID") );
		pMechUnit->AddField( pSimpleArrayGUID, "SimpleArrayGUID", 7, L" SimpleArrayGUID", 0, 0 );
		STypeDef *pSimpleArrayBinaryFlags = new STypeArray( pBinaryFlags );
		pMechUnit->AddField( pSimpleArrayBinaryFlags, "SimpleArrayBinaryFlags", 8, L" SimpleArrayBinaryFlags", 0, 0 );
		STypeDef *pSimpleArrayEnumUnitType = new STypeArray( pEnumUnitType );
		pMechUnit->AddField( pSimpleArrayEnumUnitType, "SimpleArrayEnumUnitType", 9, L" SimpleArrayEnumUnitType", 0, 0 );
		STypeDef *pSimpleArrayString = new STypeArray( GetSimpleType("string") );
		pMechUnit->AddField( pSimpleArrayString, "SimpleArrayString", 10, L" SimpleArrayString", 0, 0 );
		STypeDef *pSimpleArrayWString = new STypeArray( GetSimpleType("wstring") );
		pMechUnit->AddField( pSimpleArrayWString, "SimpleArrayWString", 11, L" SimpleArrayWString", 0, 0 );
//
//		STypeDef *pSimpleArrayWString = new STypeArray( GetSimpleType("wstring") );
//		pMechUnit->AddField( pSimpleArrayWString, "SimpleArrayWString", 5, L"SimpleArrayWString", 0, 0 );
//		STypeDef *pSimpleArrayGUID = new STypeArray( GetSimpleType("GUID") );
//		pMechUnit->AddField( pSimpleArrayGUID, "SimpleArrayGUID", 6, L"SimpleArrayGUID", 0, 0 );
	}
	// nocode simple arrays
	{
		STypeDef *pSimpleArrayInt = new STypeArray( GetSimpleType("int") );
		pMechUnit->AddField( pSimpleArrayInt, "SimpleArrayIntNocode", -1, L"Nocode SimpleArrayInt", 0, 0 );
		STypeDef *pSimpleArrayFloat = new STypeArray( GetSimpleType("float") );
		pMechUnit->AddField( pSimpleArrayFloat, "SimpleArrayFloatNocode", -1, L"Nocode SimpleArrayFloat", 0, 0 );
		STypeDef *pSimpleArrayGUID = new STypeArray( GetSimpleType("GUID") );
		pMechUnit->AddField( pSimpleArrayGUID, "SimpleArrayGUIDNocode", -1, L"Nocode SimpleArrayGUID", 0, 0 );
		STypeDef *pSimpleArrayBinaryFlags = new STypeArray( pBinaryFlags );
		pMechUnit->AddField( pSimpleArrayBinaryFlags, "SimpleArrayBinaryFlagsNocode", -1, L"Nocode SimpleArrayBinaryFlags", 0, 0 );
		STypeDef *pSimpleArrayEnumUnitType = new STypeArray( pEnumUnitType );
		pMechUnit->AddField( pSimpleArrayEnumUnitType, "SimpleArrayEnumUnitTypeNocode", -1, L"Nocode SimpleArrayEnumUnitType", 0, 0 );
		STypeDef *pSimpleArrayString = new STypeArray( GetSimpleType("string") );
		pMechUnit->AddField( pSimpleArrayString, "SimpleArrayStringNocode", -1, L"Nocode SimpleArrayString", 0, 0 );
		STypeDef *pSimpleArrayWString = new STypeArray( GetSimpleType("wstring") );
		pMechUnit->AddField( pSimpleArrayWString, "SimpleArrayWStringNocode", -1, L"Nocode SimpleArrayWString", 0, 0 );
	}
	//
	pMechUnit->AddField( new STypeArray(pStruct1), "ComplexArrayStruct1", 12, L"complex array", 0, 0 );
	pMechUnit->AddField( new STypeArray(pStruct2), "ComplexArrayStruct2", 13, L"complex array with array inside", 0, 0 );
	//
	pMechUnit->AddField( new STypeRef(pWeapon), "Weapon", 14, L"Weapon reference", 0, 0 );
	pMechUnit->AddField( new STypeArray( new STypeRef(pWeapon) ), "Weapons", 15, L"Weapon references array", 0, 0 );
	//
	//
	//
	//
	//
	STypeClass *pMapInfo = new STypeClass( "MapInfo2" );
	pMapInfo->nClassTypeID = 0x101A6C80;
	pTopLevelTypes->push_back( pMapInfo );
	// nested type: MapObject
	STypeStruct *pMapObject = new STypeStruct( "MapObject" );
	pMapInfo->nestedTypes.push_back( pMapObject );
	pMapObject->AddField( GetSimpleType("float"), "HP", 1, L"(проценты от 0 до 1) Жизнь", 0, 0 );
	pMapObject->AddField( GetSimpleType("Vec3"), "Pos", 2, L"(метры) Положение на карте", 0, 0 );
	pMapObject->AddField( GetSimpleType("Quat"), "Rot", 3, L"(кватернион) Ориентация объекта в пространстве", 0, 0 );
	pMapObject->AddField( GetSimpleType("GUID"), "LinkID", 4, L"Уникальный идентификатор объекта на карте", 0, 0 );
	pMapObject->AddField( GetSimpleType("GUID"), "LinkWith", 5, L"С кем объект слинкован", 0, 0 );
	pMapObject->AddField( new STypeRef(pHPObject), "Object", 6, L"Ссылка на описатель статсов объекта", 0, 0 );
//	pMapObject->AddField( GetSimpleType("string"), "Object", 6, L"Ссылка на описатель статсов объекта", 0, 0 );
	//
	pMapInfo->AddField( new STypeArray(pMapObject), "Objects", 7, L"Список всех объектов на карте", 0, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
