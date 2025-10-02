#include "StdAfx.h"
#include "TestType.h"
#include "MeasureTimer.h"
#include "../libdb/ObjMan.h"

namespace NTest
{

// TODO{
// * add test to remove at position '-1'
// TODO}

struct SSimpleArrayTestData
{
	const string szFieldName;
	CVariant vars[5];
};
// {C0968E41-7392-4c8a-9AE7-D20CDDE48BD3}
static const GUID guid0 = { 0xc0968e41, 0x7392, 0x4c8a, { 0x9a, 0xe7, 0xd2, 0xc, 0xdd, 0xe4, 0x8b, 0xd3 } };
// {83F99F40-717C-4598-B18F-959F37FCACEA}
static const GUID guid1 = { 0x83f99f40, 0x717c, 0x4598, { 0xb1, 0x8f, 0x95, 0x9f, 0x37, 0xfc, 0xac, 0xea } };
// {24EAE67F-21F0-4e7f-AB58-A2591DA1DC9E}
static const GUID guid2 = { 0x24eae67f, 0x21f0, 0x4e7f, { 0xab, 0x58, 0xa2, 0x59, 0x1d, 0xa1, 0xdc, 0x9e } };
// {96855F55-EF5B-4acc-8770-8D4A1EB57271}
static const GUID guid3 = { 0x96855f55, 0xef5b, 0x4acc, { 0x87, 0x70, 0x8d, 0x4a, 0x1e, 0xb5, 0x72, 0x71 } };
// {BF101B1E-8FDD-47a2-8AA1-E179BFA88177}
static const GUID guid4 = { 0xbf101b1e, 0x8fdd, 0x47a2, { 0x8a, 0xa1, 0xe1, 0x79, 0xbf, 0xa8, 0x81, 0x77 } };
//
static DWORD flags[5][2] = 
{
	{ 0x0f0f0f0f, 0xf0f0f0f0 },
	{ 0x11223344, 0x55667788 },
	{ 0x12345678, 0x9abcdef0 },
	{ 0x87654321, 0x0fedcba9 },
	{ 0x44332211, 0x88776655 }
};
//

static const SSimpleArrayTestData testdata[] = 
{
	{ "SimpleArrayInt", { 1, 2, 3, 4, 5 } },
	{ "SimpleArrayFloat", { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f } },
	//	{ "SimpleArrayBool", { true, false, true, false, true } },
	{ "SimpleArrayGUID", { guid0, guid1, guid2, guid3, guid4 } },
	{ "SimpleArrayBinaryFlags", { CVariant(flags[0], sizeof(flags[0])), CVariant(flags[1], sizeof(flags[1])), CVariant(flags[2], sizeof(flags[2])), CVariant(flags[3], sizeof(flags[3])), CVariant(flags[4], sizeof(flags[4])) } },
	{ "SimpleArrayEnumUnitType", { "UNIT_TYPE_INFANTRY_SNIPER", "UNIT_TYPE_ARMOR_MEDIUM", "UNIT_TYPE_ARMOR_HEAVY", "UNIT_TYPE_AVIA_FIGHTER", "UNIT_TYPE_AUTO_ENGINEER" } },
	{ "SimpleArrayString", { "Test string 0", "Test string 1", "Test string 2", "Test string 3", "Test string 4" } },
	{ "SimpleArrayWString", { L"Это очень клёвый wstring 0", L"Это очень клёвый wstring 1", L"Это очень клёвый wstring 2", L"Это очень клёвый wstring 3", L"Это очень клёвый wstring 4" } },
	//
	{ "SimpleArrayIntNocode", { 1, 2, 3, 4, 5 } },
	{ "SimpleArrayFloatNocode", { 1.1f, 2.2f, 3.3f, 4.4f, 5.5f } },
//	{ "SimpleArrayBoolNocode", { true, false, true, false, true } },
	{ "SimpleArrayGUIDNocode", { guid0, guid1, guid2, guid3, guid4 } },
	{ "SimpleArrayBinaryFlagsNocode", { CVariant(flags[0], sizeof(flags[0])), CVariant(flags[1], sizeof(flags[1])), CVariant(flags[2], sizeof(flags[2])), CVariant(flags[3], sizeof(flags[3])), CVariant(flags[4], sizeof(flags[4])) } },
	{ "SimpleArrayEnumUnitTypeNocode", { "UNIT_TYPE_INFANTRY_SNIPER", "UNIT_TYPE_ARMOR_MEDIUM", "UNIT_TYPE_ARMOR_HEAVY", "UNIT_TYPE_AVIA_FIGHTER", "UNIT_TYPE_AUTO_ENGINEER" } },
	{ "SimpleArrayStringNocode", { "Test string 0", "Test string 1", "Test string 2", "Test string 3", "Test string 4" } },
	{ "SimpleArrayWStringNocode", { L"Это очень клёвый wstring 0", L"Это очень клёвый wstring 1", L"Это очень клёвый wstring 2", L"Это очень клёвый wstring 3", L"Это очень клёвый wstring 4" } },
	{ "Weapons", { CDBID("weapons/ussr/heavy/a20.xdb"), CDBID("weapons/ussr/light/ppsh.xdb"), CDBID("weapons/ussr/heavy/a20.xdb"), CDBID("weapons/ussr/light/ppsh.xdb"), CDBID("weapons/ussr/heavy/a20.xdb") } },
	{ "", { 0, 0, 0, 0, 0 } }
};

bool CheckMetaGetForArray( NDb::IObjMan *pBind, const string &szFieldName, const int nPos, const CVariant &var )
{
	CVariant value;
	const string szFullFieldName = StrFmt( "%s.[%d]", szFieldName.c_str(), nPos );
	NI_VERIFY( pBind->GetValue( szFullFieldName, &value ) != false, StrFmt("Get %d value from array failed!", nPos), return false );
	NI_VERIFY( var == value, StrFmt("Values mismatch for %d element!", nPos), return false );
	return true;
}

bool CheckMetaSetGetForArrayElement( NDb::IObjMan *pBind, const string &szFieldName, const int nPos, const CVariant &var )
{
	CVariant value;
	const string szFullFieldName = StrFmt( "%s.[%d]", szFieldName.c_str(), nPos );
	NI_VERIFY( pBind->SetValue( szFullFieldName, var ) != false, StrFmt("Set %d value for array failed!", nPos), return false );
	NI_VERIFY( CheckMetaGetForArray( pBind, szFieldName, nPos, var ) != false, "Failed to check Meta Get for array", return false );
	return true;
}

bool TestSimpleArrayInsertRemove( NDb::IObjMan *pBind, const SSimpleArrayTestData *pData )
{
	const string szTestName = StrFmt( "Simple array \"%s\"", pData->szFieldName.c_str() );
	NTest::CMeasureTimer timer( szTestName.c_str() );
	CVariant value;
	// insert at the begining (2 elements)
	NI_VERIFY( pBind->Insert( pData->szFieldName, 0, 2 ) != false, "Insert at the begining failed", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, pData->szFieldName, 0, pData->vars[0] ) != false, "Set/get for array failed!", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, pData->szFieldName, 1, pData->vars[1] ) != false, "Set/get for array failed!", return false );
	// insert at the end (2 elements)
	NI_VERIFY( pBind->Insert( pData->szFieldName, -1, 2 ) != false, "Insert at the end failed", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, pData->szFieldName, 2, pData->vars[3] ) != false, "Set/get for array failed!", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, pData->szFieldName, 3, pData->vars[4] ) != false, "Set/get for array failed!", return false );
	// insert at the middle (1 element1)
	NI_VERIFY( pBind->Insert( pData->szFieldName, 2, 1 ) != false, "Insert at the middle failed", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 5 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, pData->szFieldName, 2, pData->vars[2] ) != false, "Set/get for array failed!", return false );
	// check all values
	for ( int i = 0; i < 5; ++i )
		NI_VERIFY( CheckMetaGetForArray( pBind, pData->szFieldName, i, pData->vars[i] ) != false, "Failed to perform Meta Get in array", return false );
	//
	// remove
	//
	// erase first element
	NI_VERIFY( pBind->Remove( pData->szFieldName, 0, 1 ) != false, "Failed to remove first element", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	for ( int i = 0; i < 4; ++i )
		NI_VERIFY( CheckMetaGetForArray( pBind, pData->szFieldName, i, pData->vars[i + 1] ) != false, "Failed to perform Meta Get in array", return false );
	// erase last element
	NI_VERIFY( pBind->Remove( pData->szFieldName, 3, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 3 == (int)value, "Can't get array size or wrong array size", return false );
	for ( int i = 0; i < 3; ++i )
		NI_VERIFY( CheckMetaGetForArray( pBind, pData->szFieldName, i, pData->vars[i + 1] ) != false, "Failed to perform Meta Get in array", return false );
	// all but first and last (middle)
	NI_VERIFY( pBind->Remove( pData->szFieldName, 1, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(pData->szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	NI_VERIFY( CheckMetaGetForArray( pBind, pData->szFieldName, 0, pData->vars[1] ) != false, "Failed to perform Meta Get in array", return false );
	NI_VERIFY( CheckMetaGetForArray( pBind, pData->szFieldName, 1, pData->vars[3] ) != false, "Failed to perform Meta Get in array", return false );
	// 
	return true;
}
bool TestSimpleArrayInsertRemove( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit )
{
	for ( const SSimpleArrayTestData *pData = testdata; !pData->szFieldName.empty(); ++pData )
		NI_VERIFY( TestSimpleArrayInsertRemove( pBind, pData ) != false, "Insert/remove failed!", return false );
	return true;
}

}

