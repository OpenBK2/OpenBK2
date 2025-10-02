#include "stdafx.h"

#include "CodeGenFile.h"
#include "CodeGenNamespace.h"
#include "StrStream.h"
#include "../System/FilePath.h"
#include "../Misc/StrProc.h"
#include "../Parser/FileNode.h"
#include "../System/XmlSaver.h"

namespace NCodeGen
{

static const string CutRootDir( const string &szFileName, const string &szRootDir )
{
	return szFileName.substr( szRootDir.size(), szFileName.size() );
}

static const string GetIncludeRefName( const vector<string> &splittedFileDirs, const string &szRootDir, const string &szFullIncludeName )
{
	const string szIncludeName = CutRootDir( szFullIncludeName, szRootDir );
	vector<string> inclDirs;
	NStr::SplitString( szIncludeName, &inclDirs, '/' );
	const string szInclFileName = inclDirs.back();
	if ( szInclFileName != "base.h" && szInclFileName != "game.h" )
	{
		inclDirs.pop_back();

		string szRefIncludeName = "";
		int nFirstNotEqualDir = 0;
		while ( nFirstNotEqualDir < splittedFileDirs.size() && nFirstNotEqualDir < inclDirs.size() && splittedFileDirs[nFirstNotEqualDir] == inclDirs[nFirstNotEqualDir] )
			++nFirstNotEqualDir;
		for ( int i = nFirstNotEqualDir; i < splittedFileDirs.size(); ++i )
			szRefIncludeName += "../";
		for ( int i = nFirstNotEqualDir; i < inclDirs.size(); ++i )
			szRefIncludeName += inclDirs[i] + "/";
		szRefIncludeName += szInclFileName;

		return szRefIncludeName;
	}

	return "";
}

CFile::CFile( NLang::CFileNode *pFileNode, const CNodes2TypeDefs &nodes2TypeDefs, const string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	szName = CutRootDir( pFileNode->GetName(), szRootDir );
	szName = szName.substr( 0, szName.size() - NFile::GetFileExt( szName ).size() );
	vector<string> dirs;
	NStr::SplitString( szName, &dirs, '/' );
	dirs.pop_back();
	for ( NLang::CFileNode::TIncludesIter iter = pFileNode->BeginIncludes(); iter != pFileNode->EndIncludes(); ++iter )
	{
		string szIncludeRefName = GetIncludeRefName( dirs, szRootDir, iter->first );
		const string szExt = NFile::GetFileExt( szIncludeRefName );
		if ( szExt == ".cll" )
			szIncludeRefName = szIncludeRefName.substr( 0, szIncludeRefName.size() - szExt.size() ) + ".h";
		if ( !szIncludeRefName.empty() && szIncludeRefName != "../base.h" && szIncludeRefName != "../game.h" )
			includes.push_back( szIncludeRefName );
	}

	sort( includes.begin(), includes.end() );

	hExternalIncludes = pFileNode->GetHExternalIncludes();
	cppExternalIncludes = pFileNode->GetCPPExternalIncludes();

	pNamespace = new CNamespace( pFileNode->GetNamespace(), nodes2TypeDefs, pTermTypesDesc );
}

void CFile::GenerateCode( const string &szRootDir )
{
	const string szFullHFileName = szRootDir + szName + ".h";
	CFileStream hStream( szFullHFileName, CFileStream::WIN_CREATE);
	const string szFullCppFileName = szRootDir + szName + ".cpp";
	CFileStream cppStream( szFullCppFileName, CFileStream::WIN_CREATE );

	if ( hStream.IsOk() && cppStream.IsOk() )
	{
		string szHFile, szCPPFile, szEOF, szCPPEOF;
		ICode::SCodeStreams code( &szHFile, &szCPPFile, &szEOF, &szCPPEOF );

		code.h << "#pragma once" << endl;
		code.h << separator;
		code.h << "// automatically generated file, don't change manually!" << endl << endl;

		for ( list<string>::iterator iter = includes.begin(); iter != includes.end(); ++iter )
			code.h <<  "#include " << qcomma << *iter << qcomma << endl;
		for ( list<string>::iterator iter = hExternalIncludes.begin(); iter != hExternalIncludes.end(); ++iter )
			code.h << "#include " << qcomma << *iter << qcomma << endl;
		code.h << separator;
		code.h << "interface IXmlSaver;" << endl;
		code.h << separator;

		int i = szFullHFileName.size() - 1;
		string szShortHFileName = "";
		while ( szFullHFileName[i] != '/' && i >= 0 )
		{
			szShortHFileName = szFullHFileName[i] + szShortHFileName;
			--i;
		}
		code.cpp << "// automatically generated file, don't change manually!" << endl << endl;
		code.cpp << "#include " << qcomma << "stdafx.h" << qcomma << endl;
		code.cpp << "#include " << qcomma << "../libdb/ReportMetaInfo.h" << qcomma << endl;
		code.cpp << "#include " << qcomma << "../libdb/Checksum.h" << qcomma << endl;
		code.cpp << "#include " << qcomma << "../System/XmlSaver.h" << qcomma << endl;
		code.cpp << "#include " << qcomma << szShortHFileName << qcomma << endl;
		for ( list<string>::iterator iter = cppExternalIncludes.begin(); iter != cppExternalIncludes.end(); ++iter )
			code.cpp << "#include " << qcomma << *iter << qcomma << endl;
		code.cpp << separator;
		code.cpp << "namespace NDb" << endl;
		code.cpp << "{" << endl;
		code.cpp << separator;

		code.cppEOF << "using namespace NDb;" << endl;

		code.h << "namespace NDb" << endl;
		code.h << "{" << endl;
		pNamespace->GenerateCode( &code, "", 0, "NDb" );
		code.h  << "}" << endl;

		code.cpp << "}" << endl;

		hStream.Write( szHFile.c_str(), szHFile.size() );
		hStream.Write( szEOF.c_str(), szEOF.size() );
		cppStream.Write( szCPPFile.c_str(), szCPPFile.size() );
		cppStream.Write( szCPPEOF.c_str(), szCPPEOF.size() );
	}
}

int CFile::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Includes", &includes );
	saver.Add( "hExternalIncludes", &hExternalIncludes );
	saver.Add( "cppExternalIncludes", &cppExternalIncludes );
	saver.Add( "Namespace", &pNamespace );
	return 0;
}

}

using namespace NCodeGen;
REGISTER_SAVELOAD_CLASS( 0x301B6D05, CFile );
