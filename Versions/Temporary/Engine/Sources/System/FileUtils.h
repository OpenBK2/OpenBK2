#pragma once

#include "System_export.h"

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>

#include <boost/filesystem/operations.hpp>

namespace NFile
{

// One directory, filtered by a FindFirstFile style mask. The mask names the
// directory and the pattern together, "dir/*.xdb", as it did when this wrapped
// FindFirstFile; the directory is resolved to an absolute path once, so every name
// handed back is absolute.
//
// A directory that cannot be opened, which callers do pass, leaves the iterator at
// its end rather than throwing, matching the INVALID_HANDLE_VALUE this used to get.
class SYSTEM_EXPORT CFileIterator
{
	std::filesystem::directory_iterator it;	// default constructed is the end
	std::string szMask;															// the pattern, without the directory

	CFileIterator( const CFileIterator &a ) = delete;
	void operator=( const CFileIterator &a ) = delete;
	void Open( const std::string &szFullMask );
	void SkipToMatch();
public:
	CFileIterator() {  }
	CFileIterator( const std::string &szFullMask ) { Open( szFullMask ); }
	// file enumeration
	const CFileIterator& Next();
	bool IsEnd() const { return it == std::filesystem::directory_iterator(); }
	const CFileIterator& operator++() { return Next(); }
	bool IsDirectory() const
	{
		std::error_code ec;
		return it->is_directory( ec );
	}
	// directory_iterator never yields '.' or '..', so the callers that skip them are
	// asking something that can no longer be true. Kept so those tests still read
	// correctly rather than being deleted out from under them.
	bool IsDots() const { return false; }
	// file time attributes
	std::time_t GetLastWriteTime() const
	{
		boost::system::error_code ec;
		const std::time_t t = boost::filesystem::last_write_time( GetFullName(), ec );
		return ec ? 0 : t;
	}
	// file length. still an int, and so still truncating past 2 GB, as the
	// nFileSizeLow it replaces did
	int GetLength() const
	{
		std::error_code ec;
		return static_cast< int >( it->file_size( ec ) );
	}
	// file name (title + ext), full path (absolute path + name)
	std::string GetFileName() const { return it->path().filename().string(); }
	std::string GetFullName() const { return it->path().string(); }
	const std::string& GetBaseMask() const { return szMask; }
};

// enumerate all files by mask.
// при рекурсивной енумерации сначала входим в директорию, а потом только получаем её имя (при выходе из рекурсии)
template <class TEnumFunc>
void EnumerateFiles( const std::string &szStartDir, const char *pszMask, TEnumFunc callback, bool bRecurse )
{
	std::string szDir = szStartDir;
	// iterate throug all files by mask
	for ( CFileIterator it( (szDir + pszMask).c_str() ); !it.IsEnd(); ++it )
	{
		if ( !it.IsDirectory() )
			callback( it );
	}
	// iterate throug all dirs by "*.*"
	for ( CFileIterator it( (szDir + "*.*").c_str() ); !it.IsEnd(); ++it )
	{
		if ( it.IsDirectory() && !it.IsDots() )
		{
			// dive into recurse
			if ( bRecurse )
				// '/' rather than a backslash: off Windows a backslash is an ordinary
				// character in a file name, and this path is about to be split again
				EnumerateFiles( (it.GetFullName() + "/").c_str(), pszMask, callback, bRecurse );
			//
			callback( it );
		}
	}
}

SYSTEM_EXPORT void GetDirectoryFiles( const char *pszDirName, const char *pszMask, std::list<std::string> *pNames, bool bRecurse = true );
void DeleteFiles( const char *pszStartDir, const char *pszMask, bool bRecursive );
SYSTEM_EXPORT void DeleteDirectory( const std::string &szDir );

SYSTEM_EXPORT bool DoesFileExist( const std::string &szFileName );
// last write time of one named file, as CFileIterator reports it for an enumerated
// one. 0 when the file cannot be stat'ed, which is what GetFileAttributesEx
// returning FALSE left the caller's FILETIME as.
SYSTEM_EXPORT std::time_t GetLastWriteTime( const std::string &szFileName );
bool DoesFolderExist( const std::string &szFolderName );bool IsValidFileName( const std::string &szFileName );
// is valid win32 file name
SYSTEM_EXPORT bool IsValidDirName( const std::string &szName );
// copy file. create dst path before copying
SYSTEM_EXPORT bool CopyFile( const std::string &szSrcName, const std::string &szDstName );

SYSTEM_EXPORT std::string GetFullName( const std::string &szPath );
SYSTEM_EXPORT void GetFullName( std::string *pResult, const std::string &szPath );

std::string GetTempPath();
std::string GetTempFileName();

std::string GetCurrDir();
std::string GetNormalizedCurrDir();
void SetCurrDir( const std::string &szDir );
class CCurrDirHolder
{
	std::string szDir;
public:
	CCurrDirHolder() { szDir = GetCurrDir(); }
	~CCurrDirHolder() { SetCurrDir( szDir ); }
};

}

