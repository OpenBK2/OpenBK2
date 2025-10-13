#include "stdafx.h"

#include "FileUtils.h"
#include "FilePath.h"
#include "Misc/StrProc.h"

#if BOOST_OS_WINDOWS
#include <objbase.h>
#include <direct.h>
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstdint>

namespace NFile
{

const uint32_t BUFFER_SIZE = 1024;
static char buffer[BUFFER_SIZE];

// ************************************************************************************************************************ //
// **
// ** class CFileIterator functions
// **
// **
// **
// ************************************************************************************************************************ //

const CFileIterator& CFileIterator::FindFirstFile( const std::string &_szMask )
{
	szPath = _szMask;
	NStr::ReplaceAllChars( &szPath, '/', '\\' );
	int pos = szPath.rfind( '\\' );
	if ( pos == std::string::npos )
	{
		szMask = _szMask;
		szPath.clear();
	}
	else
	{
		szMask = szPath.substr( pos + 1 );
		szPath = szPath.substr( 0, pos );
	}
	//
	if ( !szPath.empty() && szPath[szPath.size() - 1] != '\\' ) 
		szPath += "\\";
	// create absolute path from the relative one
	NFile::GetFullName( &szPath, szPath );
	//
	if ( !szPath.empty() && szPath[szPath.size() - 1] != '\\' ) 
		szPath += "\\";
	//
	hFind = ::FindFirstFile( _szMask.c_str(), &findinfo );
	return *this;
}

const CFileIterator& CFileIterator::Next()
{
	if ( !IsValid() )
		return *this;
	if ( ::FindNextFile(hFind, &findinfo) == 0 )
		Close();
	return *this;
}

bool CFileIterator::Close() 
{ 
	if ( IsValid() ) 
	{
		const bool bRet = ::FindClose( hFind ) != 0; 
		hFind = INVALID_HANDLE_VALUE;
		return bRet;
	}
	return true; 
}

// ************************************************************************************************************************ //
//                                         external file utilites
// ************************************************************************************************************************ //

class CDeleteFiles
{
	bool bDeleteRO;
	bool bDeleteDir;
public:
	CDeleteFiles( bool _bDeleteRO, bool _bDeleteDir ) : bDeleteRO( _bDeleteRO ), bDeleteDir( _bDeleteDir ) {  }
	//
	void operator()( const CFileIterator &it )
	{
		if ( !it.IsDirectory() )
		{
			if ( bDeleteRO && it.IsReadOnly() )
				SetFileAttributes( it.GetFullName().c_str(), it.GetAttribs() & ~FILE_ATTRIBUTE_READONLY );
			DeleteFile( it.GetFullName().c_str() );
		}
		else if ( bDeleteDir )
		{
			if ( bDeleteRO && it.IsReadOnly() )
				SetFileAttributes( it.GetFullName().c_str(), it.GetAttribs() & ~FILE_ATTRIBUTE_READONLY );
			RemoveDirectory( it.GetFullName().c_str() );
		}
	}
};

void DeleteFiles( const char *pszStartDir, const char *pszMask, bool bRecursive )
{
	EnumerateFiles( pszStartDir, pszMask, CDeleteFiles(true, false), bRecursive );
}

void DeleteDirectory( const std::string &szDir )
{
	EnumerateFiles( szDir, "*.*", CDeleteFiles(true, true), true );
	RemoveDirectory( szDir.c_str() );
}

class CDirFileEnum
{
	std::list<std::string> *pNames;			// store names here
	bool bDir;														// enumerate dirs
	bool bFile;														// enumerate files
public:
	CDirFileEnum( std::list<std::string> *_pNames, bool _bDir, bool _bFile )
		: pNames( _pNames ), bDir( _bDir ), bFile( _bFile ) {  }
	void operator()( const CFileIterator &it )
	{
		if ( it.IsDirectory() )
		{
			if ( bDir )
				pNames->push_back( it.GetFullName() );
		}
		else if ( bFile )
			pNames->push_back( it.GetFullName() );
	}
};
void GetDirectoryDirs( const char *pszDirName, std::list<std::string> *pNames, bool bRecursive )
{
	EnumerateFiles( pszDirName, "*.*", CDirFileEnum(pNames, true, false), bRecursive );
}
void GetDirectoryFiles( const char *pszDirName, const char *pszMask, std::list<std::string> *pNames, bool bRecurse )
{
	EnumerateFiles( pszDirName, pszMask, CDirFileEnum(pNames, false, true), bRecurse );
}

// return number of bytes, free for the caller on the selected drive
double GetFreeDiskSpace( const char *pszDrive )
{
	typedef BOOL (WINAPI *GDFSE)( LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER );

	GDFSE pfnGetDiskFreeSpaceEx = (GDFSE)GetProcAddress( GetModuleHandle("kernel32.dll"), "GetDiskFreeSpaceExA" );
	BOOL bRetVal = FALSE;
	double fRetVal = 0;
	if ( pfnGetDiskFreeSpaceEx )
	{
	 ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	 bRetVal = (*pfnGetDiskFreeSpaceEx)( pszDrive, &i64FreeBytesToCaller,
																	     &i64TotalBytes, &i64FreeBytes );
	 fRetVal = double( __int64(i64FreeBytesToCaller.QuadPart) );
	} 
	else 
	{
		unsigned long dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
		bRetVal = GetDiskFreeSpace( pszDrive, &dwSectPerClust, &dwBytesPerSect,
																&dwFreeClusters, &dwTotalClusters );
		fRetVal = double( dwFreeClusters ) * double( dwSectPerClust ) * double( dwBytesPerSect );
	}

	return ( bRetVal == 0 ? 0 : fRetVal );
}

bool DoesFileExist( const std::string &szFileName )
{
	//return _access( szFileName.c_str(), 0 ) != -1;
	HANDLE hFile = ::CreateFile( szFileName.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	::CloseHandle( hFile );
	return true;
}

bool DoesFolderExist( const std::string &szFolderName )
{
	//return _access( szFileName.c_str(), 0 ) != -1;
	HANDLE hFile = ::CreateFile( szFolderName.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	::CloseHandle( hFile );
	return true;
}

bool IsValidFileName( const std::string &szFileName )
{
	const char *pszFileName = szFileName.c_str();
	const char *_pszFileName = max( strrchr( pszFileName, '\\' ), strrchr( pszFileName, '/' ) );
	if ( 0 == _pszFileName )
		_pszFileName = pszFileName;
	else
		++ _pszFileName;
	static char szValidCharSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-_0123456789 ";
	return 0 == _strspnp( _pszFileName, szValidCharSet );
}

static const char *pszWrongNames[] = {
	"con", "prn", "aux", "clock$", "nul", 
	"com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9", 
	"lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9",
	0
};

// is valid win32 file name
bool IsValidDirName( const std::string &_szName )
{
	std::string szName = _szName;
	if ( szName.empty() || szName.size() > 250 )
		return false;
	NStr::ToLower( &szName );
	for ( int i = 0; i < szName.size(); ++i )
	{
		int c = (unsigned char) szName[i];
		if ( c < 33 || c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '*' || c == '?' )
			return false;
	}
	for ( const char **p = pszWrongNames; *p; ++p )
	{
		if ( szName == *p )
			return false;
	}
	return true;
}

bool CopyFile( const std::string &szSrcName, const std::string &szDstName )
{
	CreatePath( GetFilePath( szDstName ) );
	return ::CopyFile( szSrcName.c_str(), szDstName.c_str(), false ) != 0;
}

std::string GetFullName( const std::string &szPath )
{
	char *pszBufferFileName = 0;
	GetFullPathName( szPath.c_str(), 1024, buffer, &pszBufferFileName );
	return buffer;
}

void GetFullName( std::string *pResult, const std::string &szPath )
{
	char *pszBufferFileName = 0;
	const uint32_t dwLength = GetFullPathName( szPath.c_str(), 1024, buffer, &pszBufferFileName );
	pResult->resize( dwLength );
	memcpy( &((*pResult)[0]), buffer, dwLength );
}

std::string GetTempPath()
{
	int nLength = ::GetTempPath( BUFFER_SIZE, buffer );
	if ( nLength == 0 )
		return ".\\";
	else if ( buffer[nLength - 1] != '\\' )
	{
		buffer[nLength] = '\\';
		++nLength;
	}
	buffer[nLength] = 0;
	return buffer;
}
std::string GetTempFileName()
{
	GUID guid;
	::CoCreateGuid( &guid );
	std::string szFileName;
	NStr::GUID2String( &szFileName, guid );
	return GetTempPath() + szFileName;
}

std::string GetCurrDir()
{
	char buffer[1024];
	const int nLen = GetCurrentDirectory( 1024, buffer );
	return nLen == 0 ? "" : buffer;
}

std::string GetNormalizedCurrDir()
{
	std::string szCurrDir = GetCurrDir();
	NFile::NormalizePath( &szCurrDir );
	NFile::AppendSlash( &szCurrDir, '/' );
	return szCurrDir;
}

void SetCurrDir( const std::string &szDir )
{
	SetCurrentDirectory( szDir.c_str() );
}

}; // namespace NFile ends


