#pragma once

#include "Parser_export.h"


#include "NodesList.h"

namespace NLang
{

class CNamespace;
class CTypeNode;
class CAttributeDefNode;
class CEnumEntryNode;
class CComplexTypeNode;

class PARSER_EXPORT CFileNode : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CFileNode );
	
	// The file's name as the parser knows it: its full path, lowercased. It keys
	// the includes maps and is what the rest of the pipeline sees.
	std::string szFullFileName;
	// The same path spelled as on disk, with '/' separators, which is what the
	// file is opened by. Kept apart from the name because only a filesystem
	// that ignores case can open the lowercased one.
	std::string szPathOnDisk;
	std::unordered_map< std::string, CObj<CFileNode> > includes;
	std::list<std::string> cppExternalIncludes;
	std::list<std::string> hExternalIncludes;

	bool bFileExist;
	enum EParseState
	{
		EPS_NOPARSED,
		EPS_INPARSING,
		EPS_PARSED,
	};
	EParseState eParseState;
	bool bIncludedInOtherFile;
	bool bRootFile;

	std::list< CObj<CNamespace> > namespaces;
public:
	CFileNode() : eParseState( EPS_NOPARSED ), bFileExist( false ), bIncludedInOtherFile( false ) { }
	CFileNode( const std::string &_szFullFileName, bool _bRootFile )
		: bFileExist( false ), eParseState( EPS_NOPARSED ), szFullFileName( _szFullFileName ), bIncludedInOtherFile( false ), bRootFile( _bRootFile ) { }
	void SetExist() { bFileExist = true; }
	void SetIncludedInOtherFile() { bIncludedInOtherFile = true; }
	bool IsIncludedInOtherFile() const { return bIncludedInOtherFile; }
	bool IsParsed() const { return eParseState == EPS_PARSED; }
	bool IsRootFile() const { return bRootFile; }

	const std::string& GetName() const { return szFullFileName; }
	void SetFullName( const std::string &_szFullName );
	// Falls back to the name for a node nothing has recorded a path for.
	const std::string& GetPathOnDisk() const { return szPathOnDisk.empty() ? szFullFileName : szPathOnDisk; }
	void SetPathOnDisk( const std::string &_szPathOnDisk ) { szPathOnDisk = _szPathOnDisk; }
	bool HasPathOnDisk() const { return !szPathOnDisk.empty(); }

	void AddInclude( std::string szFileName );
	void AddInclude( CFileNode *pNode );
	CFileNode* GetInclude( const std::string &szFileName );

	CNamespace* GetNamespace() const;
	void OpenNewNamespace( CNodesList<CComplexTypeNode> *pVisibleTypes );
	void CloseNamespace( bool bResolveForwards );

	CAttributeDefNode* FindAttrDef( const std::string &szAttrName );
	void AddAttrDef( CAttributeDefNode *pAttrDefNode );

	void AddHExternal( const std::string &szIncludeName );
	void AddCPPExternal( const std::string &szIncludeName );

	void AddDef( CLangNode *pNode );
	CLangNode* FindDef( const std::string &szTypeName, bool bOnlyTopNamespace );
	CTypeNode* FindForward( const std::string &szTypeName, bool bOnlyTopNamespace );

	CEnumEntryNode* FindEnumEntry( const std::string &szEntryName, bool bOnlyTopNamespace );

	void Parse();

	typedef std::unordered_map< std::string, CObj<CFileNode> >::const_iterator TIncludesIter;
	TIncludesIter BeginIncludes() const { return includes.begin(); }
	TIncludesIter EndIncludes() const { return includes.end(); }

	const std::list<std::string>& GetHExternalIncludes() const { return hExternalIncludes; }
	const std::list<std::string>& GetCPPExternalIncludes() const { return cppExternalIncludes; }
};

// exported for dbcodegen, which walks the parsed tree from here
PARSER_EXPORT CFileNode* GetRootFile();

void AddInclude( const std::string &szFileName );
void AddHExternal( const std::string &szIncludeName );
void AddCPPExternal( const std::string &szIncludeName );
CFileNode* GetCurFileNode();

}


