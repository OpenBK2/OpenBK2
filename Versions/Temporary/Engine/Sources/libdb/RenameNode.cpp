#include "stdafx.h"

#include "DBWatcherClient.h"
#include "../libdb/EditorDb.h"
class CString;	// без такой ботвы #include "../../MapEditorLib/Interface_UserData.h" отказывается компилироваться!
#include "../MapEditorLib/Interface_UserData.h" 

#include "../System/FilePath.h"
#include "../System/FileUtils.h"
#include "../Misc/StrProc.h"
#include "../System/VFSOperations.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NFolderManipulator
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CheckedFileOperation( const string &szOperationDescription, bool bFileOperationResult )
{
	if ( !bFileOperationResult )
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError(); 

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			0,
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		string szErrorMessage = StrFmt( "rename failed: %s, %s", szOperationDescription.c_str(), lpMsgBuf );
		LocalFree(lpMsgBuf);

		return false;
	}

	return true;
}

bool CheckedMove( const string &szFrom, const string &szTo )
{
	return CheckedFileOperation( StrFmt( "move %s -> %s", szFrom.c_str(), szTo.c_str() ), MoveFile( szFrom.c_str(), szTo.c_str() ) );
}

bool CheckedCopy( const string &szFrom, const string &szTo )
{
	return CheckedFileOperation( StrFmt( "copy %s -> %s", szFrom.c_str(), szTo.c_str() ), CopyFile( szFrom.c_str(), szTo.c_str(), false ) );
}

struct SReplaceEntry
{
	int nPos;
	int nOldStrSize;
	string szNewStr;

	SReplaceEntry() : nPos( 0 ), nOldStrSize( 0 ) { }
	SReplaceEntry( const int _nPos, const int _nOldStrSize, const string &_szNewStr )
		: nPos( _nPos ), nOldStrSize( _nOldStrSize ), szNewStr( _szNewStr ) { }
};

struct SReferencingObjInfo
{
	CDBID dbID;
	bool bNeedToReload;

	SReferencingObjInfo() : bNeedToReload( false ) { }
	SReferencingObjInfo( const string &szObjName )
		: dbID( szObjName ), bNeedToReload( NDb::IsFileRegistered( szObjName ) ) { }
	SReferencingObjInfo( const CDBID &_dbID )
		: dbID( _dbID ), bNeedToReload( NDb::IsFileRegistered( dbID.ToString() ) ) { }
};

class CXDBEnumeration
{
	vector<SReferencingObjInfo> *pReferencingObjs;
	vector<CDBID> *pChangedObjs;
	const string &szStorageDir;
	bool bOk;
public:
	CXDBEnumeration( const string &_szStorageDir, vector<SReferencingObjInfo> *_pReferencingObjs, vector<CDBID> *_pChangedObjs )
		: pReferencingObjs( _pReferencingObjs ), szStorageDir( _szStorageDir ), pChangedObjs( _pChangedObjs ), bOk( true ) { }

	const bool IsOk() const { return bOk; }

