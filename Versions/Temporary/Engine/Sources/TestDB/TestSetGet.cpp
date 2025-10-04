#include "stdafx.h"
#include "testtype.h"
#include "MeasureTimer.h"
#include "libdb/ObjMan.h"

namespace NTest
{

DWORD flags1[2] = { 0x0f0f0f0f, 0xf0f0f0f0 };
DWORD flags2[2] = { 0x11223344, 0x55667788 };
// {51A5BCA0-B663-41a7-8DED-0673AD1C7F60}
static const GUID guid1 = { 0x51a5bca0, 0xb663, 0x41a7, { 0x8d, 0xed, 0x06, 0x73, 0xad, 0x1c, 0x7f, 0x60 } };
// {A86000C4-9F09-4241-AA78-F9DA96F3D897}
static const GUID guid2 = { 0xa86000c4, 0x9f09, 0x4241, { 0xaa, 0x78, 0xf9, 0xda, 0x96, 0xf3, 0xd8, 0x97 } };
//
struct STestSetGetData
{
	const char *pszFieldName;
	bool bHasCode;
	CVariant varFirstValue;
	CVariant varSecondValue;
};
static const STestSetGetData testdata[] = 
{
	{ "Name", true, L"Nice name 1", L"Nice name 2" },
	{ "HP", true, 3.14159f, 2.71828f },
	{ "HasPassability", true, true, false },
	{ "Flags", true, CVariant(flags1, sizeof(flags1)), CVariant(flags2, sizeof(flags2)) },
	{ "DesignerName", true, "Designer 1", "Designer 2" },
	{ "UnitType", true, "UNIT_TYPE_ARMOR_HEAVY", "UNIT_TYPE_AVIA_FIGHTER" },
	{ "Sight", true, 80.0f, 150.0f },
	{ "Speed", true, 10.8f, 20.56f },
	{ "BoundTileRadius", true, 1, 3 },
	{ "Jx.Amplitude", true, 2.3f, 7.8f },
	{ "Jx.Phase", true, 34.23f, 67.34f },
	{ "Jx.Shift", true, 10.45f, 23.53f },
	{ "Jx.Tremble.x", true, 1.2f, 5.6f },
	{ "Jx.Tremble.y", true, 1.2f * 2.0f, 5.6f * 2.0f },
	{ "Jx.Tremble.z", true, 1.2f * 3.0f, 5.6f * 3.0f },
	{ "Jy.Amplitude", true, 6.2f, 4.9f },
	{ "Jy.Phase", true, 33.33f, 66.66f },
	{ "Jy.Shift", true, 55.55f, 44.44f },
	{ "Jy.Tremble.x", true, 4.4f, 7.7f },
	{ "Jy.Tremble.y", true, 4.4f * 2.0f, 7.7f * 2.0f },
	{ "Jy.Tremble.z", true, 4.4f * 3.0f, 7.7f * 3.0f },
	{ "guid", true, guid1, guid2 },
	//
	{ "nocodeInt", false, 32, 768 },
	{ "nocodeFloat", false, 99.22f, 95.38f },
	{ "nocodeBool", false, true, false },
	{ "nocodeString", false, "nocode string 1", "nocode string 2" },
	{ "nocodeWString", false, L"nocode wstring 1", L"nocode string 2" },
	{ "nocodeGUID", false, guid1, guid2 },
	{ "nocodeEnumUnitType", false, "UNIT_TYPE_ARMOR_HEAVY", "UNIT_TYPE_AVIA_FIGHTER" },
	{ "nocodeBinaryFlags", false, CVariant(flags1, sizeof(flags1)), CVariant(flags2, sizeof(flags2)) },
	{ "Weapon", true, CDBID("weapons/ussr/heavy/a20.xdb"), CDBID("weapons/ussr/light/ppsh.xdb") },
	//
	{ 0, false, 0, 0 }
};
const STestSetGetData *FindTestData( const char *pszName )
{
	for ( const STestSetGetData *pData = testdata; pData->pszFieldName != 0; ++pData )
	{
		if ( strcmp( pszName, pData->pszFieldName ) == 0 )
			return pData;
	}
	return 0;
}

bool TestMetaSetGet( NDb::IObjMan *pBind, const STestSetGetData *pData )
{
	CVariant variant;
	bool bSetSuccessfull = pBind->SetValue( pData->pszFieldName, pData->varFirstValue );
	NI_VERIFY( bSetSuccessfull != false, StrFmt("Incorrect 'Set' for field \"%s\"", pData->pszFieldName), return false );
	bool bGetSuccessfull = pBind->GetValue( pData->pszFieldName, &variant );
	NI_VERIFY( bGetSuccessfull != false, StrFmt("Incorrect 'Get' for field \"%s\"", pData->pszFieldName), return false );
	NI_VERIFY( variant == pData->varFirstValue, StrFmt("Incorrect value after set/get for field \"%s\"", pData->pszFieldName), return false );
	return true;
}

bool TestMetaSetGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit )
{
	NTest::CMeasureTimer timer( "TestMetaSetGet" );
	for ( const STestSetGetData *pData = testdata; pData->pszFieldName != 0; ++pData )
		NI_VERIFY( TestMetaSetGet( pBind, pData ) != false, "Set/Get test failed", return false );
	return true;
}

