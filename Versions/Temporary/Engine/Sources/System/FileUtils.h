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
// remove one file. Win32's DeleteFile, which cannot be spelled that way here:
// windows.h rewrites the name to DeleteFileA, and a translation unit that has not
// included it would then look for a symbol nobody defines.
SYSTEM_EXPORT bool RemoveFile( const std::string &szFileName );
SYSTEM_EXPORT bool DoesFolderExist( const std::string &szFolderName );
// is valid win32 file name
SYSTEM_EXPORT bool IsValidFileName( const std::string &szFileName );
SYSTEM_EXPORT bool IsValidDirName( const std::string &szName );
// copy file. create dst path before copying
SYSTEM_EXPORT bool CopyFile( const std::string &szSrcName, const std::string &szDstName );

SYSTEM_EXPORT std::string GetFullName( const std::string &szPath );
SYSTEM_EXPORT void GetFullName( std::string *pResult, const std::string &szPath );

//! Respell a path that arrived from the game data so that it matches what is
//! actually on disk, forgiving the separator and the case its author's
//! filesystem accepted.
//!
//! szBaseDir is the directory szRelPath is relative to and must end in a
//! separator; on success *pRes is szRelPath respelled, so szBaseDir + *pRes
//! opens the file. Both '/' and '\' are accepted on the way in and the result
//! uses PATH_SEPARATOR throughout.
//!
//! For the *data* boundary only, and deliberately not for paths this repository
//! builds itself. The engine's own paths are a closed set that is spelled
//! correctly at the source; the data's are an open set, since a map published in
//! 2007 cannot be recompiled and a mod published tomorrow will be authored on
//! Windows and never tested anywhere else. Forgiving the second is the only way
//! unmodifiable data loads at all; forgiving the first would hide callers that
//! should be fixed. See docs/port/PORT_ROADMAP.md.
//!
//! An exact match is always preferred, and only a component that has none is
//! searched for case insensitively, so nothing that already resolves changes
//! meaning. Where several names differ only in case - which Windows cannot
//! produce but a Linux checkout can - the lexicographically smallest wins, so
//! two machines with the same tree resolve to the same file rather than to
//! whichever the directory happened to list first. That matters beyond
//! tidiness: the file that loads here feeds a simulation that has to stay
//! bit-identical across machines.
//!
//! Always false on Windows, where the filesystem has already done this and a
//! miss means the file is genuinely absent, so scanning would only cost time.
SYSTEM_EXPORT bool ResolveDataPathCase( std::string *pRes, const std::string &szBaseDir, const std::string &szRelPath );

SYSTEM_EXPORT std::string GetTempPath();
SYSTEM_EXPORT std::string GetTempFileName();

// exported because every caller of these three is outside System: Game reads the
// normalized one for its log paths, MapEditor sets the directory, and dbcodegen,
// dbstruct and TestParsing read it. Without the macro they link only inside the
// DLL that defines them, which MSVC reports and GCC does not
SYSTEM_EXPORT std::string GetCurrDir();
SYSTEM_EXPORT std::string GetNormalizedCurrDir();
SYSTEM_EXPORT void SetCurrDir( const std::string &szDir );
class CCurrDirHolder
{
	std::string szDir;
public:
	CCurrDirHolder() { szDir = GetCurrDir(); }
	~CCurrDirHolder() { SetCurrDir( szDir ); }
};

}

