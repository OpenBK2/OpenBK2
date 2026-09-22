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

#include "port/time.h"

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <system_error>
#include <fstream>
#include <cerrno>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/filesystem/operations.hpp>

static std::mutex g_WinVFSCriticalSection;

namespace NVFS
{

enum EStreamPath
{
	STREAM_PATH_ABSOLUTE = 0,
	STREAM_PATH_RELATIVE = 1
};

static CDataStream *OpenWinFileDirect( const std::string &szPath, bool bRead )
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

static CDataStream* OpenWinFile( const std::string &szPath, bool bRead )
{
	return OpenWinFileDirect( szPath, bRead );
}

// helper function - create/open win file and check for success
static CDataStream* OpenWinFileRW( const std::string &szPath )
{
	return OpenWinFile( szPath, false );
}

static void PreprocessPath( std::string *pResPath, EStreamPath *pStreamPathType, const std::string &_szPath, const std::string &szBasePath )
{
	// Normalize before classifying: legacy separators must mean the same path
	// for reading, writing and file stats, including absolute paths on Linux.
	std::string szPath = _szPath;
	NStr::ReplaceAllChars( &szPath, NFile::PATH_SEPARATOR == '/' ? '\\' : '/', NFile::PATH_SEPARATOR );
	if ( szBasePath.empty() )
	{
		*pStreamPathType = STREAM_PATH_ABSOLUTE;
		*pResPath = szPath;
	}
	else if ( NFile::ComparePathEq(0, szBasePath.size(), szBasePath, 0, (std::min)(szBasePath.size(), szPath.size()), szPath) != false )
	{
		*pStreamPathType = STREAM_PATH_RELATIVE;
		*pResPath = szPath.c_str() + szBasePath.size();
	}
	else if ( std::filesystem::u8path( szPath ).is_absolute() ||
		( szPath.size() > 2 && szPath[1] == ':' && NFile::IsFolderSeparator(szPath[2]) ) )
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

bool GetWinFileStats( struct SFileStats *pStats, const std::string &szPath )
{
	pStats->pszName = 0;
	pStats->nSize = 0;
	pStats->mtime = 0;
	// the error_code overloads report a missing file rather than throwing,
	// which is what the callers expect from a false return
	boost::system::error_code ec;
	const boost::uintmax_t nSize = boost::filesystem::file_size( szPath, ec );
	if ( ec )
		return false;
	const std::time_t mtime = boost::filesystem::last_write_time( szPath, ec );
	if ( ec )
		return false;
	pStats->nSize = int( nSize );
	pStats->mtime = PackFileTime( mtime );
	return true;
}

bool DoesWinFileExist( const std::string &szPath )
{
	return NFile::DoesFileExist( szPath );
}

// szPathName is how the caller asked for the file, which the map's folded key lets
// differ from how it is spelled on disk. Only the latter opens, so that is what
// these two use and the parameter goes unread.
CDataStream* CWinVFS::CWinFileEntry::OpenStream( const std::string &szPathName )
{
	return OpenWinFileDirect( szBasePath + szRealPath, true );
}

bool CWinVFS::CWinFileEntry::GetStats( SFileStats *pStats, const std::string &szPathName ) const
{
	return pStats == 0 ? false : GetWinFileStats( pStats, szBasePath + szRealPath );
}



CWinVFS::CWinVFS( const std::string &_szBasePath )
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
	std::string szDir = _szBasePath;
	std::string szExt;
	{
		// To the platform's separator, not always to a backslash. The split
		// below finds the last one, and off Windows a backslash is an ordinary
		// filename character, so converting to it turned the whole absolute
		// path into a single relative name and every lookup that concatenated
		// onto szBasePath afterwards missed.
		NStr::ReplaceAllChars( &szDir, NFile::PATH_SEPARATOR == '/' ? '\\' : '/', NFile::PATH_SEPARATOR );
		const int nPos = szDir.rfind( NFile::PATH_SEPARATOR );
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
	// The platform's own separator, and any separator counts as one already
	// being there. Forcing a backslash off Windows appended one to a path that
	// already ended in a slash, and every lookup below concatenates onto this.
	if ( !NFile::IsFolderSeparator( szDir[szDir.size() - 1] ) )
		szDir += NFile::PATH_SEPARATOR;
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
			if ( zip.GetFileAttribs( i ) & CZipFile::DOS_ATTR_DIRECTORY )
				continue;
			// check for non-zero file length
			if ( zip.GetFileLen( i ) <= 0 )
				continue;
			// add this entry
			const uint32_t dwCheckTime = zip.GetModDateTime( i );
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
	std::lock_guard csLock( g_WinVFSCriticalSection );
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
				pos->second = new CWinFileEntry( fileStats.mtime, szBasePath, szPath );
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
			CWinFileEntry *pEntry = new CWinFileEntry( fileStats.mtime, szBasePath, szPath );
			pEntry->SetChecked();
			streamEntriesMap[szPath] = pEntry;
			return pEntry;
		}
		// Nothing is spelled that way, but the data does not agree with the disk
		// about spelling and never had to: the shipped tree has UI/MainMenuMovie
		// while the database asks for UI/mainmenumovie, and on Windows the
		// filesystem settled it. Ask again for whatever is really there.
		//
		// Filed under the name that was asked for rather than the one that was
		// found, because that is the lookup that has to hit next time and the key
		// folds case anyway, so both spellings reach this one entry.
		NFile::CFilePath szRealPath;
		if ( NFile::ResolveDataPathCase( &szRealPath, szBasePath, szPath ) &&
			   GetWinFileStats( &fileStats, szBasePath + szRealPath ) != false )
		{
			CWinFileEntry *pEntry = new CWinFileEntry( fileStats.mtime, szBasePath, szRealPath );
			pEntry->SetChecked();
			streamEntriesMap[szPath] = pEntry;
			return pEntry;
		}
	}
	return 0;
}

