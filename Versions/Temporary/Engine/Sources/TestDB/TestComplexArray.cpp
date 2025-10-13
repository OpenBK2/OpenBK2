#include "stdafx.h"
#include "testtype.h"
#include "MeasureTimer.h"
#include "libdb/ObjMan.h"

#include <cstdint>

namespace NTest
{

// TODO{
// * add test to remove at position '-1'
// TODO}

// {A3965C8E-B3EA-472f-8289-4117D37BC8AA}
static const GUID guid0 = { 0xa3965c8e, 0xb3ea, 0x472f, { 0x82, 0x89, 0x41, 0x17, 0xd3, 0x7b, 0xc8, 0xaa } };
// {D355FB02-F8C8-4b16-A387-73F3FCBE4A5B}
static const GUID guid1 = { 0xd355fb02, 0xf8c8, 0x4b16, { 0xa3, 0x87, 0x73, 0xf3, 0xfc, 0xbe, 0x4a, 0x5b } };
// {368B1489-11B0-45d5-9BE2-28148111A71A}
static const GUID guid2 = { 0x368b1489, 0x11b0, 0x45d5, { 0x9b, 0xe2, 0x28, 0x14, 0x81, 0x11, 0xa7, 0x1a } };
// {4EC8482F-C39F-41fb-AA5D-1900E43D6BE6}
static const GUID guid3 = { 0x4ec8482f, 0xc39f, 0x41fb, { 0xaa, 0x5d, 0x19, 0x0, 0xe4, 0x3d, 0x6b, 0xe6 } };
// {C6DA102B-7AE6-4ce8-AB94-C222A7A2721E}
static const GUID guid4 = { 0xc6da102b, 0x7ae6, 0x4ce8, { 0xab, 0x94, 0xc2, 0x22, 0xa7, 0xa2, 0x72, 0x1e } };
//
static uint32_t flags[5][2] =
{
	{ 0x0f0f0f0f, 0xf0f0f0f0 },
	{ 0x11223344, 0x55667788 },
	{ 0x12345678, 0x9abcdef0 },
	{ 0x87654321, 0x0fedcba9 },
	{ 0x44332211, 0x88776655 }
};

struct STestDataStruct1
{
	int nVal;
	float fVal;
	bool bVal;
	GUID guidVal;
	string szVal;
	wstring wszVal;
	string szUnitType;
	NDb::CBinaryFlags flagsVal;
};

static const STestDataStruct1 s_ComplexArrayTestData[5] = 
{
	{ 1, 1.0f, true , guid0, "test string 1", L"test wstring 1", "UNIT_TYPE_INFANTRY_SNIPER", flags[0] },
	{ 2, 2.0f, false, guid1, "test string 2", L"test wstring 2", "UNIT_TYPE_ARMOR_MEDIUM", flags[1] },
	{ 3, 3.0f, true , guid2, "test string 3", L"test wstring 3", "UNIT_TYPE_ARMOR_HEAVY", flags[2] },
	{ 4, 4.0f, false, guid3, "test string 4", L"test wstring 4", "UNIT_TYPE_AVIA_FIGHTER", flags[3] },
	{ 5, 5.0f, true , guid4, "test string 5", L"test wstring 5", "UNIT_TYPE_SPG_ASSAULT", flags[4] },
};

bool CheckMetaSetGet( NDb::IObjMan *pBind, const string &szFieldName, const CVariant &value )
{
	CVariant var;
	NI_VERIFY( pBind->SetValue( szFieldName, value ) != false, StrFmt("Failed to set value \"%s\" to complex array", szFieldName.c_str()), return false );
	NI_VERIFY( pBind->GetValue( szFieldName, &var ) != false, StrFmt("Failed to get value \"%s\" from complex array", szFieldName.c_str()), return false );
	NI_VERIFY( var == value, "Values mistmatch after set/get!", return false );
	return true;
}

bool CheckMetaSetGetForArrayElement( NDb::IObjMan *pBind, const string &szFieldName, const int nIndex, const STestDataStruct1 *pData )
{
	const string szStructName = StrFmt( "%s.[%d].", szFieldName.c_str(), nIndex );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeInt", pData->nVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeFloat", pData->fVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeBool", pData->bVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeGUID", pData->guidVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeString", pData->szVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeWString", pData->wszVal) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeEnumUnitType", pData->szUnitType) != false, "Failed set/get test for field in array element!", return false );
	NI_VERIFY( CheckMetaSetGet(pBind, szStructName + "TypeBinaryFlags", pData->flagsVal) != false, "Failed set/get test for field in array element!", return false );
	return true;
}