	void operator()( const NFile::CFileIterator &iter )
	{
		if ( !iter.IsDirectory() && bOk )
		{
			const string szName = iter.GetFullName().substr( szStorageDir.size() );
			pChangedObjs->push_back( CDBID( szName ) );

			vector<CDBID> refObjs;
			NDBWatcherClient::IDBWatcherClient::EResult eClientResult;
			while ( ( eClientResult = Singleton<NDBWatcherClient::IDBWatcherClient>()->GetReferencingObjects( szName, &refObjs ) )
				== NDBWatcherClient::IDBWatcherClient::EResult::SERVICE_NOT_READY );
			const bool bResult = ( eClientResult == NDBWatcherClient::IDBWatcherClient::EResult::COMPLETE );
			if ( !bResult )
				bOk = false;
			else
				pReferencingObjs->insert( pReferencingObjs->end(), refObjs.begin(), refObjs.end() );
		}
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsFolderName( const string &szName )
{
	return !szName.empty() && ( szName[szName.size() - 1] == '\\' || szName[szName.size() - 1] == '/' );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool ReplaceEntriesInFile( const string &szFileName, const string &szNewFileName, const vector<SReplaceEntry> &entries, const vector<char> &buffer, const string &szStorageDir )
{
	NFile::CreatePath( NFile::GetFilePath(szStorageDir + szNewFileName) );
	if ( entries.empty() )
	{
		if ( szFileName != szNewFileName )
		{
			DeleteFile( (szStorageDir + szNewFileName).c_str() );
			return CheckedCopy( szStorageDir + szFileName, szStorageDir + szNewFileName );
		}
		return true;
	}

	const string szNewTempFileName = szNewFileName + ".$$$";

	{
		CFileStream newStream( NVFS::GetMainFileCreator(), szNewTempFileName );
		if ( !newStream.IsOk() )
			return false;

		int nLastPos = 0;
		for ( int i = 0; i < entries.size(); ++i )
		{
			newStream.Write( &(buffer[nLastPos]), entries[i].nPos - nLastPos );
			if ( !entries[i].szNewStr.empty() )
				newStream.Write( &(entries[i].szNewStr[0]), entries[i].szNewStr.size() );

			nLastPos = entries[i].nPos + entries[i].nOldStrSize;
		}
		newStream.Write( &(buffer[nLastPos]), buffer.size() - nLastPos );
	}

	DeleteFile( (szStorageDir + szNewFileName).c_str() );

	return CheckedMove( szStorageDir + szNewTempFileName, szStorageDir + szNewFileName );
}

static bool ChangeReference( const string &szFileName, const string &_szOldObjName, const string &_szNewObjName, const string &szStorageDir )
{
	string szOldObjName = "href=\"" + _szOldObjName;
	string szNewObjName = "href=\"" + _szNewObjName;

	replace( szOldObjName.begin(), szOldObjName.end(), '\\', '/' );
	replace( szNewObjName.begin(), szNewObjName.end(), '\\', '/' );

	vector<char> buffer;
	{
		CFileStream stream( NVFS::GetMainVFS(), szStorageDir + szFileName );
		if ( !stream.IsOk() || stream.GetSize() == 0 )
			return false;

		buffer.resize( stream.GetSize() );
		stream.Read( &(buffer[0]), stream.GetSize() );
	}

	vector<int> entries;
	NStr::FastSearch( &(buffer[0]), buffer.size(), szOldObjName, &entries, NStr::SASCIICharsComparer() );

	vector<SReplaceEntry> replaceEntries;
	replaceEntries.reserve( entries.size() );
	for ( int i = 0; i < entries.size(); ++i )
		replaceEntries.push_back( SReplaceEntry( entries[i], szOldObjName.size(), szNewObjName ) );

	return ReplaceEntriesInFile( szFileName, szFileName, replaceEntries, buffer, szStorageDir );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool ConvertToRelativeNameIfDownByDirs( const string &szSrcName, const string &szSearchName, string *pszRelativeName )
{
	list<string> searchPath;
	NFile::SplitPath( &searchPath, szSearchName );
	searchPath.pop_back();

	list<string> srcPath;
	NFile::SplitPath( &srcPath, szSrcName );

	list<string>::iterator searchIter = searchPath.begin();
	list<string>::iterator srcIter = srcPath.begin();

	while ( searchIter != searchPath.end() && *searchIter == *srcIter )
	{
		++searchIter;
		++srcIter;
	}

	if ( searchIter != searchPath.end() )
		return false;

	pszRelativeName->clear();
	while ( srcIter != srcPath.end() )
	{
		const string szSeparator = pszRelativeName->empty() ? "" : "/";
		*pszRelativeName += szSeparator;
		*pszRelativeName += *srcIter;

		++srcIter;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool FixReferencingFile( const string &szFileName, const string &szName, const string &szNewName, const string &szStorageDir )
{
	string szNewRelativeName = szNewName;
	const bool bNewNameConverted = ConvertToRelativeNameIfDownByDirs( szNewName, szFileName, &szNewRelativeName );
	if ( !bNewNameConverted )
		szNewRelativeName = "/" + szNewRelativeName;

	if ( !ChangeReference( szFileName, "/" + szName, szNewRelativeName, szStorageDir ) )
		return false;

	string szRelativeName;
	const bool bNameConverted = ConvertToRelativeNameIfDownByDirs( szName, szFileName, &szRelativeName );
	if ( bNameConverted )
	{
		if ( !ChangeReference( szFileName, szRelativeName, szNewRelativeName, szStorageDir ) )
			return false;
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool FixChangedFile( const string &szOldName, const string &szNewName, const string &szStorageDir )
{
	if ( NFile::GetFilePath( szOldName ) == NFile::GetFilePath( szNewName ) )
	{
		DeleteFile( (szStorageDir+szNewName).c_str() );
		return CheckedCopy( szStorageDir + szOldName, szStorageDir + szNewName );
	}

	vector<char> buffer;
	{
		CFileStream stream( NVFS::GetMainVFS(), szStorageDir + szOldName );
		if ( !stream.IsOk() || stream.GetSize() == 0 )
			return true;

		buffer.resize( stream.GetSize() );
		stream.Read( &(buffer[0]), stream.GetSize() );
	}

	const string szSearchStr = " href=\"";
	vector<int> entries;
	NStr::FastSearch( &(buffer[0]), buffer.size(), szSearchStr, &entries, NStr::SASCIICharsComparer() );

	string szOldPath = NFile::GetFilePath( szOldName );
	NStr::ReplaceAllChars( &szOldPath, '\\', '/' );
	vector<SReplaceEntry> replaceEntries;
	replaceEntries.reserve( entries.size() );
	for ( int i = 0; i < entries.size(); ++i )
	{
		if ( entries[i] + szSearchStr.size() >= buffer.size() )
			break;
		if ( buffer[entries[i] + szSearchStr.size()] == '/' )
			continue;

		string szRefFile = "";
		for ( int j = entries[i] + szSearchStr.size();j < buffer.size() && buffer[j] != '#' && buffer[j] != '\"'; ++j )
			szRefFile += buffer[j];

		NStr::TrimBoth( szRefFile );
		if ( szRefFile.empty() )
			continue;
		NStr::ReplaceAllChars( &szRefFile, '\\', '/' );

		const string szFullRefFile = szOldPath + szRefFile;
		string szRightName;
		const bool bRefChanged = ConvertToRelativeNameIfDownByDirs( szFullRefFile, szNewName, &szRightName );
		if ( !bRefChanged )
			szRightName = "/" + szFullRefFile;

		replaceEntries.push_back( SReplaceEntry( entries[i] + szSearchStr.size(), szRefFile.size(), szRightName ) );
	}

	return ReplaceEntriesInFile( szOldName, szNewName, replaceEntries, buffer, szStorageDir );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RenameNode2( const string &_szName, const string &_szNewName )
{
	vector<SReferencingObjInfo> referencingObjs;
	vector<CDBID> changedObjs;

	string szName, szNewName;
	NStr::ToLowerASCII( &szName, _szName );
	NStr::ToLowerASCII( &szNewName, _szNewName );

	const string szStorageDir = Singleton<IUserDataContainer>()->Get()->constUserData.szDataStorageFolder;

	DbgTrc( "collecting files..." );
	if ( IsFolderName( szName ) )
	{
		CXDBEnumeration xdbEnumeration( szStorageDir, &referencingObjs, &changedObjs );
		NFile::EnumerateFiles( szStorageDir + szName, "*.xdb", xdbEnumeration, true );
		if ( !xdbEnumeration.IsOk() )
			return false;
	}
	else
	{
		vector<CDBID> refObjs;
		while ( Singleton<NDBWatcherClient::IDBWatcherClient>()->GetReferencingObjects( szName, &refObjs ) == 
			NDBWatcherClient::IDBWatcherClient::EResult::SERVICE_NOT_READY );
		referencingObjs.insert( referencingObjs.end(), refObjs.begin(), refObjs.end() );
		changedObjs.push_back( szName );
	}

	for ( int i = 0; i < changedObjs.size(); ++i )
		NDb::RemoveObject( changedObjs[i] );
	for ( int i = 0; i < referencingObjs.size(); ++i )
		NDb::RemoveObject( referencingObjs[i].dbID );

	DbgTrc( "changed: %d files", changedObjs.size() );
	DbgTrc( "referencing files: %d", referencingObjs.size() );
	for ( int i = 0; i < changedObjs.size(); ++i )
	{
		string szNewObjName = changedObjs[i].ToString();
		szNewObjName.replace( 0, szName.size(), szNewName );

		if ( !FixChangedFile( changedObjs[i].ToString(), szNewObjName, szStorageDir ) )
			return false;
	}
	for ( int i = 0; i < referencingObjs.size(); ++i )
	{
		if ( !FixReferencingFile( referencingObjs[i].dbID.ToString(), szName, szNewName, szStorageDir ) )
			return false;
	}

	for ( int i = 0; i < changedObjs.size(); ++i )
	{
		string szNewObjName = changedObjs[i].ToString();
		szNewObjName.replace( 0, szNewObjName.size(), szNewName );
		if ( changedObjs[i].ToString() != szNewObjName )
			DeleteFile( (szStorageDir + changedObjs[i].ToString()).c_str() );
	}

	for ( int i = 0; i < changedObjs.size(); ++i )
	{
		string szObjName = changedObjs[i].ToString();
		szObjName.replace( 0, szName.size(), szNewName );

		NDb::RegisterResourceFile( szObjName );
	}
	for ( int i = 0; i < referencingObjs.size(); ++i )
	{
		if ( referencingObjs[i].bNeedToReload )
			NDb::RegisterResourceFile( referencingObjs[i].dbID.ToString() );
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RenameNode( const string &szOldName, const string &szNewName )
{
	bool bRes = true;
	if ( IsFolderName(szOldName) )
	{
		vector<string> filenames;
		NVFS::GetMainVFS()->GetAllFileNames( &filenames, szOldName );
		const string szXDB = ".xdb";
		for ( vector<string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
		{
			if ( it->size() > 4 && NFile::ComparePathEq(it->size() - 4, 4, *it, 0, 4, szXDB) )
			{
				string szNewObjFileName = *it;
				szNewObjFileName.replace( 0, szOldName.size(), szNewName );
				bRes = bRes && NDb::RenameObject( CDBID(*it), CDBID(szNewObjFileName) );
			}
		}
	}
	else
		bRes = bRes && NDb::RenameObject( CDBID(szOldName), CDBID(szNewName) );
	//
	return bRes;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
