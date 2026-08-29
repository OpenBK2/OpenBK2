#pragma once

#include "System_export.h"

#include <boost/config.hpp>

#include <string>

namespace NFile
{

//! Check, is given char folder separator?
BOOST_FORCEINLINE bool IsFolderSeparator( const char chr )
{
	return chr == '/' || chr == '\\';
}

//! The separator this tree builds paths with, on every platform.
//!
//! Forward slash rather than the platform's own, and that is not a compromise
//! for the sake of one side. Windows file APIs take it; ComparePathEq and
//! MakeRelativePath already fold a backslash to one before comparing, in
//! ConvertFolderSeparator; GetNormalizedCurrDir already hands one back. This is
//! the separator the tree had settled on without ever writing it down.
//!
//! A literal backslash in a path is a bug off Windows, where it is an ordinary
//! filename character and turns the whole path into one nonexistent name. It is
//! also silent: nothing reports a file it could not open.
constexpr char PATH_SEPARATOR = '/';

// The directories the game keeps its files in, spelled once each.
//
// Case matters off Windows, and this tree did not previously agree with itself:
// Game/main.cpp asked for "profiles" while Main/Profiles.cpp built "Profiles",
// and Main/MODs.cpp used both "MODs" and "Mods" four lines apart. All four
// worked on Windows and none of the disagreements were visible there.
//
// The spelling here follows Versions/Current, which is what this repository
// ships and what CopyData.bat lays down. A distribution that spells them
// differently - the retail Fall of the Reich tree uses "data", "profiles" and
// "mods" - has to be renamed to match, because nothing here will guess.
constexpr char DIR_DATA[] = "Data";
constexpr char DIR_PROFILES[] = "Profiles";
constexpr char DIR_MODS[] = "MODs";

//! Append one component to a path being built, with exactly one separator
//! between them.
//!
//! Tolerates what the callers actually have: an empty component, a base that
//! already ends in a separator, which is what GetBaseDir and GetProfileRootDir
//! hand back, and a component that begins with one.
inline void AppendPathPart( std::string *pRes, const std::string &szPart )
{
	if ( szPart.empty() )
	{
		return;
	}
	// A leading separator is dropped only when there is already something to
	// join onto. On the first component it is the root, and stripping it turned
	// every absolute path into a relative one.
	const size_t nSkip = !pRes->empty() && IsFolderSeparator( szPart[0] ) ? 1 : 0;
	if ( !pRes->empty() && !IsFolderSeparator( ( *pRes )[pRes->size() - 1] ) )
	{
		*pRes += PATH_SEPARATOR;
	}
	pRes->append( szPart, nSkip, std::string::npos );
}

//! Join path components with PATH_SEPARATOR.
//!
//! JoinPath( GetBaseDir(), DIR_PROFILES, "startup.cfg" ) rather than a
//! concatenation with the separator written out at each site, which is how the
//! backslashes got in and stayed.
//!
//! Does not append a trailing separator. A caller that wants a directory to
//! concatenate against says so, with AppendSlash or by joining the next
//! component instead.
template <typename... TParts>
std::string JoinPath( const TParts &...parts )
{
	std::string result;
	// fold over the pack, left to right
	( AppendPathPart( &result, parts ), ... );
	return result;
}

//! Get folder path from full file path name (c:/mydir/myfile.txt => c:/mydir/).
//! \return Folder path or empty string (in the cast of bare filename)
SYSTEM_EXPORT std::string GetFilePath( const std::string &szFullFilePath );

//! Get file name from full file path name (c:/mydir/myfile.txt => myfile.txt).
//! \return File name or empty string (in the case of folder path)
SYSTEM_EXPORT std::string GetFileName( const std::string &szFullFilePath );

//! Get file title full file path name (c:/mydir/myfile.txt => myfile)
//! \return File title or empty string (in the case of folder path)
SYSTEM_EXPORT std::string GetFileTitle( const std::string &szFullFilePath );

//! Get file extension full file path name (c:/mydir/myfile.txt => .txt)
//! \return File extension or empty string (in the case of folder path or extension-less file name)
SYSTEM_EXPORT std::string GetFileExt( const std::string &szFullFilePath );

//! Cut file's extension (any part before '.' (and this '.' must be before path separator))
//! \return File full path without extension
SYSTEM_EXPORT std::string CutFileExt( const std::string &szFullFilePath, const char *pszExt );

//! Split full file path into parts.
SYSTEM_EXPORT void SplitPath( std::list<std::string> *pRes, const std::string &szFullFilePath );

//! Compare (sub)pathes on equality, ignore case and path separator type.
//! \return True if (sub)pathes are equal and false in other case
SYSTEM_EXPORT bool ComparePathEq( const int nStart1, const int nLength1, const std::string &szPath1,
									  const int nStart2, const int nLength2, const std::string &szPath2 );
//! Compare full pathes on equality, ignore case and path separator type.
//! \return True if pathes are equal and false in other case
BOOST_FORCEINLINE bool ComparePathEq( const std::string &szPath1, const std::string &szPath2 ) { return ComparePathEq( 0, szPath1.size(), szPath1, 0, szPath2.size(), szPath2 ); }

//! Compare (sub)pathes on less-then, ignore case and path separator type.
//! \return True if first (sub)path are less then second one and false in other case
SYSTEM_EXPORT bool ComparePathLt( const int nStart1, const int nLength1, const std::string &szPath1,
									 const int nStart2, const int nLength2, const std::string &szPath2 );
//! Compare full pathes on less-then, ignore case and path separator type.
//! \return True if first path are less then second and false in other case
BOOST_FORCEINLINE bool ComparePathLt( const std::string &szPath1, const std::string &szPath2 ) { return ComparePathLt( 0, szPath1.size(), szPath1, 0, szPath2.size(), szPath2 ); }

//! Returns true if supplyed path is in relative form
SYSTEM_EXPORT bool IsPathRelative( const std::string &szPath );
//! Make relative path from parent's and full one. Prepends with '/' in the case of the result path are absolute (can't be represented as relative to parent's)
SYSTEM_EXPORT void MakeRelativePath( std::string *pRes, const std::string &szFullPath, const std::string &szParentPath );
//! Make full path from relative and parent's. Is relative path already prepended with '/' or '\', it is treated as absolute
SYSTEM_EXPORT void MakeFullPath( std::string *pRes, const std::string &szRelativePath, const std::string &szParentPath );

//! Replace all '\' to '/'. One can pass the same string in both parameters
SYSTEM_EXPORT void NormalizePath( std::string *pRes, const std::string &szFilePath );
BOOST_FORCEINLINE void NormalizePath( std::string *pFilePath ) { NormalizePath( pFilePath, *pFilePath ); }

SYSTEM_EXPORT void AppendSlash( std::string *pFilePath, const char cSlash = '\\' );
SYSTEM_EXPORT void ConvertSlashes( std::string *pFilePath, const char cFrom, const char cTo );

//! Create every missing directory in the path. True if the path exists
//! afterwards, which includes the case where it already did.
SYSTEM_EXPORT bool CreatePath( const std::string &szFullPath );

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