bool TestComplexArrayInsertRemove( NDb::IObjMan *pBind, const string &szFieldName )
{
	const string szTestName = StrFmt( "Complex array \"%s\"", szFieldName.c_str() );
	NTest::CMeasureTimer timer( szTestName.c_str() );
	CVariant value;
	// insert at the begining (2 elements)
	NI_VERIFY( pBind->Insert( szFieldName, 0, 2 ) != false, "Insert at the begining failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 0, &s_ComplexArrayTestData[0] ) != false, "Set/get for array failed!", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 1, &s_ComplexArrayTestData[1] ) != false, "Set/get for array failed!", return false );
	// insert at the end (2 elements)
	NI_VERIFY( pBind->Insert( szFieldName, -1, 2 ) != false, "Insert at the end failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 2, &s_ComplexArrayTestData[3] ) != false, "Set/get for array failed!", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 3, &s_ComplexArrayTestData[4] ) != false, "Set/get for array failed!", return false );
	// insert at the middle (1 element1)
	NI_VERIFY( pBind->Insert( szFieldName, 2, 1 ) != false, "Insert at the middle failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 5 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 2, &s_ComplexArrayTestData[2] ) != false, "Set/get for array failed!", return false );
	// check all values
	for ( int i = 0; i < 5; ++i )
		NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, i, &s_ComplexArrayTestData[i] ) != false, "Failed to perform Meta Get in array", return false );
	//
	// remove
	//
	// erase first element
	NI_VERIFY( pBind->Remove( szFieldName, 0, 1 ) != false, "Failed to remove first element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	for ( int i = 0; i < 4; ++i )
		NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, i, &s_ComplexArrayTestData[i + 1] ) != false, "Failed to perform Meta Get in array", return false );
	// erase last element
	NI_VERIFY( pBind->Remove( szFieldName, 3, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 3 == (int)value, "Can't get array size or wrong array size", return false );
	for ( int i = 0; i < 3; ++i )
		NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, i, &s_ComplexArrayTestData[i + 1] ) != false, "Failed to perform Meta Get in array", return false );
	// all but first and last (middle)
	NI_VERIFY( pBind->Remove( szFieldName, 1, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 0, &s_ComplexArrayTestData[1] ) != false, "Failed to perform Meta Get in array", return false );
	NI_VERIFY( CheckMetaSetGetForArrayElement( pBind, szFieldName, 1, &s_ComplexArrayTestData[3] ) != false, "Failed to perform Meta Get in array", return false );
	// 
	return true;
}

bool TestComplexArray2InsertRemove( NDb::IObjMan *pBind, const string &szFieldName )
{
	const string szTestName = StrFmt( "Complex array 2 \"%s\"", szFieldName.c_str() );
	NTest::CMeasureTimer timer( szTestName.c_str() );
	CVariant value;
	string szFullFieldName = szFieldName;
	// insert at the begining (2 elements)
	NI_VERIFY( pBind->Insert( szFieldName, 0, 2 ) != false, "Insert at the begining failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( TestComplexArrayInsertRemove( pBind, szFullFieldName + ".[0].Structs" ) != false, "Test complex array-in-array failed for 0 element", return false );
	NI_VERIFY( TestComplexArrayInsertRemove( pBind, szFullFieldName + ".[1].Structs" ) != false, "Test complex array-in-array failed for 1 element", return false );
	// insert at the end (2 elements)
	NI_VERIFY( pBind->Insert( szFieldName, -1, 2 ) != false, "Insert at the end failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( TestComplexArrayInsertRemove( pBind, szFullFieldName + ".[2].Structs" ) != false, "Test complex array-in-array failed for 2 element", return false );
	NI_VERIFY( TestComplexArrayInsertRemove( pBind, szFullFieldName + ".[3].Structs" ) != false, "Test complex array-in-array failed for 3 element", return false );
	// insert at the middle (1 element1)
	NI_VERIFY( pBind->Insert( szFieldName, 2, 1 ) != false, "Insert at the middle failed", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 5 == (int)value, "Can't get array size or wrong array size", return false );
	// set/get values
	NI_VERIFY( TestComplexArrayInsertRemove( pBind, szFullFieldName + ".[2].Structs" ) != false, "Test complex array-in-array failed for 2 element", return false );
	//
	// erase first element
	NI_VERIFY( pBind->Remove( szFieldName, 0, 1 ) != false, "Failed to remove first element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	// erase last element
	NI_VERIFY( pBind->Remove( szFieldName, 3, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 3 == (int)value, "Can't get array size or wrong array size", return false );
	// all but first and last (middle)
	NI_VERIFY( pBind->Remove( szFieldName, 1, 1 ) != false, "Failed to remove last element", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 2 == (int)value, "Can't get array size or wrong array size", return false );
	//
	NI_VERIFY( pBind->SetValue( szFieldName, 3 ) != false, "Failed to resize to 3 elements", return false );
	NI_VERIFY( pBind->GetValue(szFieldName, &value) != false && 3 == (int)value, "Can't get array size or wrong array size", return false );
	NI_VERIFY( pBind->SetValue( szFieldName + ".[2].Structs", 4 ) != false, "Failed to resize to 3 elements", return false );
	NI_VERIFY( pBind->GetValue(szFieldName + ".[2].Structs", &value) != false && 4 == (int)value, "Can't get array size or wrong array size", return false );
	NI_VERIFY( pBind->SetValue( szFieldName + ".[2].Structs.[1].TypeInt", 1234567890) != false, "Failed to set value", return false );
	NDb::SMechUnit *pMechUnit = dynamic_cast<NDb::SMechUnit *>( pBind->GetObject() );
	NI_VERIFY( pMechUnit->complexArrayStruct2[2].structs[1].nTypeInt == 1234567890, "Meta set direct get failed for complex array", return false );
	// 
	return true;
}

}


