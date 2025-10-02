#include "StdAfx.h"


#include "Database.h"
#include "EditorDb.h"
#include "HeaderXmlSaxVisitor.h"
#include "StructMetaInfo.h"
#include "ReportMetaInfo.h"
#include "DBObserver.h"
#include "Index.h"
#include "Misc/StrProc.h"
#include "System/VFS.h"
#include "System/FilePath.h"

BASIC_REGISTER_CLASS(NDb::IDbObserver);


namespace NDb
{

CBasicDatabase *CreateEditorDatabase();
CBasicDatabase *CreateGameDatabase();

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

static CObj<CBasicDatabase> s_pMainDatabase;
static struct SDatabaseAutoMagic
{
	~SDatabaseAutoMagic()
	{
		CloseDatabase();
	}
} aDatabaseAutoMagic;

void SetDatabaseDataChanged() { if ( s_pMainDatabase ) s_pMainDatabase->SetDataChanged(); }
void SetDatabaseDataChanged( const CDBID &dbid ) { if ( s_pMainDatabase ) s_pMainDatabase->SetDataChanged( dbid ); }

bool OpenDatabase( NVFS::IVFS *pVFS, NVFS::IFileCreator *pFileCreator, EDatabaseMode eMode )
{
	if ( eMode == DATABASE_MODE_EDITOR )
	{
		s_pMainDatabase = CreateEditorDatabase();
	}
	else if ( eMode == DATABASE_MODE_GAME )
	{
		s_pMainDatabase = CreateGameDatabase();
		NMetaInfo::DropMetaInfo();
	}
	else
	{
		NI_ASSERT( false, "other modes still not supported!" );
		return false;
	}
	return s_pMainDatabase->OpenDatabase( pVFS, pFileCreator );
}

void CloseDatabase()
{
	s_pMainDatabase = 0;
}

void SetLoadDepth( int nLoadDepth )
{
	if ( s_pMainDatabase )
		s_pMainDatabase->SetLoadDepth( nLoadDepth );
}

bool RegisterResourceFile( const string &szFileName )
{
	return s_pMainDatabase->RegisterResourceFile( szFileName );
}

bool IsFileRegistered( const string &szFileName )
{
	return s_pMainDatabase->IsFileRegistered( szFileName );
}

IObjMan *GetManipulator( const CDBID &dbid )
{
	return dbid.IsEmpty() ? 0 : s_pMainDatabase->GetManipulator( dbid );
}

CResource *GetObject( const CDBID &dbid )
{
	return dbid.IsEmpty() ? 0 : s_pMainDatabase->GetObject( dbid );
}

bool DoesObjectExist( const CDBID &dbid )
{
	return dbid.IsEmpty() ? false : s_pMainDatabase->DoesObjectExist( dbid );
}

IObjMan *CreateNewObject( const string &szClassTypeName )
{
	return s_pMainDatabase->CreateNewObject( szClassTypeName );
}

bool AddNewObject( const string &szFilePath, const CDBID &dbid, IObjMan *pObjMan )
{
	return s_pMainDatabase->AddNewObject( szFilePath, dbid, pObjMan );
}

bool RemoveObject( const CDBID &dbid )
{
	return s_pMainDatabase->RemoveObject( dbid );
}

bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew )
{
	return s_pMainDatabase->RenameObject( dbidOld, dbidNew );
}

void MarkChanged( const CDBID &dbid )
{
	s_pMainDatabase->MarkChanged( dbid );
}

void SaveChanges()
{
	if ( s_pMainDatabase )
		s_pMainDatabase->SaveChanges();
}

void DropCachedResources()
{
	if ( s_pMainDatabase )
		s_pMainDatabase->DropCachedResources();
}

bool GetClassesList( vector<NTypeDef::STypeClass*> *pRes )
{
	return s_pMainDatabase->GetClassesList( pRes );
}

bool GetObjectsList( vector<CDBID> *pRes, const string &szClassTypeName )
{
	return s_pMainDatabase->GetObjectsList( pRes, szClassTypeName );
}

bool GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID )
{
	return s_pMainDatabase->GetObjectsList( pRes, nClassTypeID );
}

string GetClassTypeName( const CDBID &dbid )
{
	return s_pMainDatabase->GetClassTypeName( dbid );
}

void AddDbObserver( IDbObserver *pObserver )
{
	s_pMainDatabase->AddDbObserver( pObserver );
}

bool CBasicDatabase::ReadResourceHeader( STypeObjectHeader *pHeader, const CDBID &dbid )
{
	CFileStream stream( pVFS, GetFileName(dbid) );
	if ( stream.IsOk() )
	{
		CObj<NLXML::IXmlSaxVisitor> pVisitor = new CObjectHeaderXmlSaxVisitor( pHeader );
		NLXML::ParseXML( pVisitor, &stream );
		return true;
	}
	return false;
}

bool CBasicDatabase::LoadIndex()
{
	ResetIndexChanged();
	vector<SFullTypeHeader> objectsIndex;
	CFileStream stream( GetVFS(), INDEX_FILE_NAME );
	if ( LoadIndexData( &objectsIndex, &stream ) )
	{
		// register all objects from index
		for ( vector<SFullTypeHeader>::iterator it = objectsIndex.begin(); it != objectsIndex.end(); ++it )
			RegisterObject( *it );
	}
	else
		return false;
	return true;
}

// check, have we changed DB objects?
bool HasChangedObjects()
{
	return s_pMainDatabase->HasChangedObjects();
}

// ************************************************************************************************************************ //
// **
// ** helpers and utils
// **
// **
// **
// ************************************************************************************************************************ //

void NormalizeDBID( CDBID *pRes, const CDBID &dbid )
{
	string szRes = dbid.ToString();
//	NStr::ToLowerASCII( &szRes, dbid.ToString() );
	NStr::ReplaceAllChars( &szRes, '\\', '/' );
	*pRes = szRes;
}

void CResourceHelper::SetDBID( CResource *pRes, const CDBID &_dbid )
{
	if ( pRes )
		pRes->dbid = _dbid;
}

void CResourceHelper::SetLoaded( CResource *pRes )
{
	if ( pRes )
		pRes->bLoaded = true;
}

void CResourceHelper::CallPostLoad( CResource *pRes, bool bInEditor )
{
	if ( pRes )
		pRes->PostLoad( bInEditor );
}

bool IsDBIDValid( const CDBID &dbid )
{
	const string szFullPath = GetFileName( dbid );
	list<string> parts;
	NFile::SplitPath( &parts, szFullPath );
	// check filename - must have .xdb at the end
	{
		const string &szFileName = parts.back();
		if ( szFileName.size() < 4 )
			return false;
		if ( NFile::ComparePathEq( szFileName.size() - 4, 4, szFileName, 0, 4, ".xdb" ) == false )
			return false;
	}
	parts.pop_back();
	// check each file path directory entry - must not contain .xdb at the end
	for ( list<string>::const_iterator it = parts.begin(); it != parts.end(); ++it )
	{
		if ( it->size() >= 4 && NFile::ComparePathEq( it->size() - 4, 4, *it, 0, 4, ".xdb" ) != false )
			return false;
	}
	//
	return true;
}

}

