#ifndef __LIGHTXML_H__
#define __LIGHTXML_H__

#pragma ONCE

#include "System_export.h"


#include "LightXMLUtils.h"

typedef NLXML::CWriteStream NLXML_STREAM;

namespace NLXML
{

// ************************************************************************************************************************ //
// **
// ** base class for all XML elements
// **
// **
// **
// ************************************************************************************************************************ //

class CXMLBase
{
public:
	virtual ~CXMLBase() {  }
};

// ************************************************************************************************************************ //
// **
// ** An attribute is a name-value pair. Elements have an arbitrary number of attributes, each with a unique name.
// ** The attributes are not Nodes, since they are not part of the XML Document Object Model. There are other
// ** suggested ways to look at this problem.
// ** Attributes have a parent
// **
// **
// ************************************************************************************************************************ //

class CXMLAttribute : public CXMLBase
{
	friend class CXMLElement;	// only element can set attribute value directly
	//
	string szName;										// name
	string szValue;									// value
	mutable DWORD dwHashCode;							// internal hash code value (for fast search)
	//
	void SetValue( const string &_szValue ) { szValue = _szValue; }
	DWORD GetHashCode() const 
	{
		if ( dwHashCode == 0 ) 
			dwHashCode = hash<string>()( szName );
		return dwHashCode;
	}
public:
	CXMLAttribute() : dwHashCode( 0 ) {  }
	CXMLAttribute( const string &_szName, const string &_szValue )
		: szName( _szName ), szValue( _szValue ), dwHashCode( 0 ) {  }
	virtual ~CXMLAttribute() {  }
	//
	bool operator==( const CXMLAttribute &attr ) const
	{
		return GetHashCode() == attr.GetHashCode() ? szName == attr.szName : false;
	}
	bool operator<( const CXMLAttribute &attr ) const
	{
		return GetHashCode() != attr.GetHashCode() ? szName < attr.szName : false;
	}
	//
	const char* Parse( const char *pszBegin, const char *pszEnd );
	void Store( NLXML_STREAM &stream ) const;
	// set/get value
	void Set( const string &_szValue ) { szValue = _szValue; }
	void Set( const CToStringConvertor &value ) { szValue = value; }
	const string& GetValue() const { return szValue; }
	const string& GetName() const { return szName; }
};

// ************************************************************************************************************************ //
// **
// ** The parent class for everything in the Document Object Model. (Except for attributes, which are contained in elements.)
// ** Nodes have siblings, a parent, and children. A node can be in a document, or stand on its own. The type of a Node
// ** can be queried, and it can be cast to its more defined type.
// **
// ************************************************************************************************************************ //

class CXMLNode : public CXMLBase
{
public:
	enum EType
	{
		DOCUMENT,
		DECLARATION,
		COMMENT,
		UNKNOWN,
		ELEMENT,
		TEXT
	};
private:
	const EType eType;										// node type (to avoid dynamic_cast)
	DWORD dwHashCode;											// hash code for this node by value (== 0 if empty)
protected:
	string szValue;									// node value
public:
	CXMLNode( const EType _eType );
	virtual ~CXMLNode();
	//
	EType GetType() const { return eType; }
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd ) = 0;
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const = 0;
	//
	const string& GetValue() const { return szValue; }
	void SetValue( const string &_szValue ) { szValue = _szValue; dwHashCode = 0; }
	bool IsMatch( const string &szMatchValue, const DWORD dwMatchHashCode )
	{
		if ( dwHashCode == 0 ) 
			dwHashCode = hash<string>()( szValue );
		if ( dwHashCode == dwMatchHashCode ) 
			return szMatchValue == szValue;
		else
			return false;
	}
};

// ************************************************************************************************************************ //
// **
// ** The parent class for document and element in the Document Object Model.
// ** Multinodes have children. A node can be in a document or user element. 
// ** 
// **
// ************************************************************************************************************************ //

class CXMLMultiNode : public CXMLNode
{
	typedef vector<CXMLNode*> CNodesList;
public:
	typedef CNodesList::iterator iterator;
	typedef CNodesList::const_iterator const_iterator;
private:
	CNodesList children;									// children nodes
	mutable const_iterator posOptimal;		// optimal search position (for fast incremental search)
protected:
	void ResetOptimalPos() const { posOptimal = children.begin(); }
	CXMLNode* Identify( const char *pszBegin, const char *pszEnd );
public:
	CXMLMultiNode( const CXMLNode::EType _eType ) 
		: CXMLNode( _eType ) {  }
	virtual ~CXMLMultiNode();
	//
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
	//
	iterator begin() { return children.begin(); }
	iterator end() { return children.end(); }
	const_iterator begin() const { return children.begin(); }
	const_iterator end() const { return children.end(); }
	//
	void AddChild( CXMLNode *pNode ) { children.push_back( pNode ); ResetOptimalPos(); }
	bool HasChildren() const { return !children.empty(); }
	CXMLNode* GetChild( const int nIndex ) const { return nIndex < children.size() ? children[nIndex] : 0; }
	CXMLNode* GetChildFast( const int nIndex ) const { return children[nIndex]; }
	int CountChildren() const { return children.size(); }
	CXMLNode* FindChild( const string &_szValue ) const;
};

