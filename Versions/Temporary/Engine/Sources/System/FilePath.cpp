#include "stdafx.h"

#include "FilePath.h"

#include <filesystem>
#include <system_error>

#include "Misc/StrProc.h"

#include <boost/config.hpp>

namespace NFile
{

BOOST_FORCEINLINE char ConvertFolderSeparator( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	return (chr & mask) | ('/' & (~mask));
}

BOOST_FORCEINLINE char ConvertChar( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	const char chr1 = (chr & mask) | ('/' & (~mask));
	return chr1 - ( ('A' - 'a') & ( (('A' - chr1 - 1) & (chr1 - 'Z' - 1)) >> 7 ) );
}

static size_t FindLastSlash( const std::string &szFullFilePath )
{
	for (size_t i = szFullFilePath.size(); i-- > 0; )
	{
		if ( IsFolderSeparator(szFullFilePath[i]) )
			return i;
	}
	return std::string::npos;
}

// ************************************************************************************************************************ //
// **
// ** path splitting functions
// **
// **
// **
// ************************************************************************************************************************ //

std::string GetFilePath( const std::string &szFullFilePath )
{
	const size_t nPos = FindLastSlash( szFullFilePath );
	return nPos != std::string::npos ? szFullFilePath.substr( 0, nPos + 1 ) : "";
}

std::string GetFileName( const std::string &szFullFilePath )
{
	const size_t nPos = FindLastSlash( szFullFilePath );
	return nPos != std::string::npos ? szFullFilePath.substr( nPos + 1 ) : szFullFilePath;
}

std::string GetFileTitle( const std::string &szFullFilePath )
{
	const std::string szFileName = GetFileName( szFullFilePath );
	const size_t nPos = szFileName.rfind( '.' );
	return nPos != std::string::npos ? szFileName.substr( 0, nPos ) : szFileName;
}

std::string GetFileExt( const std::string &szFullFilePath )
{
	const std::string szFileName = GetFileName( szFullFilePath );
	const size_t nPos = szFileName.rfind( '.' );
	return nPos != std::string::npos ? szFileName.substr( nPos ) : "";
}

std::string CutFileExt( const std::string &szFullFilePath, const char *pszExt )
{
	if ( szFullFilePath.empty() )
		return szFullFilePath;
	//
	if ( pszExt == 0 || pszExt[0] == 0 )
	{
		const size_t nPos = FindLastSlash( szFullFilePath );
		for ( size_t i = szFullFilePath.size() - 1; i > nPos; --i )
		{
			if ( szFullFilePath[i] == '.' )
				return szFullFilePath.substr( 0, i );
		}
		return szFullFilePath;
	}
	else
	{
		const size_t nPos = szFullFilePath.rfind( '.' );
		const size_t nCmpSize = szFullFilePath.size() - (nPos + 1);
		if ( nPos != std::string::npos && ComparePathEq(nPos + 1, nCmpSize, szFullFilePath, 0, strlen(pszExt), pszExt) != false )
			return szFullFilePath.substr( 0, nPos );
		return  szFullFilePath;
	}
}

static size_t FindNextSlash( const std::string &szFullFilePath, const int nStartPos )
{
	for ( size_t i = nStartPos; i < szFullFilePath.size(); ++i )
	{
		if ( IsFolderSeparator(szFullFilePath[i]) )
			return i;
	}
	return std::string::npos;
}
void SplitPath( std::list<std::string> *pRes, const std::string &szFullFilePath )
{
	size_t nLastPos = 0;
	do
	{
		const size_t nPos = FindNextSlash( szFullFilePath, nLastPos );
		pRes->push_back( szFullFilePath.substr( nLastPos, nPos - nLastPos ) );
		nLastPos = nPos + 1;
	} while( nLastPos != (std::string::npos + 1) );
}

// ************************************************************************************************************************ //
// **
// ** comparison functions
// **
// **
// **
// ************************************************************************************************************************ //

BOOST_FORCEINLINE char ConvertCharASCII( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	const char chr1 = (chr & mask) | ('/' & (~mask));
	return chr1 - ( ('A' - 'a') & ( (('A' - chr1 - 1) & (chr1 - 'Z' - 1)) >> 7 ) );
}

bool ComparePathEq( const int nStart1, const int nLength1, const std::string &szPath1,
									  const int nStart2, const int nLength2, const std::string &szPath2 )
{
	if ( nLength1 != nLength2 )
		return false;
	if ( &szPath1 == &szPath2 )
		return true;
	if ( nLength1 == 0 )
		return true;
	//
	const char *p1 = &( szPath1[nStart1] );
	const char *p2 = &( szPath2[nStart2] );
	for ( int i = 0; i < nLength1; ++i )
	{
		if ( ConvertCharASCII(p1[i]) != ConvertCharASCII(p2[i]) )
			return false;
	}
	return true;
}

bool ComparePathLt( const int nStart1, const int nLength1, const std::string &szPath1,
									 const int nStart2, const int nLength2, const std::string &szPath2 )
{
	if ( nLength1 < nLength2 )
		return true;
	if ( &szPath1 == &szPath2 )
		return false;
	if ( nLength2 == 0 )
		return false;
	//
	const int nSize = (std::min)( nLength1, nLength2 );
	const char *p1 = &( szPath1[nStart1] );
	const char *p2 = &( szPath2[nStart2] );
	for ( int i = 0; i < nSize; ++i )
	{
		if ( ConvertCharASCII(p1[i]) >= ConvertCharASCII(p2[i]) )
			return false;
	}
	return true;
}

bool IsPathRelative( const std::string &szPath )
{
	return !IsFolderSeparator(szPath[0]) && (szPath[1] != ':');
}

void MakeRelativePath( std::string *pRes, const std::string &szFullPath, const std::string &szParentPath )
{
	if ( szFullPath.empty() )
	{
		pRes->clear();
		return;
	}
	//
	const size_t nPos = FindLastSlash( szParentPath );
	if ( nPos != std::string::npos && ComparePathEq(0, nPos + 1, szParentPath, 0, nPos + 1, szFullPath) )
		*pRes = szFullPath.c_str() + nPos + 1;
	else
		*pRes = '/' + szFullPath;
}

void MakeFullPath( std::string *pRes, const std::string &szRelativePath, const std::string &szParentPath )
{
	if ( szRelativePath.empty() )
	{
		pRes->clear();
		return;
	}
	else if ( szParentPath.empty() )
	{
		*pRes = IsFolderSeparator(szRelativePath[0]) == true ? szRelativePath.c_str() + 1 : szRelativePath;
		return;
	}
	//
	if ( IsFolderSeparator(szRelativePath[0]) ) // absolute path
		*pRes = szRelativePath.c_str() + 1;
	else	// relative to parent's path
	{
		const size_t nPos = FindLastSlash( szParentPath );
		if ( nPos != std::string::npos )
			*pRes = szParentPath.substr( 0, nPos ) + "/" + szRelativePath;
		else
			*pRes = szRelativePath;
	}
}

void NormalizePath( std::string *pRes, const std::string &szFilePath )
{
	const size_t nSize = szFilePath.size();
	pRes->resize( szFilePath.size() );
	for ( size_t i = 0; i < nSize; ++i )
		(*pRes)[i] = ConvertFolderSeparator( szFilePath[i] );
}

void AppendSlash( std::string *pFilePath, const char cSlash )
{
	if ( !pFilePath->empty() && !IsFolderSeparator((*pFilePath)[pFilePath->size() - 1]) )
		(*pFilePath) += cSlash;
}
void ConvertSlashes( std::string *pFilePath, const char cFrom, const char cTo )
{
	for ( std::string::iterator it = pFilePath->begin(); it != pFilePath->end(); ++it )
	{
		if ( *it == cFrom )
			*it = cTo;
	}
}

bool CreatePath( const std::string &_szFullPath )
{
	// GetFilePath yields an empty string for a name with no directory part,
	// and several callers hand that straight over. There is nothing to create
	// and nothing wrong, but create_directories calls it a missing path.
	if ( _szFullPath.empty() )
	{
		return true;
	}

	// The path is used as given. Callers that build one with a raw backslash
	// are wrong off Windows, where a backslash is an ordinary filename
	// character and the whole thing becomes a single directory, but rewriting
	// separators here would hide those callers rather than fix them.
	//
	// The throwing overload would turn a full disk or a denied directory into
	// an exception at fourteen call sites that have never handled one. The
	// error_code overload reports it instead.
	//
	// The return value cannot be used: it is false when the directory was
	// already there, and also false on success when the path ends in a
	// separator, which is the common case here because callers pass the result
	// of GetFilePath. Only the error_code says whether anything went wrong.
	std::error_code ec;
	std::filesystem::create_directories( _szFullPath, ec );
	return !ec;
}

// ************************************************************************************************************************ //
// **
// **
// **
// **
// **
// ************************************************************************************************************************ //

int CFilePath::MakeHashKey() const
{
	unsigned int uHashKey = 0;
	for ( std::string::const_iterator it = this->begin(); it != this->end(); ++it )
		uHashKey = 5*uHashKey + ConvertChar( *it );
	return uHashKey;
}

int CFilePath::operator&( IBinSaver &saver )
{
	saver.Add( 1, (std::string*)this );
	return 0;
}

}
