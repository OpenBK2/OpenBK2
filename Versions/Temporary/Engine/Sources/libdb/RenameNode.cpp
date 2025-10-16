#include "stdafx.h"

#include "DBWatcherClient.h"
#include "libdb/EditorDb.h"
class CString;	// без такой ботвы #include "../../MapEditorLib/Interface_UserData.h" отказывается компилироваться!
#include "MapEditorLib/Interface_UserData.h"

#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include "System/VFSOperations.h"

#include <cstdint>

#include <fmt/format.h>

namespace NFolderManipulator
{

bool CheckedFileOperation( const std::string &szOperationDescription, bool bFileOperationResult )
{
	if ( !bFileOperationResult )
	{
		LPVOID lpMsgBuf;
		uint32_t dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			0,
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		std::string szErrorMessage = fmt::format( "rename failed: {}, {}", szOperationDescription, lpMsgBuf );
		LocalFree(lpMsgBuf);

		return false;
	}

	return true;
}

bool CheckedMove( const std::string &szFrom, const std::string &szTo )
{
	return CheckedFileOperation( fmt::format( "move {} -> {}", szFrom, szTo ), MoveFile( szFrom.c_str(), szTo.c_str() ) );
}

bool CheckedCopy( const std::string &szFrom, const std::string &szTo )
{
	return CheckedFileOperation( fmt::format( "copy {} -> {}", szFrom, szTo ), CopyFile( szFrom.c_str(), szTo.c_str(), false ) );
}

struct SReplaceEntry
{
	int nPos;
	int nOldStrSize;
	std::string szNewStr;

	SReplaceEntry() : nPos( 0 ), nOldStrSize( 0 ) { }
	SReplaceEntry( const int _nPos, const int _nOldStrSize, const std::string &_szNewStr )
		: nPos( _nPos ), nOldStrSize( _nOldStrSize ), szNewStr( _szNewStr ) { }
};

struct SReferencingObjInfo
{
	CDBID dbID;
	bool bNeedToReload;

	SReferencingObjInfo() : bNeedToReload( false ) { }
	SReferencingObjInfo( const std::string &szObjName )
		: dbID( szObjName ), bNeedToReload( NDb::IsFileRegistered( szObjName ) ) { }
	SReferencingObjInfo( const CDBID &_dbID )
		: dbID( _dbID ), bNeedToReload( NDb::IsFileRegistered( dbID.ToString() ) ) { }
};

class CXDBEnumeration
{
	std::vector<SReferencingObjInfo> *pReferencingObjs;
	std::vector<CDBID> *pChangedObjs;
	const std::string &szStorageDir;
	bool bOk;
public:
	CXDBEnumeration( const std::string &_szStorageDir, std::vector<SReferencingObjInfo> *_pReferencingObjs, std::vector<CDBID> *_pChangedObjs )
		: pReferencingObjs( _pReferencingObjs ), szStorageDir( _szStorageDir ), pChangedObjs( _pChangedObjs ), bOk( true ) { }

	const bool IsOk() const { return bOk; }