template <class TField>
bool TestMetaSetDirectGet( TField *pField, const char *pszFieldName )
{
	const STestSetGetData *pData = FindTestData( pszFieldName );
	NI_VERIFY( pData != 0, StrFmt("Can't find field \"%s\"", pszFieldName), return false );
	NI_VERIFY( pData->varSecondValue == CVariant(*pField), StrFmt("Incorrect meta-set/direct-get for field \"%s\"", pszFieldName), return false );
	return true;
}

bool TestMetaSetDirectGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit )
{
	NTest::CMeasureTimer timer( "TestMetaSetDirectGet" );
	// set values
	for ( const STestSetGetData *pData = testdata; pData->pszFieldName != 0; ++pData )
	{
		if ( pData->bHasCode )
			pBind->SetValue( pData->pszFieldName, pData->varSecondValue );
	}
	// check values
	TestMetaSetDirectGet( &pMechUnit->wszName, "Name" );
	TestMetaSetDirectGet( &pMechUnit->fHP, "HP" );
	TestMetaSetDirectGet( &pMechUnit->bHasPassability, "HasPassability" );
	TestMetaSetDirectGet( &pMechUnit->szDesignerName, "DesignerName" );
	TestMetaSetDirectGet( &pMechUnit->flags, "Flags" );
	string szUnitType = SKnownEnum<NDb::SUnitBase::EUnitType>::ToString( pMechUnit->eUnitType );
	TestMetaSetDirectGet( &szUnitType, "UnitType" );
	TestMetaSetDirectGet( &pMechUnit->fSight, "Sight" );
	TestMetaSetDirectGet( &pMechUnit->nBoundTileRadius, "BoundTileRadius" );
	TestMetaSetDirectGet( &pMechUnit->jx.fAmplitude, "Jx.Amplitude" );
	TestMetaSetDirectGet( &pMechUnit->jx.fPhase, "Jx.Phase" );
	TestMetaSetDirectGet( &pMechUnit->jx.fShift, "Jx.Shift" );
	TestMetaSetDirectGet( &pMechUnit->jx.vTremble.x, "Jx.Tremble.x" );
	TestMetaSetDirectGet( &pMechUnit->jx.vTremble.y, "Jx.Tremble.y" );
	TestMetaSetDirectGet( &pMechUnit->jx.vTremble.z, "Jx.Tremble.z" );
	TestMetaSetDirectGet( &pMechUnit->jy.fAmplitude, "Jy.Amplitude" );
	TestMetaSetDirectGet( &pMechUnit->jy.fPhase, "Jy.Phase" );
	TestMetaSetDirectGet( &pMechUnit->jy.fShift, "Jy.Shift" );
	TestMetaSetDirectGet( &pMechUnit->jy.vTremble.x, "Jy.Tremble.x" );
	TestMetaSetDirectGet( &pMechUnit->jy.vTremble.y, "Jy.Tremble.y" );
	TestMetaSetDirectGet( &pMechUnit->jy.vTremble.z, "Jy.Tremble.z" );
	TestMetaSetDirectGet( &pMechUnit->guid, "guid" );
	// ref type
	{
		const STestSetGetData *pData = FindTestData( "Weapon" );
		NI_VERIFY( pData != 0, StrFmt("Can't find field \"%s\"", "Weapon"), return false );
		NI_VERIFY( pMechUnit->pWeapon->GetDBID() == pData->varSecondValue.GetDBID(), StrFmt("Incorrect meta-set/direct-get for field \"%s\"", "Weapon"), return false );
	}
	return true;
}

