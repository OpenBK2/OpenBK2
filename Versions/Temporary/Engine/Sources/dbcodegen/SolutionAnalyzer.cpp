#include "stdafx.h"
#include "Errors.h"
#include "SolutionAnalyzer.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"

#include <fmt/format.h>

namespace NSlnAnalyzer
{

struct SXMLValue
{
	std::string szName, szValue;

	SXMLValue() {}
	SXMLValue( const char *_szName, const char *_szValue ) : szName(_szName), szValue(_szValue) {}
	SXMLValue( const std::string &_szName, const std::string &_szValue ) : szName(_szName), szValue(_szValue) {}
};
struct SXMLSection
{
	std::string szName;
	std::vector<SXMLValue> values;
	std::vector<SXMLSection> children;

	SXMLSection() {}
	SXMLSection( const char *_szName ) : szName(_szName) {}
	SXMLSection* GetSection( const char *pszName )
	{
		if ( this == 0 )
			return 0;
		for ( int k = 0; k < children.size(); ++k )
		{
			if ( strcmp( children[k].szName.c_str(), pszName ) == 0 )
				return &children[k];
		}
		return 0;
	}
	const SXMLSection* GetSection( const char *pszName ) const
	{
		for ( int k = 0; k < children.size(); ++k )
		{
			if ( strcmp( children[k].szName.c_str(), pszName ) == 0 )
				return &children[k];
		}
		return 0;
	}
	const char* GetValueSafe( const char *pszName ) const
	{
		for ( int k = 0; k < values.size(); ++k )
		{
			if ( strcmp( values[k].szName.c_str(), pszName ) == 0 )
				return values[k].szValue.c_str();
		}
		return ""; //0;
	}
	SXMLSection& operator+=( const SXMLValue &v ) { values.push_back( v ); return *this; }
	SXMLSection& operator+=( const SXMLSection &v ) { children.push_back( v ); return *this; }
};
inline SXMLSection operator+( const SXMLSection &a, const SXMLSection &b ) 
{
	SXMLSection res(a);
	res += b;
	return res;
}
inline SXMLSection operator+( const SXMLSection &a, const SXMLValue &b ) 
{
	SXMLSection res(a);
	res += b;
	return res;
}

struct SSection
{
	std::string szName, szParam;
	std::vector<std::string> values;
	std::vector<SSection> children;
	bool bHasEndMark;

