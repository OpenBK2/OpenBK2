#include "StdAfx.h"

#include "FilePath.h"
#include "../Misc/StrProc.h"

namespace NFile
{

__forceinline char ConvertFolderSeparator( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	return (chr & mask) | ('/' & (~mask));
}

__forceinline char ConvertChar( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	const char chr1 = (chr & mask) | ('/' & (~mask));
	return chr1 - ( ('A' - 'a') & ( (('A' - chr1 - 1) & (chr1 - 'Z' - 1)) >> 7 ) );
}

static int FindLastSlash( const string &szFullFilePath )
{
	int i = szFullFilePath.size();
	while ( --i >= 0 )
	{
		if ( IsFolderSeparator(szFullFilePath[i]) )
			return i;
	}
	return string::npos;
}

// ************************************************************************************************************************ //
// **
// ** path splitting functions
// **
// **
// **
// ************************************************************************************************************************ //

string GetFilePath( const string &szFullFilePath )
{
	const int nPos = FindLastSlash( szFullFilePath );
	return nPos != string::npos ? szFullFilePath.substr( 0, nPos + 1 ) : "";
}

string GetFileName( const string &szFullFilePath )
{
	const int nPos = FindLastSlash( szFullFilePath );
	return nPos != string::npos ? szFullFilePath.substr( nPos + 1 ) : szFullFilePath;
}

string GetFileTitle( const string &szFullFilePath )
{
	const string szFileName = GetFileName( szFullFilePath );
	const int nPos = szFileName.rfind( '.' );
	return nPos != string::npos ? szFileName.substr( 0, nPos ) : szFileName;
}

string GetFileExt( const string &szFullFilePath )
{
	const string szFileName = GetFileName( szFullFilePath );
	const int nPos = szFileName.rfind( '.' );
	return nPos != string::npos ? szFileName.substr( nPos ) : "";
}

string CutFileExt( const string &szFullFilePath, const char *pszExt )
{
	if ( szFullFilePath.empty() )
		return szFullFilePath;
	//
	if ( pszExt == 0 || pszExt[0] == 0 )
	{
		const int nPos = FindLastSlash( szFullFilePath );
		for ( int i = szFullFilePath.size() - 1; i > nPos; --i )
		{
			if ( szFullFilePath[i] == '.' )
				return szFullFilePath.substr( 0, i );
		}
		return szFullFilePath;
	}
	else
	{
		const int nPos = szFullFilePath.rfind( '.' );
		const int nCmpSize = szFullFilePath.size() - (nPos + 1);
		if ( nPos != string::npos && ComparePathEq(nPos + 1, nCmpSize, szFullFilePath, 0, strlen(pszExt), pszExt) != false )
			return szFullFilePath.substr( 0, nPos );
		return  szFullFilePath;
	}
}

static int FindNextSlash( const string &szFullFilePath, const int nStartPos )
{
	for ( int i = nStartPos; i < szFullFilePath.size(); ++i )
	{
		if ( IsFolderSeparator(szFullFilePath[i]) )
			return i;
	}
	return string::npos;
}
void SplitPath( list<string> *pRes, const string &szFullFilePath )
{
	int nLastPos = 0;
	do
	{
		const int nPos = FindNextSlash( szFullFilePath, nLastPos );
		pRes->push_back( szFullFilePath.substr( nLastPos, nPos - nLastPos ) );
		nLastPos = nPos + 1;
	} while( nLastPos != (string::npos + 1) );
}

// ************************************************************************************************************************ //
// **
// ** comparison functions
// **
// **
// **
// ************************************************************************************************************************ //

__forceinline char ConvertCharASCII( const char chr )
{
	const char temp = chr - '\\';
	const char mask = (temp >> 7) | ((-temp) >> 7);
	const char chr1 = (chr & mask) | ('/' & (~mask));
	return chr1 - ( ('A' - 'a') & ( (('A' - chr1 - 1) & (chr1 - 'Z' - 1)) >> 7 ) );
}

bool ComparePathEq( const int nStart1, const int nLength1, const string &szPath1, 
									  const int nStart2, const int nLength2, const string &szPath2 )
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

bool ComparePathLt( const int nStart1, const int nLength1, const string &szPath1, 
									 const int nStart2, const int nLength2, const string &szPath2 )
{
	if ( nLength1 < nLength2 )
		return true;
	if ( &szPath1 == &szPath2 )
		return false;
	if ( nLength2 == 0 )
		return false;
	//
	const int nSize = Min( nLength1, nLength2 );
	const char *p1 = &( szPath1[nStart1] );
	const char *p2 = &( szPath2[nStart2] );
	for ( int i = 0; i < nSize; ++i )
	{
		if ( ConvertCharASCII(p1[i]) >= ConvertCharASCII(p2[i]) )
			return false;
	}
	return true;
}

bool IsPathRelative( const string &szPath )
{
	return !IsFolderSeparator(szPath[0]) && (szPath[1] != ':');
}

void MakeRelativePath( string *pRes, const string &szFullPath, const string &szParentPath )
{
	if ( szFullPath.empty() )
	{
		pRes->clear();
		return;
	}
	//
	const int nPos = FindLastSlash( szParentPath );
	if ( nPos != string::npos && ComparePathEq(0, nPos + 1, szParentPath, 0, nPos + 1, szFullPath) )
		*pRes = szFullPath.c_str() + nPos + 1;
	else
		*pRes = '/' + szFullPath;
}

void MakeFullPath( string *pRes, const string &szRelativePath, const string &szParentPath )
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
		const int nPos = FindLastSlash( szParentPath );
		if ( nPos != string::npos )
			*pRes = szParentPath.substr( 0, nPos ) + "/" + szRelativePath;
		else
			*pRes = szRelativePath;
	}
}

