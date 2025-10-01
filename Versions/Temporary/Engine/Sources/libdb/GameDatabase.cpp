#include <libdb_export.h>

#include "StdAfx.h"
#include "Database.h"
#include "Index.h"
#include "../libdb/TypeDef.h"
#include "../System/VFS.h"
#include "../System/xmlreader.h"
#include "../Misc/Win32Helper.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define _PROFILER
namespace NTest
{
	void CreateTestTypes( vector< CObj<NDb::NTypeDef::STypeDef> > *pTopLevelTypes );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDb
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int s_nMaxLoadDepth = 1;
static int s_nCurrLoadDepth = 0;
static int s_nForceLoadCounter = 0;
static bool s_bForceLoad = true;
struct SGameDbForceLoadGuard
{
	bool bOpened;
	SGameDbForceLoadGuard()
	{
		s_bForceLoad = s_nCurrLoadDepth < s_nMaxLoadDepth;
		++s_nCurrLoadDepth;
		bOpened = true;
	}
	~SGameDbForceLoadGuard()
	{
		Close();
	}
	void Close()
	{
		if ( bOpened )
			--s_nCurrLoadDepth;
		bOpened = false;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CGameDatabase : public CBasicDatabase
{
	OBJECT_NOCOPY_METHODS( CGameDatabase );
	//
	struct SElement
	{
		STypeObjectHeader typeHeader;
		CObj<CResource> pObj;
	};
	//
	typedef hash_map<CDBID, SElement> CElementsMap;
	CElementsMap elementsMap;
	typedef hash_map<string, int> CName2TypeIDMap;
	CName2TypeIDMap name2typeIDmap;
	bool bIndexChanged;
	//
	const SElement *GetElement( const CDBID &_dbid )
	{
		CDBID dbid;
		NormalizeDBID( &dbid, _dbid );
		if ( DoesObjectExist(dbid) == false )
			return 0;
		else
			return &( elementsMap[dbid] );
	}
	//
	bool ReallyRegisterResourceFile( const CDBID &dbid );
	CResource *GetRawObject( const CDBID &dbid );
	bool DoesObjectExist( const CDBID &dbid );
	//
	void ResetIndexChanged() { bIndexChanged = false; }
	void RegisterObject( const SFullTypeHeader &hdr );
	bool LoadTypesMap();
public:
	CGameDatabase(): bIndexChanged(false) {}
	//
	bool OpenDatabase( NVFS::IVFS *pVFS, NVFS::IFileCreator *pFileCreator );
	bool SaveChangedIndex();
	bool RegisterResourceFile( const string &szFileName );
	bool IsFileRegistered( const string &szFileName );
	void SetLoadDepth( int nLoadDepth ) { s_nMaxLoadDepth = nLoadDepth; }
	//
	CResource *GetObject( const CDBID &dbid );
	// editor-specific functionality
	IObjMan *GetManipulator( const CDBID &dbid ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return 0; }
	IObjMan *CreateNewObject( const string &szClassTypeName ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return 0; }
	bool AddNewObject( const string &szFilePath, const CDBID &dbid, IObjMan *pObjMan ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return false; }
	bool RemoveObject( const CDBID &dbid ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return false; }
	bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return false; }
	void MarkChanged( const CDBID &dbid ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); }
	void SaveChanges();
	void DropCachedResources() { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); }
	bool GetClassesList( vector<NTypeDef::STypeClass*> *pRes ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return false; }
	bool GetObjectsList( vector<CDBID> *pRes, const string &szClassTypeName ) { NI_ASSERT( false, "Editor-specific functionality doesn't work in game mode!" ); return false; }
	bool GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID );
	string GetClassTypeName( const CDBID &_dbid )
	{
		if ( const SElement *pElement = GetElement(_dbid) )
			return pElement->typeHeader.szClassTypeName;
		else
			return "";
	}
};
CBasicDatabase *CreateGameDatabase() { return new CGameDatabase(); }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::OpenDatabase( NVFS::IVFS *_pVFS, NVFS::IFileCreator *_pFileCreator ) 
{ 
	SetFileSystem( _pVFS, _pFileCreator );
	//
	bool bRet = LoadTypesMap(); 
	LoadIndex(); 
	return bRet; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGameDatabase::RegisterObject( const SFullTypeHeader &hdr )
{
	CDBID dbid;
	NormalizeDBID( &dbid, CDBID(hdr.szFileName) );
	elementsMap[dbid].typeHeader = *( static_cast<const STypeObjectHeader*>(&hdr) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::LoadTypesMap()
{
	vector< CObj<NDb::NTypeDef::STypeDef> > topLevelTypes;
//	NTest::CreateTestTypes( &topLevelTypes );
	CFileStream stream( GetVFS(), TYPES_FILE_NAME );
	if ( stream.IsOk() )
	{
		if ( CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ) )
		{
			pSaver->Add( "Types", &topLevelTypes );
		}
	}
	// build typeName => typeID map
	for ( vector< CObj<NDb::NTypeDef::STypeDef> >::const_iterator it = topLevelTypes.begin(); it != topLevelTypes.end(); ++it )
	{
		if ( (*it)->eType == NDb::NTypeDef::TYPE_TYPE_CLASS )
		{
			NDb::NTypeDef::STypeClass *pTypeClass = checked_cast_ptr<NDb::NTypeDef::STypeClass*>( *it );
			if ( !pTypeClass->szTypeName.empty() && pTypeClass->nClassTypeID != -1 )
				name2typeIDmap[pTypeClass->szTypeName] = pTypeClass->nClassTypeID;
		}
	}
	return !topLevelTypes.empty();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::SaveChangedIndex()
{
	if ( bIndexChanged )
	{
		CFileStream stream( GetFileCreator(), INDEX_FILE_NAME );
		if ( stream.IsOk() )
		{
			if ( CPtr<IBinSaver> pSaver = CreateBinSaver(&stream, SAVER_MODE_WRITE) )
			{
				// save changed index
				vector<SFullTypeHeader> objectsIndex( elementsMap.size() );
				int i = 0;
				for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it, ++i )
				{
					objectsIndex[i].szFileName = GetFileName( it->first );
					*( static_cast<STypeObjectHeader*>(&objectsIndex[i]) ) = it->second.typeHeader;
				}
				//
//				sort( objectsIndex.begin(), objectsIndex.end() );
				pSaver->Add( 1, &objectsIndex );
				bIndexChanged = false;
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	bIndexChanged = false;
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::DoesObjectExist( const CDBID &dbid )
{
	if ( elementsMap.find(dbid) != elementsMap.end() )
		return true;
	else
		return RegisterResourceFile( GetFileName(dbid) );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::RegisterResourceFile( const string &szFileName )
{
//	CDBID dbid, _dbid( szFileName );
//	NormalizeDBID( &dbid, _dbid );
	CDBID dbid( szFileName );
	//
	CElementsMap::iterator pos = elementsMap.find( dbid );
	if ( pos == elementsMap.end() )
	{
		if ( ReallyRegisterResourceFile(dbid) == false )
			return false;
		bIndexChanged = true;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::IsFileRegistered( const string &szFileName )
{
	CDBID dbid( szFileName );
	return elementsMap.find( dbid ) != elementsMap.end();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::ReallyRegisterResourceFile( const CDBID &dbid )
{
	NI_VERIFY( elementsMap.find( dbid ) == elementsMap.end(), StrFmt("Resource \"%s\" already exist!", dbid.ToString().c_str()), return false );
	// read object header
	STypeObjectHeader header;
	if ( ReadResourceHeader( &header, dbid ) )
	{
		elementsMap[dbid].typeHeader = header;
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CResource *CGameDatabase::GetRawObject( const CDBID &dbid )
{
	if ( DoesObjectExist(dbid) == false )
		return false;
	else
	{
		CElementsMap::iterator posElement = elementsMap.find( dbid );
		if ( posElement->second.pObj != 0 )
			return posElement->second.pObj;
		else
		{
			CName2TypeIDMap::const_iterator posTypeID = name2typeIDmap.find( posElement->second.typeHeader.szClassTypeName );
			NI_VERIFY( posTypeID != name2typeIDmap.end(), StrFmt("Can't find class typeID for \"%s\"", posElement->second.typeHeader.szClassTypeName.c_str()), return 0 );
			posElement->second.pObj = MakeObject<CResource>( posTypeID->second );
			NI_VERIFY( posElement->second.pObj != 0, StrFmt("Can't create object of type 0x%.8x", posTypeID->second), return 0 );
			CResourceHelper::SetDBID( posElement->second.pObj, dbid );
			return posElement->second.pObj;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DWORD dwLastProfilerSegment = 0;
static float fBigTimeForLoad = 0.0f;
static int nObjLoaded = 0;

class CProfiler
{
	const string szObjName;
	const DWORD dwStartTime;
public:
	CProfiler( const CDBID &dbID ) : szObjName( dbID.ToString() ), dwStartTime( GetTickCount() ) { }
	~CProfiler()
	{
		const float fLoadTime = float(GetTickCount() - dwStartTime)/1000.0f;
//		if ( fLoadTime > 0.1 )
//			DbgTrc( "load: %s loaded in %f sec", szObjName.c_str(), fLoadTime );

		fBigTimeForLoad += fLoadTime;
		++nObjLoaded;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LIBDB_EXPORT void SegmentProfiler()
{
	DWORD dwTime = GetTickCount();
	if ( dwTime - dwLastProfilerSegment > 1000 )
	{
		if ( fBigTimeForLoad != 0.0f )
			DbgTrc( "load: %f sec in segment, %d objs loaded", fBigTimeForLoad, nObjLoaded );

		dwLastProfilerSegment = dwTime;
		fBigTimeForLoad = 0.0f;
		nObjLoaded = 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _PROFILER
#include <vtuneapi.h>
#pragma comment( lib, "vtuneapi.lib" )
#endif

struct SVTuneProfiler
{
	SVTuneProfiler()
	{
#ifdef _PROFILER
			VTResume();
#endif
	}
	~SVTuneProfiler() 
	{
#ifdef _PROFILER	
		VTPause(); 
#endif
	}
};


CResource *CGameDatabase::GetObject( const CDBID &dbid )
{
	SGameDbForceLoadGuard forceLoadGuard;
	//
//	CDBID dbid;
//	NormalizeDBID( &dbid, _dbid );
	CResource *pObj = GetRawObject( dbid );
	if ( pObj == 0 )
		return 0;
	if ( !s_bForceLoad || pObj->IsLoaded() )
		return pObj;
	else
	{
		CFileStream stream( GetVFS(), GetFileName(dbid) );
		// here we must set special rounding and precision state to be sync during multiplayer
		NWin32Helper::CControl87Guard control87guard;
		//SVTuneProfiler profiler;
		_control87( _RC_CHOP | _PC_24, _MCW_RC | _MCW_PC );
		if ( stream.IsOk() )
		{
			CProfiler profiler( dbid );
			//			DebugTrace( "Loading DB object \"%s\"", GetFileName(dbid).c_str() );
			CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
			NI_VERIFY( pSaver != 0, StrFmt("Can't create saver to load DB object \"%s\"", dbid.ToString().c_str()), return 0 );
			pSaver->AddPolymorphicBase( 0, pObj );
			CResourceHelper::SetLoaded( pObj );
			forceLoadGuard.Close();
			CResourceHelper::CallPostLoad( pObj, false );
			return pObj;
		}
		NI_ASSERT( false, StrFmt("Can't create stream to load DB object \"%s\"", dbid.ToString().c_str()) );
		return 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGameDatabase::GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID )
{
	string szClassTypeName;
	// first, find class type name
	for ( CName2TypeIDMap::const_iterator it = name2typeIDmap.begin(); it != name2typeIDmap.end(); ++it )
	{
		if ( it->second == nClassTypeID )
		{
			szClassTypeName = it->first;
			break;
		}
	}
	NI_VERIFY( !szClassTypeName.empty(), StrFmt("Can't find class type name for 0x%.8x", nClassTypeID), return false );
	// get objects list by class type name
	pRes->resize( 0 );
	pRes->reserve( 512 );
	for ( CElementsMap::const_iterator it = elementsMap.begin(); it != elementsMap.end(); ++it )
	{
		if ( it->second.typeHeader.szClassTypeName == szClassTypeName )
			pRes->push_back( it->first );
	}
	return !pRes->empty();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGameDatabase::SaveChanges()
{
	SaveChangedIndex();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
