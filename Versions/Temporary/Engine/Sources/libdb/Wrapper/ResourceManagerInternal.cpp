#include "StdAfx.h"

#include <time.h>

#include "../../Misc/HPTimer.h"
#include "../../Misc/StrProc.h"
#include "../../System/FileUtils.h"
#include "../../System/FilePath.h"

#include "../EditorDb.h"
#include "../ObjectRecordIDAllocator.h"
#include "../DBWatcherClient.h"

#include "ResourceManagerInternal.h"
#include "TableManipulator.h"
#include "FolderManipulator.h"
#include "ObjectManipulator.h"

using namespace NDb::NTypeDef;

void IResourceManager::InitSingleton()
{
	NSingleton::RegisterSingleton( new CResourceManagerWrapper(), IResourceManager::tidTypeID );
}

void IResourceManager::UninitSingleton()
{
	NSingleton::UnRegisterSingleton( IResourceManager::tidTypeID );
}


CResourceManagerWrapper::CResourceManagerWrapper()
{
}

CResourceManagerWrapper::~CResourceManagerWrapper()
{
}

struct SConstUserDataForResMan
{
	string szExportSourceFolder;
	string szExportDestinationFolder;
	string szObjectRecordIDsFolder;
	//
	int operator&( IXmlSaver &saver )
	{
		saver.Add( "ExportSourceFolder", &szExportSourceFolder );
		saver.Add( "ExportDestinationFolder", &szExportDestinationFolder );
		saver.Add( "ObjectRecordIDsFolder", &szObjectRecordIDsFolder );
		return 0;
	}
};

void CResourceManagerWrapper::SetConfig( const SDBConfig &config )
{
	szSrcPath = config.szSrcFilePath;
	szDstPath = config.szDstFilePath;
	//
	if ( !config.szObjectRecordIDsPath.empty() )
		NDb::NObjectIDAllocator::SetObjectRecordIDsFolderName( config.szObjectRecordIDsPath );
	else
		NDb::NObjectIDAllocator::SetObjectRecordIDsFolderName( szSrcPath );
}

IManipulator* CResourceManagerWrapper::CreateTableManipulator()
{
	vector<STypeClass*> classes;
	NDb::GetClassesList( &classes );
	return new CTableManipulatorWrapper( classes );
}

IManipulator* CResourceManagerWrapper::CreateFolderManipulator( const string &szClassTypeName )
{
	return new CFolderManipulatorWrapper( szClassTypeName, szSrcPath, szDstPath );
}

static string GetClassTypeName( const int nClassTypeID )
{
	vector<STypeClass*> classes;
	NDb::GetClassesList( &classes );
	for ( int i = 0; i < classes.size(); ++i )
	{
		if ( classes[i]->nClassTypeID == nClassTypeID )
			return classes[i]->szTypeName;
	}
	return 0;
}

IManipulator* CResourceManagerWrapper::CreateFolderManipulator( int nClassTypeID )
{
	const string szClassTypeName = GetClassTypeName( nClassTypeID );
	return szClassTypeName.empty() ? 0 : CreateFolderManipulator( szClassTypeName );
}

IManipulator* CResourceManagerWrapper::CreateObjectManipulator( const string &szTypeName, const string &szObjectName )
{
	if ( NDb::IObjMan *pMan = NDb::GetManipulator(CDBID(szObjectName)) )
		return new CObjectManipulatorWrapper( pMan );
	return 0;
}

IManipulator* CResourceManagerWrapper::CreateObjectManipulator( const string &szTypeName, int nID )
{
	vector<STypeClass*> classes;
	NDb::GetClassesList( &classes );
	for ( int i = 0; i < classes.size(); ++i )
	{
		if ( classes[i]->szTypeName == szTypeName )
			return CreateObjectManipulator( classes[i]->nClassTypeID, nID );
	}
	return 0;
}

IManipulator* CResourceManagerWrapper::CreateObjectManipulator( int nTypeID, int nID )
{
	return 0;
}

IManipulator* CResourceManagerWrapper::CreateObjectManipulator( const string &szTypeName, const CDBID &rCDBID )
{
	if ( NDb::IObjMan *pMan = NDb::GetManipulator( rCDBID ) )
		return new CObjectManipulatorWrapper( pMan );
	return 0;
}


IManipulator* CResourceManagerWrapper::CreateObjectManipulator( const string &szObjectName )
{
	if ( NDb::IObjMan *pMan = NDb::GetManipulator(CDBID(szObjectName)) )
		return new CObjectManipulatorWrapper( pMan );
	return 0;
}


IManipulator* CResourceManagerWrapper::CreateObjectManipulator( const CDBID &rCDBID )
{
	if ( NDb::IObjMan *pMan = NDb::GetManipulator( rCDBID ) )
		return new CObjectManipulatorWrapper( pMan );
	return 0;
}


bool CResourceManagerWrapper::SerializeObject( CDataStream *pStream, int nTypeID, int nObjectID )
{
	return false;
}

void CResourceManagerWrapper::SerializeObjects( const string &szFile, const string &szTypeName, const string &szRootObject )
{
}

void CResourceManagerWrapper::SerializeObjects( const string &szFile )
{
}

void CResourceManagerWrapper::FillReferencingObjects( bool *pServiceIsReady, const string &szTypeName, const string &szObjectName, list<string> &results )
{
	results.clear();
	if ( NDBWatcherClient::IDBWatcherClient *pClient = Singleton<NDBWatcherClient::IDBWatcherClient>() )
	{
		vector<CDBID> referencingObjs;
		const NDBWatcherClient::IDBWatcherClient::EResult eClientResult = pClient->GetReferencingObjects( szObjectName, &referencingObjs );
		if ( eClientResult == NDBWatcherClient::IDBWatcherClient::EResult::COMPLETE )
		{
			for ( vector<CDBID>::const_iterator it = referencingObjs.begin(); it != referencingObjs.end(); ++it )
			{
				string szFileName;
				NFile::NormalizePath( &szFileName, NDb::GetFileName(*it) );
				results.push_back( szFileName );
			}
			if ( pServiceIsReady )
				*pServiceIsReady = true;
		}
		else
		{
			if ( pServiceIsReady )
				*pServiceIsReady = ( eClientResult != NDBWatcherClient::IDBWatcherClient::EResult::SERVICE_NOT_READY );
		}
	}
}

// CRAP{ remove it ASAP
bool CResourceManagerWrapper::HasReferencingObjects( bool *pServiceIsReady, const string &szTypeName, int nObjectID )
{
	return false;
}
// CRAP}

void CResourceManagerWrapper::ResetCache()
{
	NDb::DropCachedResources();
}

void CResourceManagerWrapper::SyncDB()
{
	NDb::SaveChanges();
}

bool CResourceManagerWrapper::CanSyncDB()
{
	return NDb::HasChangedObjects();
}

void CResourceManagerWrapper::SetDataDir( const string &dataDir )
{
}

bool CResourceManagerWrapper::CheckIn()
{
	return false;
}

bool CResourceManagerWrapper::CheckOut()
{
	return false;
}

bool CResourceManagerWrapper::GetLatest()
{
	return false;
}

bool CResourceManagerWrapper::InitializeVersionControl()
{
	return true;
}

REGISTER_SAVELOAD_CLASS( 0x10074AC0, CResourceManagerWrapper );

