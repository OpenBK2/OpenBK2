#include "StdAfx.h"

#include <crtdbg.h>
#include "MeasureTimer.h"
#include "TestType.h"
#include "../libdb/EditorDb.h"
#include "../libdb/ObjMan.h"
#include "../System/FileUtils.h"
#include "../System/XMLSAXParser.h"
#include "../System/WinVFS.h"
#include "../System/VFSOperations.h"

int CRAPTooSmartCompiler_DBTools_TypeDef();

namespace NTest
{
	bool TestMetaSetGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit );
	bool TestMetaSetDirectGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit );
	bool TestDirectSetMetaGet( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit );
	bool TestSimpleArrayInsertRemove( NDb::IObjMan *pBind, NDb::SMechUnit *pMechUnit );
	bool TestComplexArrayInsertRemove( NDb::IObjMan *pBind, const string &szFieldName );
	bool TestComplexArray2InsertRemove( NDb::IObjMan *pBind, const string &szFieldName );
	bool TestXmlSaxParser( const char *pszTestFileName );
	bool TestIterator( NDb::IObjMan *pObjMan );
}
namespace NDb
{
	bool RegisterResourceFile( const string &szFileName );
}

int main( int argc, char *argv[] )
{
	CreateMutex( 0, TRUE, "XML_DATABASE_TEST" ); // при выходе система сама уничтожит этот mutex
	if ( GetLastError() == ERROR_ALREADY_EXISTS )
		return 0xDEAD;
	//
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	const int nLeakId = -1;
	_CrtSetBreakAlloc( nLeakId );
	//

	string szDataPath;
	{
		char buff[1024];
		GetModuleFileName( 0, buff, 1024 );
		szDataPath = buff;
		szDataPath = szDataPath.substr( 0, szDataPath.rfind( '\\' ) );
		szDataPath += "\\..\\TestDB\\TestData\\";
//		szDataPath += "\\..\\DBTest\\TestData\\";
		szDataPath = NFile::GetFullName( szDataPath );
		if ( szDataPath[szDataPath.size() - 1] != '\\' )
			szDataPath += '\\';
	}

	CObj<NVFS::IVFS> pMainVFS = NVFS::CreateWinVFS( szDataPath );
	CObj<NVFS::IFileCreator> pMainFileCreator = NVFS::CreateWinFileCreator( szDataPath );
	NVFS::SetMainVFS( pMainVFS );
	NVFS::SetMainFileCreator( pMainFileCreator );

	NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR );
//		{
//			NTest::CMeasureTimer timer( "File system and database startup" );
//			//
////			AddFileCollector( NDb::CreateDBFilesCollector() );
//			OpenStorage( szDataPath );
//			NDb::SaveChanges();
//		}
//		//
//		{
//			NTest::CMeasureTimer timer( "test map loading" );
//			//
//			CObj<NDb::IObjMan> pObjMan = NDb::GetManipulator( CDBID("MapInfo/Game/GB/ChapterGB3/GB3.0/MapInfo.xdb") );
//
//			//
//			int a = 1;
//		}
//		return 0;
	//
	{
		NTest::CMeasureTimer timer( "Resource registering" );
		NDb::RegisterResourceFile( "object.xdb" );
		NDb::RegisterResourceFile( "object2.xdb" );
		NDb::RegisterResourceFile( "test_map.xdb" );
		NDb::RegisterResourceFile( "Campaigns/USSR/Stalingrad/test_map1.xdb" );
		NDb::RegisterResourceFile( "Units/Technics/USSR/Tanks/JS-3/object.xdb" );
		NDb::RegisterResourceFile( "Weapons/USSR/Heavy/A20.xdb" );
		NDb::RegisterResourceFile( "Weapons/USSR/Light/PPSh.xdb" );
	}
	//
//		{
//			NTest::CMeasureTimer timer( "Large map game load" );
//			const NDb::SMapInfo2 *pMapInfo = (NDb::SMapInfo2 *)NDb::GetObject( "Campaigns/USSR/Stalingrad/test_map1.xdb" );
//			const NDb::SHPObject *pObj = pMapInfo->objects[1111].pObject;
//			int a = 1;
//		}

//		{
//			NDb::GetObject( "Units/Technics/USSR/Tanks/JS-3/object.xdb" );
//			NTest::CMeasureTimer timer( "Large map iteration time" );
//			CObj<NDb::IObjMan> pMap = NDb::GetManipulator( "Campaigns/USSR/Stalingrad/test_map1.xdb" );
//			for ( CObj<NDb::IObjManIterator> pIt = pMap->CreateIterator(); !pIt->IsEnd(); pIt->Next() )
//			{
//				const string szName = pIt->GetName();
//				const NDb::NTypeDef::STypeStructBase::SField *pField = pIt->GetDesc();
//				CVariant value;
//				pMap->GetValue( szName, &value );
//			}
//		}
//		CPtr<NDb::SMapInfo> pMapInfo = checked_cast<NDb::SMapInfo *>( pMap->GetObject() );

	NI_VERIFY( NTest::TestXmlSaxParser("test.xml") != false, "XML SAX parser test failed!", return false );
	NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
	//
	{
//			CObj<NDb::IObjMan> pObject2 = NDb::GetManipulator( "object4.xdb" );
		
		CObj<NDb::IObjMan> pObjMan = NDb::CreateNewObject( "MechUnit" );
		CObj<NDb::SMechUnit> pMechUnit = dynamic_cast<NDb::SMechUnit *>( pObjMan->GetObject() );
		//
		//
		//
		//
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestMetaSetGet( pObjMan, pMechUnit );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestMetaSetDirectGet( pObjMan, pMechUnit );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestDirectSetMetaGet( pObjMan, pMechUnit );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		//
		NTest::TestSimpleArrayInsertRemove( pObjMan, pMechUnit );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestComplexArrayInsertRemove( pObjMan, "ComplexArrayStruct1" );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestComplexArray2InsertRemove( pObjMan, "ComplexArrayStruct2" );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		//
		CObj<NDb::IObjMan> pObject2 = NDb::GetManipulator( "object2.xdb" );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
		NTest::TestIterator( pObject2 );
		NI_ASSERT( _CrtCheckMemory() == TRUE, "Memory check failed!" );
//		pArrayMan->SetValue( "TypeFloat", 1212.3f );
//		float fVal = 0;
//		static_cast_ptr<NDb::IObjMan*>(pObjMan)->GetValue( "ComplexArrayStruct1.[1].TypeFloat", &fVal );
//		pObjMan->Remove( "ComplexArrayStruct1", 0, 1 );
//		pArrayMan->GetValue( "TypeFloat", &fVal );


		pObjMan->SetValue( "Jx.Tremble.y", 9876.4f );
		pObjMan->SetValue( "Name", L"This is a nice test!" );
		//
		pObjMan->SetAttribute( "Owner", L"Юрий Блажевич" );
		pObjMan->SetAttribute( "MOD", L"Мой любимый MOD" );
		pObjMan->SetAttribute( "Color", L"0xff00ff00" );
		pObjMan->SetAttribute( "ObjectRecordID", L"1234" );
		//
		NDb::AddNewObject( "object3.xdb", CDBID("object3.xdb"), pObjMan );
	}
	NDb::SaveChanges();
	NDb::CloseDatabase();
	// remove index and object3
	::DeleteFile( (szDataPath + "index.bin").c_str() );
	::DeleteFile( (szDataPath + "object3.xdb").c_str() );
	//
	return 0;
}


