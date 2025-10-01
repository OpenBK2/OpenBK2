#pragma once

#include "System_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../Misc/Pool.h"
#define HREF_ATTRIBUTE_NAME "href"
#define DATA_ATTRIBUTE_NAME "data"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NXml
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SXmlValue
{
	const char *pszBegin;
	int nSize;

	SXmlValue() : pszBegin( 0 ), nSize( 0 ) { }

	const string ToString() const { return string( pszBegin, nSize ); }
};

inline bool operator==( const SXmlValue &val, const string &str )
{
	return val.nSize == str.size() && strncmp( val.pszBegin, str.c_str(), val.nSize ) == 0;
}
inline bool operator==( const string &str, const SXmlValue &val )
{
	return val.nSize == str.size() && strncmp( val.pszBegin, str.c_str(), val.nSize ) == 0;
}

inline bool operator==( const SXmlValue &val, const char *pStr )
{
	int i = 0;
	while ( i < val.nSize && *(pStr + i) != 0 && *(pStr + i ) == *(val.pszBegin + i) )
		++i;

	return i == val.nSize && *(pStr + i) == 0;
}

inline bool operator==( const char *pStr, const SXmlValue &val )
{
	return val == pStr;
}

struct SXmlAttribute
{
	SXmlValue name;
	SXmlValue value;
};

class SYSTEM_EXPORT CXmlNode
{
	vector<const SXmlAttribute*> attributes;
	vector<const CXmlNode*> nodes;
	SXmlValue name;
	SXmlValue value;

	const SXmlAttribute* pHRefAttr;
	const SXmlAttribute* pHDataAttr;

	mutable int nBestNodeToRead;
	mutable int nBestAttrToRead;

	struct SPool *pPool;

	const char* ParseNodeInternal( const char *pszBegin );
	const char* ParseNodeAttributes( const char *pszBegin );
public:
	CXmlNode() : nBestNodeToRead( 0 ), pHRefAttr( 0 ), nBestAttrToRead( 0 ), pHDataAttr( 0 ), pPool( 0 ) { }
	
	const char* Parse( const char *pszBegin, SPool *pPool );

	const SXmlValue& GetValue() const { return value; }
	const SXmlValue& GetName() const { return name; }

	const vector<const SXmlAttribute*>& GetAttributes() const { return attributes; }
	const vector<const CXmlNode*>& GetNodes() const { return nodes; }

	const CXmlNode* FindChild( const char *pszChild ) const;

	const SXmlAttribute* GetHRefAttribute() const;
	const SXmlAttribute* GetDataAttribute() const;
	const SXmlAttribute* GetAttribute( const char *pszAttr ) const;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SPool
{
	CPool<CXmlNode, 32> nodesPool;
	CPool<SXmlAttribute, 32> attrPool;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SYSTEM_EXPORT CXmlReader : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CXmlReader )

	CXmlNode rootNode;
	char* buf;
	SPool pool;
public:
	CXmlReader() : buf( 0 ) {}
	CXmlReader( const char *pszBegin, const char *pszEnd );
	~CXmlReader() { if ( buf ) delete []buf; }

	const CXmlNode* GetRootElement() const { return &rootNode; }
};
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