CDataStream* CWinVFS::OpenFileDirect( const std::string &_szPath )
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
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

CDataStream* CWinVFS::OpenFile( const std::string &szPath )
{
	return OpenFileDirect( szPath );
}

static uint32_t dwLastProfilerSegment = 0;
static float fBigTimeForLoad = 0.0f;
static int nCallsNumber = 0;

class CProfiler
{
	const std::string szPath;
	const uint32_t dwStartTime;
public:
	// szPath( szPath ) initialised the member from itself, reading an
	// uninitialised std::string's length and asking the allocator for whatever
	// that garbage happened to be. Undefined behaviour since it was written;
	// MSVC left values benign enough to survive it, and GCC at -O0 handed over
	// a two terabyte length, which mimalloc then tried to zero.
	CProfiler( const std::string &_szPath ) : szPath( _szPath ), dwStartTime( GetCurrentTimeMilliseconds() ) { }
	~CProfiler()
	{
		const float fLoadTime = float(GetCurrentTimeMilliseconds() - dwStartTime)/1000.0f;
		//		if ( fLoadTime > 0.1 )
		//			DbgTrc( "load: %s loaded in %f sec", szPath.c_str(), fLoadTime );

		fBigTimeForLoad += fLoadTime;
		++nCallsNumber;
	}
};

void VFSSegmentProfiler()
{
	uint32_t dwTime = GetCurrentTimeMilliseconds();
	if ( dwTime - dwLastProfilerSegment > 1000 )
	{
		if ( fBigTimeForLoad != 0.0f )
			DbgTrc( "DoesFileExist: %f sec in segment, %d calls", fBigTimeForLoad, nCallsNumber );

		dwLastProfilerSegment = dwTime;
		fBigTimeForLoad = 0.0f;
		nCallsNumber = 0;
	}
}

bool CWinVFS::DoesFileExist( const std::string &_szPath )
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

bool CWinVFS::GetFileStats( SFileStats *pStats, const std::string &_szPath )
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