void NormalizePath( string *pRes, const string &szFilePath )
{
	const int nSize = szFilePath.size();
	pRes->resize( szFilePath.size() );
	for ( int i = 0; i < nSize; ++i )
		(*pRes)[i] = ConvertFolderSeparator( szFilePath[i] );
}

void AppendSlash( string *pFilePath, const char cSlash )
{
	if ( !pFilePath->empty() && !IsFolderSeparator((*pFilePath)[pFilePath->size() - 1]) ) 
		(*pFilePath) += cSlash;
}
void RemoveSlash( string *pFilePath, const char cSlash )
{
	if ( !pFilePath->empty() && !IsFolderSeparator((*pFilePath)[pFilePath->size() - 1]) ) 
		pFilePath->resize( pFilePath->size() - 1 );
}
void ConvertSlashes( string *pFilePath, const char cFrom, const char cTo )
{
	for ( string::iterator it = pFilePath->begin(); it != pFilePath->end(); ++it )
	{
		if ( *it == cFrom ) 
			*it = cTo;
	}
}

void CreatePath( const string &_szFullPath )
{	
	static char buffer[1024];
	string szFullPath = _szFullPath;
	NStr::ReplaceAllChars( &szFullPath, '/', '\\' );
	// remember current directory
	GetCurrentDirectory( 1024, buffer );
	// create entire path dir by dir
	string szDir;
	for ( NStr::CStringIterator<char> it(szFullPath, '\\'); !it.IsEnd(); it.Next() )
	{
		it.Get( &szDir );
		if ( !szDir.empty() ) 
		{
			CreateDirectory( szDir.c_str(), 0 );
			SetCurrentDirectory( (szDir + "\\").c_str() );
		}
		else
			break;
	}
	// restore old current directory
	SetCurrentDirectory( buffer );
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
	for ( string::const_iterator it = this->begin(); it != this->end(); ++it )
		uHashKey = 5*uHashKey + ConvertChar( *it );
	return uHashKey;
}

int CFilePath::operator&( IBinSaver &saver )
{
	saver.Add( 1, (string*)this );
	return 0;
}

}