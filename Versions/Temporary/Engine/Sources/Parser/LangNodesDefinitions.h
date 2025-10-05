#pragma once

#include "Parser_export.h"


#include "LangNode.h"
#include "NodesList.h"
#include "Visitor.h"

namespace NLang
{

class CTypeNode : public CLangNode
{
	bool bForward;
	CPtr<CTypeNode> pRealTypeIfForward;
public:
	CTypeNode() : bForward( false ) { }
	CTypeNode( const std::string &szName, const std::string &szFile, int nLine ) : CLangNode( szName, szFile, nLine ), bForward( false ) { }
	CTypeNode( const std::string &szName, bool _bForward, const std::string &szFile, int nLine ) : CLangNode( szName, szFile, nLine ), bForward( _bForward ) { }

	bool IsForward() const { return bForward; }
	CTypeNode* GetRealType() { return bForward ? pRealTypeIfForward : this; }
	void SetRealType( CTypeNode *pRealType );
};

class CAttributeNode : public CLangNode
{
	OBJECT_NOCOPY_METHODS( CAttributeNode );
	CSimpleValue value;
public:
	CAttributeNode() { }
	CAttributeNode( const std::string &szName, const std::string &szValue, bool bStringValue, const std::string &szFile, int nLine )
		: CLangNode( szName, szFile, nLine ), value( szValue, bStringValue ) { }

