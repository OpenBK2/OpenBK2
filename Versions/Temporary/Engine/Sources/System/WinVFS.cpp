#include "stdafx.h"
#include "FilePath.h"
#include "FileTime.h"
#include "FileUtils.h"
#include "VFSOperations.h"
#include "WinVFS.h"
#include "WinVFS.hpp"
#include "ZipArchieve.h"
#include "Misc/StrProc.h"
#include "Misc/Win32Helper.h"

static NWin32Helper::CCriticalSection g_WinVFSCriticalSection;
namespace NVFS
{

enum EStreamPath
{
	STREAM_PATH_ABSOLUTE = 0,
	STREAM_PATH_RELATIVE = 1
};

static CDataStream *OpenWinFileDirect( const string &szPath, bool bRead )
{
	CMemoryMappedFile *p;
	if ( bRead )
		p = new CMemoryMappedFile( szPath.c_str(), STREAM_ACCESS_READ );
	else
		p = new CMemoryMappedFile( szPath.c_str(), STREAM_ACCESS_READ_WRITE );
	if ( !p->IsOk() )
	{
		delete p;
		return 0;
	}
	return p;
}

static CDataStream* OpenWinFile( const string &szPath, bool bRead )
{
	return OpenWinFileDirect( szPath, bRead );
}

// helper function - create/open win file and check for success
static CDataStream* OpenWinFileRW( const string &szPath )
{
	return OpenWinFile( szPath, false );
}

static void PreprocessPath( string *pResPath, EStreamPath *pStreamPathType, const string &szPath, const string &szBasePath )
{
	if ( szBasePath.empty() )
	{
		*pStreamPathType = STREAM_PATH_ABSOLUTE;
		*pResPath = szPath;
	}
	else if ( NFile::ComparePathEq(0, szBasePath.size(), szBasePath, 0, Min(szBasePath.size(), szPath.size()), szPath) != false )
	{
		*pStreamPathType = STREAM_PATH_RELATIVE;
		*pResPath = szPath.c_str() + szBasePath.size();
	}
	else if ( szPath.size() > 2 && szPath[1] == ':' && NFile::IsFolderSeparator(szPath[2]) )
	{
		*pStreamPathType = STREAM_PATH_ABSOLUTE;
		*pResPath = szPath;
	}
	else
	{
		*pStreamPathType = STREAM_PATH_RELATIVE;
		*pResPath = szPath;
	}
}

bool GetWinFileStats( struct SFileStats *pStats, const string &szPath )
{
	pStats->pszName = 0;
	pStats->nSize = 0;
	pStats->atime = 0;
	pStats->ctime = 0;
	pStats->mtime = 0;
	HANDLE hFile = CreateFile( szPath.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	BY_HANDLE_FILE_INFORMATION info;
	bool bRes = ::GetFileInformationByHandle( hFile, &info );
	CloseHandle( hFile );
	if ( !bRes )
		return false;
	pStats->nSize = info.nFileSizeLow;
	pStats->atime = FILETIMEToWin32DateTime( info.ftLastAccessTime );
	pStats->ctime = FILETIMEToWin32DateTime( info.ftCreationTime );
	pStats->mtime = FILETIMEToWin32DateTime( info.ftLastWriteTime );
	pStats->pszName = 0;
	return true;
}

bool DoesWinFileExist( const string &szPath )
{
	return NFile::DoesFileExist( szPath );
}

CDataStream* CWinVFS::CWinFileEntry::OpenStream( const string &szPathName )
{
	return OpenWinFileDirect( szBasePath + szPathName, true );
}

bool CWinVFS::CWinFileEntry::GetStats( SFileStats *pStats, const string &szPathName ) const
{
	return pStats == 0 ? false : GetWinFileStats( pStats, szBasePath + szPathName );
}



CWinVFS::CWinVFS( const string &_szBasePath )
: bAllWinFilesChecked( false ), bArchiveOnly( false )
{
	if ( _szBasePath.empty() )
	{
		szBasePath = _szBasePath;
		return;
	}
	//
	bArchiveOnly = NFile::DoesFileExist( _szBasePath );
	//
	string szDir = _szBasePath;
	string szExt;
	{
		NStr::ReplaceAllChars( &szDir, '/', '\\' );
		const int nPos = szDir.rfind( '\\' );
		if ( nPos != szDir.size() - 1 )
		{
			szExt = szDir.substr( nPos + 1 );
			if ( !szExt.empty() )
				szDir = szDir.substr( 0, nPos + 1 );
			else
				szExt = "*.pak";
		}
		else
			szExt = "*.pak";
	}
	//
	if ( szDir[szDir.size() - 1] != '\\' )
		szDir += '\\';
	//
	szBasePath = szDir;
	// enumerate all ZIP files and retrieve packed files from them
	for ( NFile::CFileIterator it( (szDir + szExt).c_str() ); !it.IsEnd(); ++it )
	{
		if ( it.IsDirectory() ) 
			continue;
		// open ZIP file
		CObj<CZipFile> pArch = new CZipFile( it.GetFullName().c_str() );
		if ( !pArch->IsOk() )
			continue;
		zipFiles.push_back( pArch );
		// add this ZIP file to the common list of opened ZIP files
		CZipFile &zip = *pArch;
		// enumerate all file names in the ZIP file and build structure
		for ( int i = 0; i < zip.GetNumFiles(); ++i )
		{
			// check for non-directory
			if ( zip.GetFileAttribs( i ) & FILE_ATTRIBUTE_DIRECTORY ) 
				continue;
			// check for non-zero file length
			if ( zip.GetFileLen( i ) <= 0 )
				continue;
			// add this entry
			const DWORD dwCheckTime = zip.GetModDateTime( i );
			NFile::CFilePath szFileName;
			zip.GetFileName( i, &szFileName );
			NStr::ReplaceAllChars( &szFileName, '/', '\\' );
			CStreamEntriesMap::iterator pos = streamEntriesMap.find( szFileName );
			if ( pos == streamEntriesMap.end() ) 
				streamEntriesMap[szFileName] = new CZipFileEntry( dwCheckTime, zip, i );
			else
			{
				if ( pos->second->GetCheckTime() < dwCheckTime )
				{
					delete pos->second;
					pos->second = new CZipFileEntry( dwCheckTime, zip, i );
				}
			}
		}
	}
}

CWinVFS::~CWinVFS()
{
	for ( CStreamEntriesMap::iterator iter = streamEntriesMap.begin(); iter != streamEntriesMap.end(); ++iter )
	{
		CFileEntry *pFileEntry = iter->second;
		delete pFileEntry;
	}
}

CWinVFS::CFileEntry *CWinVFS::UpdateFileEntry( const NFile::CFilePath &szPath )
{
	NWin32Helper::CCriticalSectionLock csLock( g_WinVFSCriticalSection );
	// first, check for registered file
	CStreamEntriesMap::iterator pos = streamEntriesMap.find( szPath );
	if ( pos != streamEntriesMap.end() ) 
	{
		// check for win-file if this entry still not checked
		if ( !pos->second->IsChecked() )
		{
			SFileStats fileStats;
			if ( GetWinFileStats(&fileStats, szBasePath + szPath) != false && pos->second->GetCheckTime() < fileStats.mtime )
			{
				delete pos->second;
				pos->second = new CWinFileEntry( fileStats.mtime, szBasePath );
			}
		}
		pos->second->SetChecked();
		return pos->second;
	}
	else
	{
		// check for unregistered win-file
		SFileStats fileStats;
		if ( GetWinFileStats(&fileStats, szBasePath + szPath) != false )
		{
			CWinFileEntry *pEntry = new CWinFileEntry( fileStats.mtime, szBasePath );
			pEntry->SetChecked();
			streamEntriesMap[szPath] = pEntry;
			return pEntry;
		}
	}
	return 0;
}

CDataStream* CWinVFS::OpenFileDirect( const string &_szPath )
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	//
	NStr::ReplaceAllChars( &szPath, '/', '\\' );
	//
	if ( ePath == STREAM_PATH_RELATIVE ) 
	{
		if ( CFileEntry *pEntry = UpdateFileEntry(szPath) )
			return pEntry->OpenStream( szPath );
	}
	else
		return OpenWinFileDirect( szPath, true );
	//
	return 0;
}

CDataStream* CWinVFS::OpenFile( const string &szPath )
{
	return OpenFileDirect( szPath );
}

static DWORD dwLastProfilerSegment = 0;
static float fBigTimeForLoad = 0.0f;
static int nCallsNumber = 0;

class CProfiler
{
	const string szPath;
	const DWORD dwStartTime;
public:
	CProfiler( const string &_szPath ) : szPath( szPath ), dwStartTime( GetTickCount() ) { }
	~CProfiler()
	{
		const float fLoadTime = float(GetTickCount() - dwStartTime)/1000.0f;
		//		if ( fLoadTime > 0.1 )
		//			DbgTrc( "load: %s loaded in %f sec", szPath.c_str(), fLoadTime );

		fBigTimeForLoad += fLoadTime;
		++nCallsNumber;
	}
};

void VFSSegmentProfiler()
{
	DWORD dwTime = GetTickCount();
	if ( dwTime - dwLastProfilerSegment > 1000 )
	{
		if ( fBigTimeForLoad != 0.0f )
			DbgTrc( "DoesFileExist: %f sec in segment, %d calls", fBigTimeForLoad, nCallsNumber );

		dwLastProfilerSegment = dwTime;
		fBigTimeForLoad = 0.0f;
		nCallsNumber = 0;
	}
}

bool CWinVFS::DoesFileExist( const string &_szPath )
{
	CProfiler profiler( _szPath );

	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	//
	if ( ePath == STREAM_PATH_RELATIVE )
		return UpdateFileEntry( szPath ) != 0;
	else
		return NFile::DoesFileExist( szPath );
}

bool CWinVFS::GetFileStats( SFileStats *pStats, const string &_szPath )
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	//
	if ( ePath == STREAM_PATH_RELATIVE )
	{
		if ( CFileEntry *pEntry = UpdateFileEntry( szPath ) )
			return pEntry->GetStats( pStats, szPath );
	}
	else
		return GetWinFileStats( pStats, szPath );
	//
	return false;
}

