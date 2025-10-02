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
	
	string szFullFileName;
	hash_map< string, CObj<CFileNode> > includes;
	list<string> cppExternalIncludes;
	list<string> hExternalIncludes;

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

	list< CObj<CNamespace> > namespaces;
public:
	CFileNode() : eParseState( EPS_NOPARSED ), bFileExist( false ), bIncludedInOtherFile( false ) { }
	CFileNode( const string &_szFullFileName, bool _bRootFile )
		: bFileExist( false ), eParseState( EPS_NOPARSED ), szFullFileName( _szFullFileName ), bIncludedInOtherFile( false ), bRootFile( _bRootFile ) { }
	void SetExist() { bFileExist = true; }
	void SetIncludedInOtherFile() { bIncludedInOtherFile = true; }
	bool IsIncludedInOtherFile() const { return bIncludedInOtherFile; }
	bool IsParsed() const { return eParseState == EPS_PARSED; }
	bool IsRootFile() const { return bRootFile; }

	const string& GetName() const { return szFullFileName; }
	void SetFullName( const string &_szFullName );

	void AddInclude( string szFileName );
	void AddInclude( CFileNode *pNode );
	CFileNode* GetInclude( const string &szFileName );

	CNamespace* GetNamespace() const;
	void OpenNewNamespace( CNodesList<CComplexTypeNode> *pVisibleTypes );
	void CloseNamespace( bool bResolveForwards );

	CAttributeDefNode* FindAttrDef( const string &szAttrName );
	void AddAttrDef( CAttributeDefNode *pAttrDefNode );

	void AddHExternal( const string &szIncludeName );
	void AddCPPExternal( const string &szIncludeName );

	void AddDef( CLangNode *pNode );
	CLangNode* FindDef( const string &szTypeName, bool bOnlyTopNamespace );
	CTypeNode* FindForward( const string &szTypeName, bool bOnlyTopNamespace );

	CEnumEntryNode* FindEnumEntry( const string &szEntryName, bool bOnlyTopNamespace );

	void Parse();

	typedef hash_map< string, CObj<CFileNode> >::const_iterator TIncludesIter;
	TIncludesIter BeginIncludes() const { return includes.begin(); }
	TIncludesIter EndIncludes() const { return includes.end(); }

	const list<string>& GetHExternalIncludes() const { return hExternalIncludes; }
	const list<string>& GetCPPExternalIncludes() const { return cppExternalIncludes; }
};

CFileNode* GetRootFile();

void AddInclude( const string &szFileName );
void AddHExternal( const string &szIncludeName );
void AddCPPExternal( const string &szIncludeName );
CFileNode* GetCurFileNode();

}