	void operator()( const NFile::CFileIterator &iter )
	{
		if ( !iter.IsDirectory() && bOk )
		{
			const std::string szName = iter.GetFullName().substr( szStorageDir.size() );
			pChangedObjs->push_back( CDBID( szName ) );

			std::vector<CDBID> refObjs;
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



static bool IsFolderName( const std::string &szName )
{
	return !szName.empty() && ( szName[szName.size() - 1] == '\\' || szName[szName.size() - 1] == '/' );
}

static bool ReplaceEntriesInFile( const std::string &szFileName, const std::string &szNewFileName, const std::vector<SReplaceEntry> &entries, const std::vector<char> &buffer, const std::string &szStorageDir )
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

	const std::string szNewTempFileName = szNewFileName + ".$$$";

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

static bool ChangeReference( const std::string &szFileName, const std::string &_szOldObjName, const std::string &_szNewObjName, const std::string &szStorageDir )
{
	std::string szOldObjName = "href=\"" + _szOldObjName;
	std::string szNewObjName = "href=\"" + _szNewObjName;

	replace( szOldObjName.begin(), szOldObjName.end(), '\\', '/' );
	replace( szNewObjName.begin(), szNewObjName.end(), '\\', '/' );

	std::vector<char> buffer;
	{
		CFileStream stream( NVFS::GetMainVFS(), szStorageDir + szFileName );
		if ( !stream.IsOk() || stream.GetSize() == 0 )
			return false;

		buffer.resize( stream.GetSize() );
		stream.Read( &(buffer[0]), stream.GetSize() );
	}

	std::vector<int> entries;
	NStr::FastSearch( &(buffer[0]), buffer.size(), szOldObjName, &entries, NStr::SASCIICharsComparer() );

	std::vector<SReplaceEntry> replaceEntries;
	replaceEntries.reserve( entries.size() );
	for ( int i = 0; i < entries.size(); ++i )
		replaceEntries.push_back( SReplaceEntry( entries[i], szOldObjName.size(), szNewObjName ) );

	return ReplaceEntriesInFile( szFileName, szFileName, replaceEntries, buffer, szStorageDir );
}

static bool ConvertToRelativeNameIfDownByDirs( const std::string &szSrcName, const std::string &szSearchName, std::string *pszRelativeName )
{
	std::list<std::string> searchPath;
	NFile::SplitPath( &searchPath, szSearchName );
	searchPath.pop_back();

	std::list<std::string> srcPath;
	NFile::SplitPath( &srcPath, szSrcName );

	std::list<std::string>::iterator searchIter = searchPath.begin();
	std::list<std::string>::iterator srcIter = srcPath.begin();

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
		const std::string szSeparator = pszRelativeName->empty() ? "" : "/";
		*pszRelativeName += szSeparator;
		*pszRelativeName += *srcIter;

		++srcIter;
	}
	return true;
}

static bool FixReferencingFile( const std::string &szFileName, const std::string &szName, const std::string &szNewName, const std::string &szStorageDir )
{
	std::string szNewRelativeName = szNewName;
	const bool bNewNameConverted = ConvertToRelativeNameIfDownByDirs( szNewName, szFileName, &szNewRelativeName );
	if ( !bNewNameConverted )
		szNewRelativeName = "/" + szNewRelativeName;

	if ( !ChangeReference( szFileName, "/" + szName, szNewRelativeName, szStorageDir ) )
		return false;

	std::string szRelativeName;
	const bool bNameConverted = ConvertToRelativeNameIfDownByDirs( szName, szFileName, &szRelativeName );
	if ( bNameConverted )
	{
		if ( !ChangeReference( szFileName, szRelativeName, szNewRelativeName, szStorageDir ) )
			return false;
	}

	return true;
}

static bool FixChangedFile( const std::string &szOldName, const std::string &szNewName, const std::string &szStorageDir )
{
	if ( NFile::GetFilePath( szOldName ) == NFile::GetFilePath( szNewName ) )
	{
		DeleteFile( (szStorageDir+szNewName).c_str() );
		return CheckedCopy( szStorageDir + szOldName, szStorageDir + szNewName );
	}

	std::vector<char> buffer;
	{
		CFileStream stream( NVFS::GetMainVFS(), szStorageDir + szOldName );
		if ( !stream.IsOk() || stream.GetSize() == 0 )
			return true;

		buffer.resize( stream.GetSize() );
		stream.Read( &(buffer[0]), stream.GetSize() );
	}

	const std::string szSearchStr = " href=\"";
	std::vector<int> entries;
	NStr::FastSearch( &(buffer[0]), buffer.size(), szSearchStr, &entries, NStr::SASCIICharsComparer() );

	std::string szOldPath = NFile::GetFilePath( szOldName );
	NStr::ReplaceAllChars( &szOldPath, '\\', '/' );
	std::vector<SReplaceEntry> replaceEntries;
	replaceEntries.reserve( entries.size() );
	for ( int i = 0; i < entries.size(); ++i )
	{
		if ( entries[i] + szSearchStr.size() >= buffer.size() )
			break;
		if ( buffer[entries[i] + szSearchStr.size()] == '/' )
			continue;

		std::string szRefFile = "";
		for ( int j = entries[i] + szSearchStr.size();j < buffer.size() && buffer[j] != '#' && buffer[j] != '\"'; ++j )
			szRefFile += buffer[j];

		NStr::TrimBoth( szRefFile );
		if ( szRefFile.empty() )
			continue;
		NStr::ReplaceAllChars( &szRefFile, '\\', '/' );

		const std::string szFullRefFile = szOldPath + szRefFile;
		std::string szRightName;
		const bool bRefChanged = ConvertToRelativeNameIfDownByDirs( szFullRefFile, szNewName, &szRightName );
		if ( !bRefChanged )
			szRightName = "/" + szFullRefFile;

		replaceEntries.push_back( SReplaceEntry( entries[i] + szSearchStr.size(), szRefFile.size(), szRightName ) );
	}

	return ReplaceEntriesInFile( szOldName, szNewName, replaceEntries, buffer, szStorageDir );
}

bool RenameNode2( const std::string &_szName, const std::string &_szNewName )
{
	std::vector<SReferencingObjInfo> referencingObjs;
	std::vector<CDBID> changedObjs;

	std::string szName, szNewName;
	NStr::ToLowerASCII( &szName, _szName );
	NStr::ToLowerASCII( &szNewName, _szNewName );

	const std::string szStorageDir = Singleton<IUserDataContainer>()->Get()->constUserData.szDataStorageFolder;

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
		std::vector<CDBID> refObjs;
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
		std::string szNewObjName = changedObjs[i].ToString();
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
		std::string szNewObjName = changedObjs[i].ToString();
		szNewObjName.replace( 0, szNewObjName.size(), szNewName );
		if ( changedObjs[i].ToString() != szNewObjName )
			DeleteFile( (szStorageDir + changedObjs[i].ToString()).c_str() );
	}

	for ( int i = 0; i < changedObjs.size(); ++i )
	{
		std::string szObjName = changedObjs[i].ToString();
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

bool RenameNode( const std::string &szOldName, const std::string &szNewName )
{
	bool bRes = true;
	if ( IsFolderName(szOldName) )
	{
		std::vector<std::string> filenames;
		NVFS::GetMainVFS()->GetAllFileNames( &filenames, szOldName );
		const std::string szXDB = ".xdb";
		for ( std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
		{
			if ( it->size() > 4 && NFile::ComparePathEq(it->size() - 4, 4, *it, 0, 4, szXDB) )
			{
				std::string szNewObjFileName = *it;
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

}

