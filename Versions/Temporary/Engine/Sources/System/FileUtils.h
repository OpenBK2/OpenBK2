#pragma once

#include "System_export.h"


namespace NFile
{

class SYSTEM_EXPORT CFileIterator
{
	HANDLE hFind;													// find file handle of the last search result
	WIN32_FIND_DATA findinfo;							// last search info
	string szPath;                   // path to the file
	string szMask;

	bool IsValid() const { return hFind != INVALID_HANDLE_VALUE; }
	CFileIterator( const CFileIterator &a ) {}
	void operator=( const CFileIterator &a ) {}
	bool Close();
	const CFileIterator& FindFirstFile( const string &szMask );
public:
	CFileIterator() : hFind( INVALID_HANDLE_VALUE ) {  }
	CFileIterator( const string &szMask ) { FindFirstFile( szMask ); }
	~CFileIterator() { Close(); }
	// file enumeration
	const CFileIterator& Next();
	bool IsEnd() const { return !IsValid(); }
	const CFileIterator& operator++() { return Next(); }
	// current file attributes check
	DWORD GetAttribs() const { return findinfo.dwFileAttributes; }
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
	// file time attributes
	//FILETIME GetCreationTime() const { return findinfo.ftCreationTime; }
	//FILETIME GetLastAccessTime() const { return findinfo.ftLastAccessTime; }
	const FILETIME &GetLastWriteTime() const { return findinfo.ftLastWriteTime; }
	// file length
	int GetLength() const { return findinfo.nFileSizeLow; }
	// file name (title + ext), full path (absolute path + name)
	string GetFileName() const { return findinfo.cFileName; }
	string GetFullName() const { return szPath + findinfo.cFileName; }
	const string& GetBasePath() const { return szPath; }
	const string& GetBaseMask() const { return szMask; }
};

// enumerate all files by mask.
// при рекурсивной енумерации сначала входим в директорию, а потом только получаем её имя (при выходе из рекурсии)
template <class TEnumFunc>
void EnumerateFiles( const string &szStartDir, const char *pszMask, TEnumFunc callback, bool bRecurse )
{
	string szDir = szStartDir;
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

void GetDirectoryDirs( const char *pszDirName, list<string> *pNames, bool bRecursive = true );
SYSTEM_EXPORT void GetDirectoryFiles( const char *pszDirName, const char *pszMask, list<string> *pNames, bool bRecurse = true );
void DeleteFiles( const char *pszStartDir, const char *pszMask, bool bRecursive );
SYSTEM_EXPORT void DeleteDirectory( const string &szDir );

SYSTEM_EXPORT bool DoesFileExist( const string &szFileName );
bool DoesFolderExist( const string &szFolderName );bool IsValidFileName( const string &szFileName );
// is valid win32 file name
SYSTEM_EXPORT bool IsValidDirName( const string &szName );
// copy file. create dst path before copying
bool CopyFile( const string &szSrcName, const string &szDstName );

SYSTEM_EXPORT string GetFullName( const string &szPath );
SYSTEM_EXPORT void GetFullName( string *pResult, const string &szPath );

string GetTempPath();
string GetTempFileName();

string GetCurrDir();
string GetNormalizedCurrDir();
void SetCurrDir( const string &szDir );
class CCurrDirHolder
{
	string szDir;
public:
	CCurrDirHolder() { szDir = GetCurrDir(); }
	~CCurrDirHolder() { SetCurrDir( szDir ); }
};

// return number of bytes, free for the caller on the selected drive
double GetFreeDiskSpace( const char *pszDrive );
}
