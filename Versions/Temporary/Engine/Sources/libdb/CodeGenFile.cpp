#include "stdafx.h"

#include "CodeGenFile.h"
#include "CodeGenNamespace.h"
#include "StrStream.h"
#include "System/FilePath.h"
#include "Misc/StrProc.h"
#include "Parser/FileNode.h"
#include "System/XmlSaver.h"

#include "libdb_export.h"

namespace NCodeGen
{

static const std::string CutRootDir( const std::string &szFileName, const std::string &szRootDir )
{
	return szFileName.substr( szRootDir.size(), szFileName.size() );
}

static const std::string GetIncludeRefName( const std::vector<std::string> &splittedFileDirs, const std::string &szRootDir, const std::string &szFullIncludeName )
{
	const std::string szIncludeName = CutRootDir( szFullIncludeName, szRootDir );
	std::vector<std::string> inclDirs;
	NStr::SplitString( szIncludeName, &inclDirs, '/' );
	const std::string szInclFileName = inclDirs.back();
	if ( szInclFileName != "base.h" && szInclFileName != "game.h" )
	{
		inclDirs.pop_back();

		// A header in the including file's own directory is named bare, any other
		// by its path from the root of the sources, which every module has on its
		// include path: "Stats_B2_M1/RPGStats.h", not "../Stats_B2_M1/RPGStats.h".
		// That is how the tree's DB sources, and the rest of the port, spell them.
		if ( inclDirs == splittedFileDirs )
		{
			return szInclFileName;
		}
		std::string szRefIncludeName = "";
		for ( int i = 0; i < inclDirs.size(); ++i )
			szRefIncludeName += inclDirs[i] + "/";
		szRefIncludeName += szInclFileName;

		return szRefIncludeName;
	}

	return "";
}

CFile::CFile( NLang::CFileNode *pFileNode, const CNodes2TypeDefs &nodes2TypeDefs, const std::string &szRootDir, NDb::NTypeDef::CTerminalTypesDescriptor *pTermTypesDesc )
{
	// The output path, and the includes of other generated headers below, come
	// from the files' paths on disk rather than their lowercased names, so the
	// generated files land in Stats_B2_M1/ and include "Stats_B2_M1/RPGStats.h",
	// both of which only resolve as lowercase where the filesystem ignores case.
	// The directories and the include paths have to change together: they are
	// compared to find the directories the two files share.
	szName = CutRootDir( pFileNode->GetPathOnDisk(), szRootDir );
	szName = szName.substr( 0, szName.size() - NFile::GetFileExt( szName ).size() );
	std::vector<std::string> dirs;
	NStr::SplitString( szName, &dirs, '/' );
	dirs.pop_back();
	for ( NLang::CFileNode::TIncludesIter iter = pFileNode->BeginIncludes(); iter != pFileNode->EndIncludes(); ++iter )
	{
		std::string szIncludeRefName = GetIncludeRefName( dirs, szRootDir, iter->second->GetPathOnDisk() );
		const std::string szExt = NFile::GetFileExt( szIncludeRefName );
		if ( szExt == ".cll" )
			szIncludeRefName = szIncludeRefName.substr( 0, szIncludeRefName.size() - szExt.size() ) + ".h";
		// base.cll and game.cll, at the root, generate nothing to include; from a
		// module directory their paths are now the bare root-relative names
		if ( !szIncludeRefName.empty() && szIncludeRefName != "base.h" && szIncludeRefName != "game.h" )
			includes.push_back( szIncludeRefName );
	}

	// Ignoring case, the order they had when they were all lowercase and the
	// order the tree's DB headers list them in.
	includes.sort( []( const std::string &a, const std::string &b )
	{
		std::string szA( a ), szB( b );
		NStr::ToLowerASCII( &szA );
		NStr::ToLowerASCII( &szB );
		return szA < szB;
	} );

	hExternalIncludes = pFileNode->GetHExternalIncludes();
	cppExternalIncludes = pFileNode->GetCPPExternalIncludes();

	pNamespace = new CNamespace( pFileNode->GetNamespace(), nodes2TypeDefs, pTermTypesDesc );
}

void CFile::GenerateCode( const std::string &szRootDir )
{
	const std::string szFullHFileName = szRootDir + szName + ".h";
	CFileStream hStream( szFullHFileName, CFileStream::WIN_CREATE);
	const std::string szFullCppFileName = szRootDir + szName + ".cpp";
	CFileStream cppStream( szFullCppFileName, CFileStream::WIN_CREATE );

	if ( hStream.IsOk() && cppStream.IsOk() )
	{
		std::string szHFile, szCPPFile, szEOF, szCPPEOF;
		ICode::SCodeStreams code( &szHFile, &szCPPFile, &szEOF, &szCPPEOF );
		// the module is the first directory of the file's path under the root;
		// base.cll and game.cll, at the root itself, belong to none
		const std::string::size_type nSlash = szName.find( '/' );
		if ( nSlash != std::string::npos )
		{
			code.szModule = szName.substr( 0, nSlash );
		}

		// The bodies first: which includes the files need depends on them.
		code.cpp << "namespace NDb" << endl;
		code.cpp << "{" << endl;
		code.cpp << separator;

		code.cppEOF << "using namespace NDb;" << endl;

		code.h << "namespace NDb" << endl;
		code.h << "{" << endl;
		pNamespace->GenerateCode( &code, "", 0, "NDb" );
		code.h  << "}" << endl;

		code.cpp << "}" << endl;

		// The module's export header, where its export macro is used: by a type
		// or enum marked [export] in the header, and in the .cpp by the
		// registration macros, which expand to it.
		const std::string szExportHeader = code.szModule + "_export.h";
		const bool bHUsesExport = !code.szModule.empty() &&
			( szHFile.find( code.GetModuleMacroName() + "_EXPORT" ) != std::string::npos ||
			  szEOF.find( code.GetModuleMacroName() + "_EXPORT" ) != std::string::npos );
		const bool bCppUsesExport = !code.szModule.empty() && szCPPEOF.find( "REGISTER_DATABASE_CLASS(" ) != std::string::npos;
		// GUID fields: the uuid type in the header, AddUuidChunk in the .cpp
		const bool bHUsesUuid = szHFile.find( "boost::uuids::uuid" ) != std::string::npos ||
			szEOF.find( "boost::uuids::uuid" ) != std::string::npos;
		const bool bCppUsesUuid = szCPPFile.find( "AddUuidChunk(" ) != std::string::npos;

		// The include blocks, laid out as the tree's DB sources lay them out:
		// the export header, then the other quoted includes, then <cstdint>,
		// which the generated code uses throughout, each block followed by an
		// empty line.
		std::string szHHead, szCppHead;
		CStrStream h( &szHHead ), cpp( &szCppHead );
		h << "#pragma once" << endl;
		h << separator;
		h << "// automatically generated file, don't change manually!" << endl << endl;
		if ( bHUsesExport )
		{
			h << "#include " << qcomma << szExportHeader << qcomma << endl << endl;
		}
		if ( !includes.empty() || !hExternalIncludes.empty() )
		{
			for ( std::list<std::string>::iterator iter = includes.begin(); iter != includes.end(); ++iter )
				h << "#include " << qcomma << *iter << qcomma << endl;
			for ( std::list<std::string>::iterator iter = hExternalIncludes.begin(); iter != hExternalIncludes.end(); ++iter )
				h << "#include " << qcomma << *iter << qcomma << endl;
			h << endl;
		}
		h << "#include <cstdint>" << endl;
		if ( bHUsesUuid )
		{
			h << endl << "#include <boost/uuid/uuid.hpp>" << endl;
		}
		h << separator;
		h << "struct IXmlSaver;" << endl;
		h << separator;

		int i = szFullHFileName.size() - 1;
		std::string szShortHFileName = "";
		while ( szFullHFileName[i] != '/' && i >= 0 )
		{
			szShortHFileName = szFullHFileName[i] + szShortHFileName;
			--i;
		}
		cpp << "// automatically generated file, don't change manually!" << endl << endl;
		cpp << "#include " << qcomma << "stdafx.h" << qcomma << endl;
		cpp << "#include " << qcomma << "libdb/ReportMetaInfo.h" << qcomma << endl;
		cpp << "#include " << qcomma << "libdb/Checksum.h" << qcomma << endl;
		cpp << "#include " << qcomma << "System/XmlSaver.h" << qcomma << endl;
		cpp << "#include " << qcomma << szShortHFileName << qcomma << endl;
		if ( bCppUsesUuid )
		{
			cpp << "#include " << qcomma << "System/UuidChunk.h" << qcomma << endl;
		}
		for ( std::list<std::string>::iterator iter = cppExternalIncludes.begin(); iter != cppExternalIncludes.end(); ++iter )
			cpp << "#include " << qcomma << *iter << qcomma << endl;
		cpp << endl;
		if ( bCppUsesExport )
		{
			cpp << "#include " << qcomma << szExportHeader << qcomma << endl << endl;
		}
		cpp << "#include <cstdint>" << endl;
		cpp << separator;

		hStream.Write( szHHead.c_str(), szHHead.size() );
		hStream.Write( szHFile.c_str(), szHFile.size() );
		hStream.Write( szEOF.c_str(), szEOF.size() );
		cppStream.Write( szCppHead.c_str(), szCppHead.size() );
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
REGISTER_SAVELOAD_CLASS( LIBDB, 0x301B6D05, CFile );