void CWinVFS::GetAllFileNames( std::vector<std::string> *pFileNames, const std::string &rszFolder )
{
	std::lock_guard csLock( g_WinVFSCriticalSection );
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


CWinFileCreator::CWinFileCreator( const std::string &_szBasePath )
: szBasePath( _szBasePath )
{
	// The VFS reader accepts old Windows separators on every platform. Its
	// writer must use the same interpretation, including the storage root.
	NStr::ReplaceAllChars( &szBasePath, NFile::PATH_SEPARATOR == '/' ? '\\' : '/', NFile::PATH_SEPARATOR );
	NFile::AppendSlash( &szBasePath, NFile::PATH_SEPARATOR );
}

std::string CWinFileCreator::GetFullPath( const std::string &_szPath ) const
{
	NFile::CFilePath szPath;
	EStreamPath ePath;
	PreprocessPath( &szPath, &ePath, _szPath, szBasePath );
	std::string fullPath = ePath == STREAM_PATH_RELATIVE ? szBasePath + szPath : szPath;
	const auto path = std::filesystem::u8path( fullPath );
	std::string realPath;
	// Resolve existing parents even for a new file, avoiding a second "custom"
	// beside "Custom" on Linux. Absolute paths must not be appended to the root.
	if ( NFile::ResolveDataPathCase( &realPath, path.root_path().u8string(), path.relative_path().u8string(), true ) )
		fullPath = path.root_path().u8string() + realPath;
	return fullPath;
}

static bool IsForeignDrivePath( const std::string &path )
{
#if BOOST_OS_WINDOWS
	return false;
#else
	// A saved C:\... path is not a relative Linux directory called "C:".
	return path.size() > 1 && path[1] == ':';
#endif
}

CDataStream* CWinFileCreator::CreateFile( const std::string &_szPath )
{
	const std::string szFullFilePath = GetFullPath( _szPath );
	if ( IsForeignDrivePath( szFullFilePath ) || !NFile::CreatePath( NFile::GetFilePath(szFullFilePath) ) )
		return nullptr;
	//
	CDataStream *pRes = OpenWinFileDirect( szFullFilePath, false );
	if ( !pRes )
		return 0;
	pRes->Trunc();
	return pRes;
}

bool CWinFileCreator::RemoveFile( const std::string &_szPath )
{
	const std::string szFullFilePath = GetFullPath( _szPath );
	if ( IsForeignDrivePath( szFullFilePath ) )
		return false;
	//
	// remove reports false both when the file was not there and when it could not be
	// removed, which is what DeleteFile returning FALSE meant to this caller.
	std::error_code ec;
	return std::filesystem::remove( szFullFilePath, ec );
}

std::string GetWritePath( IFileCreator *pCreator, const std::string &path )
{
	const auto *native = dynamic_cast<CWinFileCreator*>( pCreator );
	return native ? native->GetFullPath( path ) : path;
}

bool WriteFile( IFileCreator *pCreator, const std::string &path, const CDataStream &data, std::string *pError )
{
	const std::string destination = GetWritePath( pCreator, path );
	const auto Fail = [&]( const std::string &reason ) {
		if ( pError ) *pError = destination + "\n" + reason;
		return false;
	};
	if ( pError ) pError->clear();
	if ( !pCreator || !data.IsOk() )
		return Fail( "No writable storage or serialization failed." );
	if ( IsForeignDrivePath( destination ) )
		return Fail( "This Windows drive path is unavailable on this platform. Reopen the mod from its local folder." );
	if ( !dynamic_cast<CWinFileCreator*>( pCreator ) )
	{
		CFileStream stream( pCreator, path );
		if ( !stream.IsOk() ) return Fail( "Cannot open the output stream." );
		stream.Write( data.GetBuffer(), data.GetSize() );
		stream.Flush();
		return stream.IsOk() || Fail( "Writing the output stream failed." );
	}
	// Serialize before touching the original, then replace it only after the
	// complete file has been written and closed successfully.
	std::filesystem::path temporary;
	try
	{
		std::error_code ec;
		auto target = std::filesystem::u8path( destination );
		// Preserve the old writer's behaviour for symlinked data files.
		if ( std::filesystem::is_symlink( target, ec ) ) target = std::filesystem::canonical( target );
		if ( std::filesystem::exists( target ) )
		{
			// Replacement must still respect a read-only destination, just as
			// opening the original file for writing did before atomic saves.
			std::fstream writable;
			writable.exceptions( std::ios::badbit | std::ios::failbit );
			writable.open( target, std::ios::binary | std::ios::in | std::ios::out );
			writable.close();
		}
		if ( !target.parent_path().empty() ) std::filesystem::create_directories( target.parent_path() );
		temporary = target;
		temporary += "." + boost::uuids::to_string( boost::uuids::random_generator()() ) + ".tmp";
		std::ofstream output;
		output.exceptions( std::ios::badbit | std::ios::failbit );
		output.open( temporary, std::ios::binary | std::ios::trunc );
		if ( data.GetSize() ) output.write( reinterpret_cast<const char*>( data.GetBuffer() ), data.GetSize() );
		output.flush();
		output.close();
		const auto status = std::filesystem::status( target, ec );
		if ( !ec && std::filesystem::exists( status ) ) std::filesystem::permissions( temporary, status.permissions() );
		std::filesystem::rename( temporary, target );
		return true;
	}
	catch ( const std::exception &error )
	{
		const int code = errno;
		const std::string reason = std::string( error.what() ) + ( code ? "\n" + std::generic_category().message( code ) : "" );
		std::error_code ignored;
		if ( !temporary.empty() ) std::filesystem::remove( temporary, ignored );
		return Fail( reason );
	}
}

IVFS* CreateWinVFS( const std::string &szBasePath )
{
	return new NVFS::CWinVFS( szBasePath );
}

IFileCreator* CreateWinFileCreator( const std::string &szBasePath )
{
	return new CWinFileCreator( szBasePath );
}

}