void CWinVFS::GetAllFileNames( vector<string> *pFileNames, const string &rszFolder )
{
	NWin32Helper::CCriticalSectionLock csLock( g_WinVFSCriticalSection );
	if ( !bArchiveOnly )
	{
		if ( bAllWinFilesChecked == false )
		{
			// enumerate non-zip files and add/replace it in accordance with its modification time
			CWinFileAdder adder( szBasePath, this );
			NFile::EnumerateFiles( szBasePath + rszFolder, "*.*", adder, true );
			// all files checked only then getallfiles called with empty start folder name!
			if ( rszFolder.empty() )
				bAllWinFilesChecked = true;
		}
	}
	//
	NFile::CFilePath folderPath = rszFolder;
	pFileNames->clear();
	pFileNames->reserve( streamEntriesMap.size() );
	for ( CStreamEntriesMap::const_iterator it = streamEntriesMap.begin(); it != streamEntriesMap.end(); ++it )
	{
		if ( folderPath.empty() )
			pFileNames->push_back( it->first );
		else if ( it->first.size() > folderPath.size() &&
			        NFile::ComparePathEq(0, folderPath.size(), folderPath, 0, folderPath.size(), it->first) != false )
		{
			pFileNames->push_back( it->first );
		}
	}
	sort( pFileNames->begin(), pFileNames->end() );
}


CWinFileCreator::CWinFileCreator( const string &_szBasePath )
: szBasePath( _szBasePath )
{
}

CDataStream* CWinFileCreator::CreateFile( const string &_szPath )
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	//
	const string szFullFilePath = ePath == STREAM_PATH_RELATIVE ? szBasePath + szPath : szPath;
	NFile::CreatePath( NFile::GetFilePath(szFullFilePath) );
	//
	CDataStream *pRes = OpenWinFileDirect( szFullFilePath, false );
	if ( !pRes )
		return 0;
	pRes->Trunc();
	return pRes;
}

bool CWinFileCreator::RemoveFile( const string &_szPath )
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	const string szFullFilePath = ePath == STREAM_PATH_RELATIVE ? szBasePath + szPath : szPath;
	//
	return ::DeleteFile( szFullFilePath.c_str() ) != FALSE;
}

IVFS* CreateWinVFS( const string &szBasePath )
{
	return new NVFS::CWinVFS( szBasePath );
}

IFileCreator* CreateWinFileCreator( const string &szBasePath )
{
	return new CWinFileCreator( szBasePath );
}

}