	SSection() : bHasEndMark(false) {}
};

static const char *LoadSection( const char *pszData, std::vector<SSection> *pRes, int _nShift )
{
	for(;*pszData;)
	{
		int nShift = 0;
		const char *pszLine = pszData;
		for ( ; *pszData != 0 && *pszData == '\t'; ++nShift, ++pszData );
		if ( nShift < _nShift )
			return pszLine;
		SSection s;
		//form s
		std::string szLine;
		// read line; stop at LF too, the checkout may have converted the CRLF
		// the original only expected
		for ( ;*pszData && *pszData != 0xd && *pszData != 0xa; ++pszData )
		{
			szLine += *pszData;
		}
		while ( *pszData == 0xd || *pszData == 0xa )
			++pszData;

		// parse line
		enum EState
		{
			Start,
			Param,
			Val,
			ValQuot
		} state = Start;
		std::string szVal;
		for ( int k = 0; k < szLine.size(); ++k )
		{
			switch ( state )//szLine[k] )
			{
			case Start:
				if ( szLine[k] == '(' )
				{
					state = Param;
				}
				else if ( szLine[k] == ' ' && szLine[k+1] == '=' && szLine[k+2] == ' ' )
				{
					state = Val;
					k += 2;
				}
				else
					s.szName += szLine[k];
				break;
			case Param:
				if ( szLine[k] == ')' )
				{
					state = Start;
				}
				else
					s.szParam += szLine[k];
				break;
			case Val:
				if ( szLine[k] == '\"' )
				{
					state = ValQuot;
				}
				else if ( szLine[k] == ',' )
				{
					s.values.push_back( szVal );
					szVal = "";
				}
				else if ( szLine[k] == ' ' )
					break;
				else
					szVal += szLine[k];
				break;
			case ValQuot:
				if ( szLine[k] == '\"' )
				{
					szVal = "\"" + szVal + "\"";
					state = Val;
				}
				else
					szVal += szLine[k];
				break;
			}
		}
		if ( state == Val )
			s.values.push_back( szVal );
		// load subsections
		pszData = LoadSection( pszData, &s.children, _nShift + 1 );
		// check if its closing statement
		if ( !pRes->empty() && s.szName == "End" + pRes->back().szName )
		{
			pRes->back().bHasEndMark = true;
			continue;
		}
		pRes->push_back( s );
	}
	return pszData;
}

// The header is matched without its line ending, then either CRLF or LF is
// skipped: git checks the .sln files out with LF, and the original compared
// against "...8.00\r\n" and so rejected every solution in the tree.
const char *pszSlnHeader = "Microsoft Visual Studio Solution File, Format Version 8.00";
static void LoadSln( const char *pszData, std::vector<SSection> *pRes, const std::string &szSlnFile )
{
	pRes->resize(0);
	int nHdrSize = strlen( pszSlnHeader );
	if ( strncmp( pszData, pszSlnHeader, nHdrSize ) != 0 )
		throw CCodeGenException( fmt::format( "unknown solution format, file {}", szSlnFile ) );

	pszData += nHdrSize;
	while ( *pszData == 0xd || *pszData == 0xa )
	{
		++pszData;
	}
	while ( *pszData )
		pszData = LoadSection( pszData, pRes, 0 );
}

struct SProject
{
	std::string szName, szGUID;
	std::vector<std::string> depends; // guids
};
static void CollectProjects( const std::vector<SSection> &sln, std::vector<SProject> *pRes )
{
	for ( int k = 0; k < sln.size(); ++k )
	{
		const SSection &s = sln[k];
		if ( s.szName == "Project" )
		{
			SProject proj;
			proj.szName = s.values[0];
			proj.szName = proj.szName.substr( 1, proj.szName.size() - 2 );
			proj.szGUID = s.values[2];
			proj.szGUID = proj.szGUID.substr( 1, proj.szGUID.size() - 2 );
			for ( int k = 0; k < s.children.size(); ++k )
			{
				if ( s.children[k].szName == "ProjectSection" && s.children[k].szParam == "ProjectDependencies" )
				{
					const SSection &dep = s.children[k];
					for ( int i = 0; i < dep.children.size(); ++i )
						proj.depends.push_back( dep.children[i].szName );
				}
			}
			pRes->push_back( proj );
		}
	}
}

static void CollectProjects( const std::string &szSlnName, const std::string &szBasePath, std::vector<SProject> *pProjects )
{
	const std::string szSlnFile = szBasePath + szSlnName + ".sln";

	CMemoryStream ms;
	{
		CFileStream stream( szSlnFile, CFileStream::WIN_READ_ONLY );
		ms.SetSize( stream.GetSize() );
		stream.Read( ms.GetBufferForWrite(), ms.GetSize() );
		ms.Seek( ms.GetSize() );
		char cZero = 0;
		ms.Write( &cZero, 1 );
	}

	std::vector<SSection> sections;
	LoadSln( (const char*)ms.GetBuffer(), &sections, szSlnFile );

	CollectProjects( sections, pProjects );
}

void GetProjectsOfSln( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pProjects )
{
	std::vector<SProject> projects;
	CollectProjects( szSlnName, szBasePath, &projects );

	pProjects->resize( projects.size() );
	for ( int i = 0; i < projects.size(); ++i )
		(*pProjects)[i] = projects[i].szName;
}

static const char *SkipSpaces( const char *p )
{
	while ( *p && isspace(*p) )
		++p;
	return p;
}
static const char *ReadName( const char *p, std::string *pRes )
{
	*pRes = "";
	if ( *p == '"' )
	{
		++p;
		while ( *p && *p != '"' )
			*pRes += *p++;
		if ( *p )
			++p;
		return p;
	}
	while ( *p != 0 && (isalnum( *p ) || *p == '_' ) )
	{
		*pRes += *p++;
	}
	return p;
}

static const char *LoadXMLStatement( SXMLSection *pRes, const char *pszData )
{
	pszData = SkipSpaces( pszData );
	if ( *pszData != '<' )
		return pszData;
	++pszData;
	bool bIsFinal = false;
	if ( *pszData == '/' )
	{
		bIsFinal = true;
		++pszData;
	}
	pszData = ReadName( pszData, &pRes->szName );
	for(; *pszData;)
	{
		pszData = SkipSpaces( pszData );
		if ( *pszData == '>' )
		{
			++pszData;
			break;
		}
		if ( *pszData == '/' && pszData[1] == '>' )
		{
			return pszData + 2;
		}
		SXMLValue val;
		pszData = ReadName( pszData, &val.szName );
		if ( *pszData != '=' )
			continue;
		++pszData;
		pszData = ReadName( pszData, &val.szValue );
		pRes->values.push_back( val );
	}
	if ( bIsFinal )
	{
		pRes->szName = std::string("/") + pRes->szName;
		return pszData;
	}
	while ( *pszData )
	{
		SXMLSection s;
		pszData = LoadXMLStatement( &s, pszData );
		if ( s.szName == std::string( "/" ) + pRes->szName )
			return pszData;
		pRes->children.push_back( s );
	}
	return pszData;
}

const char *szXMLHeader = "<?xml version=\"1.0\" encoding=\"windows-1251\"?>";

static bool LoadXML( SXMLSection *pRes, const char *pszData )
{
	int nXMLHeaderSize = strlen(szXMLHeader);
	if ( memcmp( pszData, szXMLHeader, nXMLHeaderSize ) != 0 )
		return false;
	pszData += nXMLHeaderSize;
	while( *pszData )
	{
		SXMLSection s;
		pszData = LoadXMLStatement( &s, pszData );
		if ( s.szName != "" )
			pRes->children.push_back( s );
	}
	return true;
}

static void CollectFiles( SXMLSection *pRes, std::vector<std::string> *pFiles )
{
	if ( !pRes )
		return;
	for ( int k = 0; k < pRes->children.size(); ++k )
	{
		if ( pRes->children[k].szName == "Filter" )
			CollectFiles( &pRes->children[k], pFiles );
		else if ( pRes->children[k].szName == "File" )
		{
			SXMLSection &s = pRes->children[k];
			pFiles->push_back( s.GetValueSafe( "RelativePath" ) );
		}
	}
}

struct SFilesAdder
{
	std::vector<std::string> *pFiles;

