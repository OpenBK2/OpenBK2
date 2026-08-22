#pragma once

#include "System_export.h"

#include <cstdint>
#include <ctime>

#include <boost/filesystem/operations.hpp>

namespace NFile
{

class SYSTEM_EXPORT CFileIterator
{
	HANDLE hFind;													// find file handle of the last search result
	WIN32_FIND_DATA findinfo;							// last search info
	std::string szPath;                   // path to the file
	std::string szMask;

	bool IsValid() const { return hFind != INVALID_HANDLE_VALUE; }
	CFileIterator( const CFileIterator &a ) {}
	void operator=( const CFileIterator &a ) {}
	bool Close();
	const CFileIterator& FindFirstFile( const std::string &szMask );
public:
	CFileIterator() : hFind( INVALID_HANDLE_VALUE ) {  }
	CFileIterator( const std::string &szMask ) { FindFirstFile( szMask ); }
	~CFileIterator() { Close(); }
	// file enumeration
	const CFileIterator& Next();
	bool IsEnd() const { return !IsValid(); }
	const CFileIterator& operator++() { return Next(); }
	// current file attributes check
	uint32_t GetAttribs() const { return findinfo.dwFileAttributes; }
	bool IsReadOnly() const { return ( GetAttribs() & FILE_ATTRIBUTE_READONLY ) != 0; }
	bool IsSystem() const { return ( GetAttribs() & FILE_ATTRIBUTE_SYSTEM ) != 0; }
	bool IsHidden() const { return ( GetAttribs() & FILE_ATTRIBUTE_HIDDEN ) != 0; }
	bool IsDirectory() const { return ( GetAttribs() & FILE_ATTRIBUTE_DIRECTORY ) != 0; }
	// special kind of directory: '.' - this dir and '..' - parent dir
	bool IsDots() const
	{
		return ( ( findinfo.cFileName[0] == '.' ) && 
			       ( (findinfo.cFileName[1] == '\0') || 
						   ((findinfo.cFileName[1] == '.') && (findinfo.cFileName[2] == '\0')) ) );
	}
	// file time attributes. findinfo holds the bare name, so the full path has
	// to be rebuilt before asking the filesystem about it
	std::time_t GetLastWriteTime() const
	{
		boost::system::error_code ec;
		const std::time_t t = boost::filesystem::last_write_time( GetFullName(), ec );
		return ec ? 0 : t;
	}
	// file length
	int GetLength() const { return findinfo.nFileSizeLow; }
	// file name (title + ext), full path (absolute path + name)
	std::string GetFileName() const { return findinfo.cFileName; }
	std::string GetFullName() const { return szPath + findinfo.cFileName; }
	const std::string& GetBasePath() const { return szPath; }
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
				EnumerateFiles( (it.GetFullName() +  "\\").c_str(), pszMask, callback, bRecurse );
			//
			callback( it );
		}
	}
}

void GetDirectoryDirs( const char *pszDirName, std::list<std::string> *pNames, bool bRecursive = true );
SYSTEM_EXPORT void GetDirectoryFiles( const char *pszDirName, const char *pszMask, std::list<std::string> *pNames, bool bRecurse = true );
void DeleteFiles( const char *pszStartDir, const char *pszMask, bool bRecursive );
SYSTEM_EXPORT void DeleteDirectory( const std::string &szDir );

SYSTEM_EXPORT bool DoesFileExist( const std::string &szFileName );
bool DoesFolderExist( const std::string &szFolderName );bool IsValidFileName( const std::string &szFileName );
// is valid win32 file name
SYSTEM_EXPORT bool IsValidDirName( const std::string &szName );
// copy file. create dst path before copying
bool CopyFile( const std::string &szSrcName, const std::string &szDstName );

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

// return number of bytes, free for the caller on the selected drive
double GetFreeDiskSpace( const char *pszDrive );
}

