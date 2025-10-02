#include "stdafx.h"

#include "LightXMLUtils.h"
#include "XMLReader.h"

namespace NXml
{

static const char* FindNodeStart( const char *p )
{
	while ( *p && *p != '<' )
		++p;

	if ( *p )
		++p;

	return p;
}

static const char* FindNodeEnd( const char *p )
{
	while ( *p && *p != '>' )
		++p;

	if ( *p )
		++p;

	return p;
}

static  bool IsWhiteSpace( const char chr )
{
	return (chr == ' ') || ( chr == '\n' ) || ( chr =='\r' ) || ((chr >= 0x09) && (chr <= 0x0d));
}

static const char* FindNotText( const char *p )
{
	while ( *p && !IsWhiteSpace( *p ) && *p != '>' && *p != '/' && *p != '=' && *p != '<' )
		++p;

	return p;
}

static const char* FindEndOfNodeText( const char *p )
{
	while ( *p && *p != '<' )
		++p;

	return p;
}

static  const char* SkipWhiteSpace( const char *p )
{
	while ( *p != 0 && IsWhiteSpace(*p) ) 
		++p;
	//
	return p;
}

static const char* SkipComments( const char *p )
{
	while ( true )
	{
		if ( *p != '!' )
			return p;

		++p;
		while ( true )
		{
			if ( !(*p) || !(*(p+1)) || !(*(p+2)) )
				break;

			if ( *p == '-' && *(p+1) == '-' && *(p+2) == '>' )
			{
				p += 3;
				break;
			}

			++p;
		}
		p = SkipWhiteSpace( p );
		if ( !(*p) )
			return p;

		++p;
	}
}

const char* CXmlNode::ParseNodeAttributes( const char *p )
{
	while ( true )
	{
		p = SkipWhiteSpace( p );
		if ( !*p )
			return p;

		if ( *p == '>' || *p == '/' )
			return p;

		SXmlAttribute *pAttr = pPool->attrPool.Alloc();

		pAttr->name.pszBegin = p;
		p = FindNotText( p );
		pAttr->name.nSize = p - pAttr->name.pszBegin;

		p = SkipWhiteSpace( p );
		if ( !*p || *p != '=' )
			return p;

		++p;
		p = SkipWhiteSpace( p );
		if ( *p != '"' )
			return p;

		++p;
		pAttr->value.pszBegin = p;
		while ( *p && *p != '"' )
			++p;
		pAttr->value.nSize = p - pAttr->value.pszBegin;
		if ( *p )
			++p;

		if  ( pAttr->name == HREF_ATTRIBUTE_NAME )
			pHRefAttr = pAttr;
		if ( pAttr->name == DATA_ATTRIBUTE_NAME )
			pHDataAttr = pAttr;

		attributes.push_back( pAttr );
	}
}

const char* CXmlNode::ParseNodeInternal( const char *p )
{
	while ( true )
	{
		p = SkipWhiteSpace( p );
		if ( !*p )
			return p;

		if ( *p == '<' )
		{
			if ( *(p+1) == '/' )
			{
				p = FindNodeEnd( p );
				return p;
			}
			if ( *(p+1) == '!' )
			{
				p = SkipComments( p + 1 );
				--p;
				continue;
			}

			CXmlNode *pChild = pPool->nodesPool.Alloc();
			p = pChild->Parse( p + 1, pPool );
			nodes.push_back( pChild );
		}
		else
		{
			value.pszBegin = p;
			p = FindEndOfNodeText( p );
/*
			if ( p != value.pszBegin && IsWhiteSpace( *p ) )
			{
				while ( p - 1 != value.pszBegin && IsWhiteSpace( *(p-1) ) )
					--p;
			}
*/
			value.nSize = p - value.pszBegin;
		}
	}

	return p;
}

const char* CXmlNode::Parse( const char *p, SPool *_pPool )
{
	nBestNodeToRead = 0;
	nBestAttrToRead = 0;
	nodes.clear();
	attributes.clear();
	pHRefAttr = 0;
	pHDataAttr = 0;
	pPool = _pPool;
	
	p = SkipComments( p );
	p = SkipWhiteSpace( p );
	if ( !*p )
		return p;

	name.pszBegin = p;
	p = FindNotText( p );
	name.nSize = p - name.pszBegin;

	p = SkipWhiteSpace( p );

	if ( *p == '>' )
		p = ParseNodeInternal( p + 1 );
	else if ( *p == '/' )
	{
		++p;
		if ( *p )
			++p;
	}
	else
	{
		p = ParseNodeAttributes( p );
		if ( *p == '>' )
			p = ParseNodeInternal( p + 1 );
		else if ( *p == '/' )
		{
			++p;
			if ( *p )
				++p;
		}
	}

	return p;
}

const CXmlNode* CXmlNode::FindChild( const char *pszChild ) const
{
	if ( nodes.empty() )
		return 0;

	nBestNodeToRead %= nodes.size();
	if ( nodes[nBestNodeToRead]->GetName() == pszChild )
		return nodes[nBestNodeToRead++];
	
	const int nMemBestNodeToRead = ++nBestNodeToRead;
	while ( nBestNodeToRead < nodes.size() )
	{
		if ( nodes[nBestNodeToRead]->GetName() == pszChild )
			return nodes[nBestNodeToRead++];

		++nBestNodeToRead;
	}

	nBestNodeToRead = 0;
	while ( nBestNodeToRead < nMemBestNodeToRead )
	{
		if ( nodes[nBestNodeToRead]->GetName() == pszChild )
			return nodes[nBestNodeToRead++];

		++nBestNodeToRead;
	}

	return 0;
}

const SXmlAttribute* CXmlNode::GetHRefAttribute() const
{
	return pHRefAttr;
}

const SXmlAttribute* CXmlNode::GetDataAttribute() const
{
	return pHDataAttr;
}

const SXmlAttribute* CXmlNode::GetAttribute( const char *pszAttr ) const
{
	if ( attributes.empty() )
		return 0;

	nBestAttrToRead %= attributes.size();
	if ( attributes[nBestAttrToRead]->name == pszAttr )
		return attributes[nBestAttrToRead++];
	
	const int nMemBestAttrToRead = ++nBestAttrToRead;
	while ( nBestAttrToRead < attributes.size() )
	{
		if ( attributes[nBestAttrToRead]->name == pszAttr )
			return attributes[nBestAttrToRead++];

		++nBestAttrToRead;
	}

	nBestAttrToRead = 0;
	while ( nBestAttrToRead < nMemBestAttrToRead )
	{
		if ( attributes[nBestAttrToRead]->name == pszAttr )
			return attributes[nBestAttrToRead++];

		++nBestAttrToRead;
	}

	return 0;
}

CXmlReader::CXmlReader( const char *pszBegin, const char *pszEnd )
{
	if ( !pszBegin || !*pszBegin )
		return;

	buf = new char[pszEnd - pszBegin + 1];
	memcpy( buf, pszBegin, pszEnd - pszBegin );
	buf[pszEnd - pszBegin] = 0;

	const char *p = FindNodeStart( buf );
	if ( *p == '?' )
		p = FindNodeStart( p );
	if ( !*p )
		return;

	rootNode.Parse( p, &pool );
}

}

using namespace NXml;
REGISTER_SAVELOAD_CLASS( 0x3023C380, CXmlReader );