// ************************************************************************************************************************ //
// **
// ** comment
// **
// **
// **
// ************************************************************************************************************************ //

class CXMLComment : public CXMLNode
{
public:
	CXMLComment();
	virtual ~CXMLComment();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
};

// ************************************************************************************************************************ //
// **
// ** XML text. Contained in an element.
// **
// **
// **
// ************************************************************************************************************************ //

class CXMLText : public CXMLNode
{
public:
	CXMLText();
	virtual ~CXMLText();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
	//
	bool IsBlank() const
	{
		for ( string::const_iterator it = szValue.begin(); it != szValue.end(); ++it )
		{
			if ( !isspace( BYTE(*it) ) )
				return false;
		}
		return true;
	}
};

// ************************************************************************************************************************ //
// **
// ** In correct XML the declaration is the first entry in the file.
// **		<?xml version="1.0" standalone="yes"?>
// ** LigntXML will happily read or write files without a declaration, however.
// ** There are 3 possible attributes to the declaration: version, encoding, and standalone.
// ** Note: In this version of the code, the attributes are handled as special cases, not generic attributes, simply
// ** because there can only be at most 3 and they are always the same.
// **
// **
// ************************************************************************************************************************ //

class SYSTEM_EXPORT CXMLDeclaration : public CXMLNode
{
	string szVersion;
	string szStandalone;
	string szEncoding;
public:
	CXMLDeclaration();
	virtual ~CXMLDeclaration();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
	//
	void SetVersion( const string &szValue ) { szVersion = szValue; }
	const string& GetVersion() const { return szVersion; }
	void SetStandalone( const string &szValue ) { szStandalone = szValue; }
	const string& GetStandalone() const { return szStandalone; }
	void SetEncoding( const string &szValue ) { szEncoding = szValue; }
	const string& GetEncoding() const { return szEncoding; }
};

// ************************************************************************************************************************ //
// **
// ** Any tag that LightXml doesn't recognize is save as an unknown. It is a tag of text, but should not be modified.
// ** It will be written back to the XML, unchanged, when the file is saved.
// **
// **
// ************************************************************************************************************************ //

class CXMLUnknown : public CXMLNode
{
public:
	CXMLUnknown();
	virtual ~CXMLUnknown();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
};

// ************************************************************************************************************************ //
// **
// ** The element is a container class. It has a value, the element name, 
// ** and can contain other elements, text, comments, and unknowns.
// ** Elements also contain an arbitrary number of attributes.
// **
// **
// ************************************************************************************************************************ //

class SYSTEM_EXPORT CXMLElement : public CXMLMultiNode
{
	typedef list<CXMLAttribute> CAttributesList;
	typedef hash_map<string, CXMLAttribute*> CAttributesMap;
	CAttributesList attributes;						// attributes
	CAttributesMap attrmap;								//
	string szText;										// text of this element
	//
	const char* ReadValue( const char *pszBegin, const char *pszEnd );
	void SetAttributeLocal( const string &szName, const string &szValue );
public:
	CXMLElement();
	virtual ~CXMLElement();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention ) const;
	// attributes
	void SetAttribute( const CXMLAttribute &attribute );
	void SetAttribute( const string &szName, const CToStringConvertor &value );
	void SetAttribute( const string &szName, const string &szValue ) { SetAttributeLocal(szName, szValue); }
	const CXMLAttribute* GetAttribute( const string &szName ) const;
	void RemoveAttribute( const string &szName );
	const list<CXMLAttribute> &GetAttributesList() const { return attributes; }
	// text
	const string& GetText() const { return szText; }
	void SetText( const string &_szText ) { szText = _szText; }
	void RemoveText() { szText.clear(); } 
};

// ************************************************************************************************************************ //
// **
// ** Always the top level node. A document binds together all the XML pieces. It can be saved, loaded and modified
// ** 
// ** 
// **
// ************************************************************************************************************************ //

class SYSTEM_EXPORT CXMLDocument : public CXMLMultiNode
{
public:
	CXMLDocument();
	virtual ~CXMLDocument();
	//
	virtual const char* Parse( const char *pszBegin, const char *pszEnd );
	virtual void Store( NLXML_STREAM &stream, const string &szIndention = "" ) const;
	//
	CXMLElement *GetRootElement();
};

}; // end of namespace NLXML

#endif // __LIGHTXML_H__