	SFilesAdder( std::vector<std::string> *_pFiles ) : pFiles( _pFiles ) { }
	void operator()( const NFile::CFileIterator &iter ) const
	{
		if ( !iter.IsDirectory() )
		{
			std::string szFileName = iter.GetFullName();
			NFile::NormalizePath( &szFileName );
			NStr::ToLowerASCII( &szFileName );
			pFiles->push_back( szFileName );
		}
	}
};

static void GetAllFilesOfDescProj( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pFiles )
{
	std::vector<std::string> projects;
	GetProjectsOfSln( szSlnName, szBasePath, &projects );

	for ( int i = 0; i < projects.size(); ++i )
	{
		const std::string vcProjName = projects[i];

		CMemoryStream ms;
		{
			const std::string szProjName = szBasePath + vcProjName + "/" + vcProjName + ".vcproj";
			CFileStream stream( szProjName, CFileStream::WIN_READ_ONLY );
			ms.SetSize( stream.GetSize() );
			stream.Read( ms.GetBufferForWrite(), ms.GetSize() );
			ms.Seek( ms.GetSize() );
			char cZero = 0;
			ms.Write( &cZero, 1 );
		}

		SXMLSection proj;
		if ( !LoadXML( &proj, (const char*)ms.GetBuffer() ) )
			return;
		SXMLSection *pProj = proj.GetSection( "VisualStudioProject" );

		std::vector<std::string> files;
		CollectFiles( pProj->GetSection( "Files" ), &files );
		for ( int i = 0; i < files.size(); ++i )
		{
			if ( NFile::GetFileExt( files[i] ) == ".cll" )
			{
				std::string szRelPath = files[i];
				std::string szFullPath = szBasePath + vcProjName + "/" + szRelPath.substr( 2, std::string::npos );
				NFile::NormalizePath( &szFullPath );
				NStr::ToLowerASCII( &szFullPath );
				pFiles->push_back( szFullPath );
			}
		}
	}
}

static void GetAllSlnFilesFromDisc( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pFiles )
{
	std::vector<std::string> projects;
	GetProjectsOfSln( szSlnName, szBasePath, &projects );

	std::string szBaseFilesPathWND( szBasePath );
	NStr::ReplaceAllChars( &szBaseFilesPathWND, '/', '\\' );
	for ( int i = 0; i < projects.size(); ++i )
	{
		std::string szPath = szBaseFilesPathWND + projects[i] + "\\";
		NFile::EnumerateFiles( szPath, "*.cll", SFilesAdder( pFiles ), true );
	}
}

void GetTypesDescriptorsOfSln( const std::string &szSlnName, const std::string &szBasePath, std::vector<std::string> *pFiles )
{
	std::vector<std::string> allDBFiles;
	GetAllFilesOfDescProj( szSlnName, szBasePath, &allDBFiles );

	std::vector<std::string> neededFiles;
	GetAllSlnFilesFromDisc( szSlnName, szBasePath, &neededFiles );

	std::sort( allDBFiles.begin(), allDBFiles.end() );
	std::sort( neededFiles.begin(), neededFiles.end() );

	int nAll = 0;
	int nNeed = 0;
	while ( nAll < allDBFiles.size() && nNeed < neededFiles.size() )
	{
		const int nCompare = strcmp( allDBFiles[nAll].c_str(), neededFiles[nNeed].c_str() );
		switch ( nCompare )
		{
			case -1: 
				++nAll;
				break;
			case 0: 
				pFiles->push_back( allDBFiles[nAll] );
				++nAll;
				++nNeed;
				break;
			case 1: 
				++nNeed;
				break;
		}
	}

	pFiles->push_back( szBasePath + "base.cll" );
	pFiles->push_back( szBasePath + "game.cll" );
}

}


