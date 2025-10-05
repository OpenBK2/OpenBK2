#pragma once
#include "System_export.h"

namespace NFile
{

//! Check, is given char folder separator?
__forceinline bool IsFolderSeparator( const char chr )
{
	return chr == '/' || chr == '\\';
}

//! Get folder path from full file path name (c:/mydir/myfile.txt => c:/mydir/).
//! \return Folder path or empty string (in the cast of bare filename)
SYSTEM_EXPORT std::string GetFilePath( const std::string &szFullFilePath );

//! Get file name from full file path name (c:/mydir/myfile.txt => myfile.txt).
//! \return File name or empty string (in the case of folder path)
SYSTEM_EXPORT std::string GetFileName( const std::string &szFullFilePath );

//! Get file title full file path name (c:/mydir/myfile.txt => myfile)
//! \return File title or empty string (in the case of folder path)
std::string GetFileTitle( const std::string &szFullFilePath );

//! Get file extension full file path name (c:/mydir/myfile.txt => .txt)
//! \return File extension or empty string (in the case of folder path or extension-less file name)
SYSTEM_EXPORT std::string GetFileExt( const std::string &szFullFilePath );

//! Cut file's extension (any part before '.' (and this '.' must be before path separator))
//! \return File full path without extension
std::string CutFileExt( const std::string &szFullFilePath, const char *pszExt );

//! Split full file path into parts.
SYSTEM_EXPORT void SplitPath( std::list<std::string> *pRes, const std::string &szFullFilePath );

//! Compare (sub)pathes on equality, ignore case and path separator type.
//! \return True if (sub)pathes are equal and false in other case
SYSTEM_EXPORT bool ComparePathEq( const int nStart1, const int nLength1, const std::string &szPath1,
									  const int nStart2, const int nLength2, const std::string &szPath2 );
//! Compare full pathes on equality, ignore case and path separator type.
//! \return True if pathes are equal and false in other case
__forceinline bool ComparePathEq( const std::string &szPath1, const std::string &szPath2 ) { return ComparePathEq( 0, szPath1.size(), szPath1, 0, szPath2.size(), szPath2 ); }

//! Compare (sub)pathes on less-then, ignore case and path separator type.
//! \return True if first (sub)path are less then second one and false in other case
bool ComparePathLt( const int nStart1, const int nLength1, const std::string &szPath1,
									 const int nStart2, const int nLength2, const std::string &szPath2 );
//! Compare full pathes on less-then, ignore case and path separator type.
//! \return True if first path are less then second and false in other case
__forceinline bool ComparePathLt( const std::string &szPath1, const std::string &szPath2 ) { return ComparePathLt( 0, szPath1.size(), szPath1, 0, szPath2.size(), szPath2 ); }

//! Returns true if supplyed path is in relative form
SYSTEM_EXPORT bool IsPathRelative( const std::string &szPath );
//! Make relative path from parent's and full one. Prepends with '/' in the case of the result path are absolute (can't be represented as relative to parent's)
SYSTEM_EXPORT void MakeRelativePath( std::string *pRes, const std::string &szFullPath, const std::string &szParentPath );
//! Make full path from relative and parent's. Is relative path already prepended with '/' or '\', it is treated as absolute
SYSTEM_EXPORT void MakeFullPath( std::string *pRes, const std::string &szRelativePath, const std::string &szParentPath );

//! Replace all '\' to '/'. One can pass the same string in both parameters
SYSTEM_EXPORT void NormalizePath( std::string *pRes, const std::string &szFilePath );
__forceinline void NormalizePath( std::string *pFilePath ) { NormalizePath( pFilePath, *pFilePath ); }

void AppendSlash( std::string *pFilePath, const char cSlash = '\\' );
void RemoveSlash( std::string *pFilePath, const char cSlash = '\\' );
SYSTEM_EXPORT void ConvertSlashes( std::string *pFilePath, const char cFrom, const char cTo );

//! Create path on the disk
SYSTEM_EXPORT void CreatePath( const std::string &szFullPath );

class CFilePath : public std::string
{
public:
	CFilePath() {}
	CFilePath( const char *_pszFilePath ): std::string( _pszFilePath ) {}
	CFilePath( const std::string &_szFilePath ): std::string( _szFilePath ) {}
	CFilePath( const CFilePath &path ): std::string( path ) {}
	//
	const CFilePath &operator=( const char *_pszFilePath ) { (*(std::string*)this) = _pszFilePath; return *this; }
	const CFilePath &operator=( const std::string &szFilePath ) { (*(std::string*)this) = szFilePath; return *this; }
	const CFilePath &operator=( const CFilePath &filePath ) { (*(std::string*)this) = filePath; return *this; }
	//
	bool operator==( const std::string &_szFilePath ) const { return ComparePathEq(0, this->size(), *this, 0, _szFilePath.size(), _szFilePath); }
	bool operator==( const char *_pszFilePath ) const { return ComparePathEq(0, this->size(), *this, 0, strlen(_pszFilePath), _pszFilePath); }
	bool operator!=( const std::string &_szFilePath ) const { return !operator==( _szFilePath ); }
	bool operator!=( const char *_pszFilePath ) const { return !operator==( _pszFilePath ); }
	bool operator<( const std::string &_szFilePath ) const { return ComparePathLt(0, this->size(), *this, 0, _szFilePath.size(), _szFilePath); }
	bool operator<( const char *_pszFilePath ) const { return ComparePathLt(0, this->size(), *this, 0, strlen(_pszFilePath), _pszFilePath); }
	//
	SYSTEM_EXPORT int MakeHashKey() const;
	//
	SYSTEM_EXPORT int operator&( IBinSaver &saver );
};

}

template<> struct std::hash<NFile::CFilePath>
{
	size_t operator()( const NFile::CFilePath &path ) const { return path.MakeHashKey(); }
};