template <class TField> TField GetFromVariant( const CVariant &var ) { return (TField)var; }
template <> string GetFromVariant<string>( const CVariant &var ) { return var.GetStr(); }
template <> wstring GetFromVariant<wstring>( const CVariant &var ) { return var.GetWStr(); }
template <class TField>
bool TestDirectSetMetaGet( TField *pField, const char *pszFieldName )
{
	const STestSetGetData *pData = FindTestData( pszFieldName );
	NI_VERIFY( pData != 0, StrFmt("Can't find field \"%s\"", pszFieldName), return false );
	*pField = GetFromVariant<TField>( pData->varFirstValue );
	return true;
}
bool TestDirectSetMetaGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit )
{
	NTest::CMeasureTimer timer( "TestDirectSetMetaGet" );
	// set values
	TestDirectSetMetaGet( &pMechUnit->wszName, "Name" );
	TestDirectSetMetaGet( &pMechUnit->fHP, "HP" );
	TestDirectSetMetaGet( &pMechUnit->bHasPassability, "HasPassability" );
	TestDirectSetMetaGet( &pMechUnit->szDesignerName, "DesignerName" );
	memcpy( &pMechUnit->flags, FindTestData("Flags")->varFirstValue.GetPtr(), sizeof(pMechUnit->flags) );
	pMechUnit->eUnitType = SKnownEnum<NDb::SUnitBase::EUnitType>::ToEnum( FindTestData("UnitType")->varFirstValue.GetStr() );
	TestDirectSetMetaGet( &pMechUnit->fSight, "Sight" );
	TestDirectSetMetaGet( &pMechUnit->fSpeed, "Speed" );
	TestDirectSetMetaGet( &pMechUnit->nBoundTileRadius, "BoundTileRadius" );
	TestDirectSetMetaGet( &pMechUnit->jx.fAmplitude, "Jx.Amplitude" );
	TestDirectSetMetaGet( &pMechUnit->jx.fPhase, "Jx.Phase" );
	TestDirectSetMetaGet( &pMechUnit->jx.fShift, "Jx.Shift" );
	TestDirectSetMetaGet( &pMechUnit->jx.vTremble.x, "Jx.Tremble.x" );
	TestDirectSetMetaGet( &pMechUnit->jx.vTremble.y, "Jx.Tremble.y" );
	TestDirectSetMetaGet( &pMechUnit->jx.vTremble.z, "Jx.Tremble.z" );
	TestDirectSetMetaGet( &pMechUnit->jy.fAmplitude, "Jy.Amplitude" );
	TestDirectSetMetaGet( &pMechUnit->jy.fPhase, "Jy.Phase" );
	TestDirectSetMetaGet( &pMechUnit->jy.fShift, "Jy.Shift" );
	TestDirectSetMetaGet( &pMechUnit->jy.vTremble.x, "Jy.Tremble.x" );
	TestDirectSetMetaGet( &pMechUnit->jy.vTremble.y, "Jy.Tremble.y" );
	TestDirectSetMetaGet( &pMechUnit->jy.vTremble.z, "Jy.Tremble.z" );
	memcpy( &pMechUnit->guid, FindTestData("guid")->varFirstValue.GetPtr(), sizeof(pMechUnit->guid) );
	pMechUnit->pWeapon = static_cast<NDb::SWeapon*>( NDb::GetObject( FindTestData("Weapon")->varFirstValue.GetDBID() ) );
	// check values
	for ( const STestSetGetData *pData = testdata; pData->pszFieldName != 0; ++pData )
	{
		if ( pData->bHasCode )
		{
			CVariant value;
			NI_VERIFY( pBind->GetValue( pData->pszFieldName, &value ) != false, StrFmt("Can't meta-get value for field \"%s\"", pData->pszFieldName), return false );
			NI_VERIFY( pData->varFirstValue == value, StrFmt("Incorrect direct-set/meta-get for field \"%s\"", pData->pszFieldName), return false );
		}
	}
	return true;
}

}