	ESimpleType GetType() const { return value.GetType(); }
	const CSimpleValue& GetValue() const { return value; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class PARSER_EXPORT CTypeDefNode : public CTypeNode
{
	OBJECT_NOCOPY_METHODS( CTypeDefNode );

	CPtr<CTypeNode> pReferencedType;
	bool bPointer;
	CPtr< CNodesList<CAttributeNode> > pAttrList;
public:
	CTypeDefNode() : bPointer( false ) { }
	CTypeDefNode( const std::string &szName, CNodesList<CAttributeNode> *_pAttrList, CTypeNode *_pReferencedType, bool _bPointer, const std::string &szFile, int nLine )
		: CTypeNode( szName, szFile, nLine ), pAttrList( _pAttrList ), pReferencedType( _pReferencedType ), bPointer( _bPointer ) { }

	CTypeNode* GetReferencedType( bool bRecursive ) const;
	bool IsPointer() const;

	typedef std::list< CPtr<CAttributeNode> >::const_iterator TAttrIter;
	TAttrIter AttrBegin() const;
	TAttrIter AttrEnd() const;

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class CEnumEntryNode : public CLangNode
{
	OBJECT_NOCOPY_METHODS( CEnumEntryNode )

	int nValue;
	bool bValueDefined;
public:
	CEnumEntryNode() : nValue( -1 ), bValueDefined( false ) { }
	CEnumEntryNode( const std::string &szName, int _nValue, const std::string &szFile, int nLine )
		: CLangNode( szName, szFile, nLine ), nValue( _nValue ), bValueDefined( true ) { }
	CEnumEntryNode( const std::string &szName, const std::string &szFile, int nLine )
		: CLangNode( szName, szFile, nLine ), nValue( -1 ), bValueDefined( false ) { }

	bool IsValueDefined() const { return bValueDefined; }
	int GetValue() const { return nValue; }

	void SetValue( const int _nValue ) { nValue = _nValue; bValueDefined = true; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class PARSER_EXPORT CEnumNode : public CTypeNode
{
	OBJECT_NOCOPY_METHODS( CEnumNode );

	CPtr< CNodesList<CEnumEntryNode> > pEntriesList;
	CPtr< CNodesList<CAttributeNode> > pAttrList;
public:
	CEnumNode() { }
	CEnumNode( const std::string &szFile, int nLine )
		: CTypeNode( "", szFile, nLine ) { pEntriesList = new CNodesList<CEnumEntryNode>(); }
	CEnumNode( const std::string &szName, bool bForward, const std::string &szFile, int nLine )
		: CTypeNode( szName, bForward, szFile, nLine ) { pEntriesList = new CNodesList<CEnumEntryNode>(); }

	void AddAttributes( CNodesList<CAttributeNode> *_pAttrList ) { pAttrList = _pAttrList; }

	typedef std::list< CPtr<CEnumEntryNode> >::const_iterator TEntriesIter;
	TEntriesIter EntriesBegin() const;
	TEntriesIter EntriesEnd() const;

	typedef std::list< CPtr<CAttributeNode> >::const_iterator TAttrIter;
	TAttrIter AttrBegin() const;
	TAttrIter AttrEnd() const;

	void AddEnumEntry( CEnumEntryNode *pEnumEntryNode ) { pEntriesList->GetNodes().push_back( pEnumEntryNode ); }

	CEnumEntryNode* GetEnumEntry( const std::string &szEntryName ) const;
	CEnumEntryNode* FindAnyWithCrossedEntries( class CNamespace *pNM ) const;

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class PARSER_EXPORT CVariable : public CLangNode
{
	CPtr<CTypeNode> pType;
	CPtr< CNodesList<CAttributeNode> > pAttrList;
	bool bIsPointer;

	bool bHasDefault;
	CSimpleValue defaultValue;

	bool bHasComplexDefault;
	std::string szComplexDefaultValue;
public:
	CVariable() : bIsPointer( false ), bHasDefault( false ), bHasComplexDefault( false ) { }
	CVariable( const std::string &szName, const std::string &szFile, int nLine )
		: CLangNode( szName, szFile, nLine ), bIsPointer( false ), bHasDefault( false ), bHasComplexDefault( false ) { }

	void SetVarToPointer() { bIsPointer = true; }
	bool IsPointer() const { return bIsPointer; }

	void SetComplexDefault( const std::string &szValue )
	{
		bHasComplexDefault = true;
		szComplexDefaultValue = szValue;
	}

	void SetDefault( const std::string &szValue, bool bString )
	{
		bHasDefault = true;
		defaultValue.SetValue( szValue, bString );
	}

	void SetDefaultEnum( const std::string &szValue )
	{
		bHasDefault = true;
		defaultValue.SetToEnum( szValue );
	};

	void SetDefaultWStr( const std::string &szValue )
	{
		bHasDefault = true;
		defaultValue.SetWStrValue( szValue );
	}

	bool HasDefault() const { return bHasDefault; }
	ESimpleType GetTypeOfDefault() const { return defaultValue.GetType(); }
	const CSimpleValue& GetDefault() const { return defaultValue; }

	bool HasComplexDefault() const { return bHasComplexDefault; }
	const std::string& GetComplexDefault() const { return szComplexDefaultValue; }

	void SetAttrList( CNodesList<CAttributeNode> *_pAttrList ) { pAttrList = _pAttrList; }
	void SetType( CTypeNode *_pType ) { pType = _pType; }

	CTypeNode* GetType() const { return pType; }

	typedef std::list< CPtr<CAttributeNode> >::const_iterator TAttrIter;
	TAttrIter AttrBegin() const;
	TAttrIter AttrEnd() const;
};

class CVariableNode : public CVariable
{
	OBJECT_NOCOPY_METHODS( CVariableNode );
public:
	CVariableNode()  { }
	CVariableNode( const std::string &szName, const std::string &szFile, int nLine )
		: CVariable( szName, szFile, nLine ) { }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class CVectorNode : public CVariable
{
	OBJECT_NOCOPY_METHODS( CVectorNode );
	int nMinAmount;
		// if nMaxAmount == -1 then vector is unbounded [0..
	int nMaxAmount;
public:
	CVectorNode() { }
	CVectorNode( const std::string &szName, int _nMinAmount, int _nMaxAmount, const std::string &szFile, int nLine )
		: CVariable( szName, szFile, nLine ), nMinAmount( _nMinAmount ), nMaxAmount( _nMaxAmount ) { }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}

	int GetMinAmount() const { return nMinAmount; }
	int GetMaxAmount() const { return nMaxAmount; }
};

class CAttributeDefNode : public CTypeNode
{
	OBJECT_NOCOPY_METHODS( CAttributeDefNode );

	ESimpleType eNodeType;
public:
	CAttributeDefNode() { }
	CAttributeDefNode( ESimpleType _eNodeType, const std::string &szFile, int nLine ) : CTypeNode( "", szFile, nLine ), eNodeType( _eNodeType ) { }

	ESimpleType GetType() const { return eNodeType; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class PARSER_EXPORT CNamespace : public CLangNode
{
	OBJECT_NOCOPY_METHODS( CNamespace );

	std::list< CObj<CLangNode> > insideDefList;
	std::unordered_map<std::string, CObj<CLangNode> > insideDefs;

	typedef std::unordered_map<std::string, std::list< CObj<CTypeNode> > > TForwards;
	TForwards insideForwards;
	std::unordered_map<std::string, CObj<CAttributeDefNode> > insideAttrs;

	std::unordered_set<std::string> files;
	std::list<std::string> badIncludes;

	CPtr< CNodesList<CComplexTypeNode> > pVisibleTypes;
public:
	CNamespace() { }
	CNamespace( CNodesList<CComplexTypeNode> *_pVisibleTypes, const std::string &szFile, int nLine )
		: pVisibleTypes( _pVisibleTypes ), CLangNode( "", szFile, nLine ) { }

	void MergeFiles( CNamespace *pNM, const std::string &szFileName );

	CAttributeDefNode* FindInsideAttrDef( const std::string &szAttrDef );
	void AddInsideAttrDef( CAttributeDefNode *pAttrDefNode );

	CTypeNode* FindForward( const std::string &szTypeName );
	CLangNode* FindInsideDef( const std::string &szDefName );
	void AddInsideDef( CLangNode *pNode );

	CEnumEntryNode* FindEnumEntry( const std::string &szEnumEntry ) const;
	void AddBadInclude( const std::string &szInclude ) { badIncludes.push_back( szInclude ); }

	void ResolveForwards();

	typedef std::unordered_map<std::string, CObj<CLangNode> >::const_iterator TLangNodeIter;
	TLangNodeIter DefsBegin() const;
	TLangNodeIter DefsEnd() const;

	typedef std::list< CObj<CLangNode> >::const_iterator TDefsListIter;
	TDefsListIter DefsListBegin() const { return insideDefList.begin(); }
	TDefsListIter DefsListEnd() const { return insideDefList.end(); }

	void SetDefsListFrom( CNamespace *pNM ) { insideDefList = pNM->insideDefList; }

	const std::list<std::string>& GetBadIncludes() const { return badIncludes; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

class PARSER_EXPORT CComplexTypeNode : public CTypeNode
{
	OBJECT_NOCOPY_METHODS( CComplexTypeNode );

	bool bClass;

	CObj< CNodesList<CAttributeNode> > pAttrList;
	CPtr< CNodesList<CComplexTypeNode> > pParentsList;
	CObj<CNamespace> pNamespace;
public:
	CComplexTypeNode() { }
	CComplexTypeNode( const std::string &szName, bool _bClass, bool bForward, const std::string &szFile, int nLine )
		: CTypeNode( szName, bForward, szFile, nLine ), bClass( _bClass ) { }

	bool IsClass() const { return bClass; }

	void AddParents( CNodesList<CComplexTypeNode> *_pParentsList ) { pParentsList = _pParentsList; }
	void AddAttributes( CNodesList<CAttributeNode> *_pAttrList ) { pAttrList = _pAttrList; }
	void AddNamespace( CNamespace *_pNamespace ) { pNamespace = _pNamespace; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}

	typedef std::list< CPtr<CAttributeNode> >::const_iterator TAttrIter;
	TAttrIter AttrBegin() const;
	TAttrIter AttrEnd() const;

	typedef std::list< CPtr<CComplexTypeNode> >::const_iterator TParentsIter;
	TParentsIter ParentsBegin() const;
	TParentsIter ParentsEnd() const;

	CNamespace* GetNamespace() const { return pNamespace; }
};

class CBaseTypeNode : public CTypeNode
{
	OBJECT_NOCOPY_METHODS( CBaseTypeNode );

	bool bClass;
public:
	CBaseTypeNode() : bClass( false ) { }
	CBaseTypeNode( const std::string &szTypeName, bool _bClass, const std::string &szFile, int nLine )
		: CTypeNode( szTypeName, szFile, nLine ), bClass( _bClass ) { }

	bool IsClass() const { return bClass; }

	virtual void Visit( IVisitor *pVisitor ) { pVisitor->Visit( this );	}
};

}


