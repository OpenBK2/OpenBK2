#include "StdAfx.h"

#include "LightXML.h"

namespace NLXML
{

static const char *szEndOfLine = "\x0D\n";
static const char *szTab = "\t";

// ************************************************************************************************************************ //
// **
// ** attribute
// **
// **
// **
// ************************************************************************************************************************ //

const char* CXMLAttribute::Parse( const char *pszBegin, const char *pszEnd )
{
	const char *p = SkipWhiteSpace( pszBegin );
	if ( !p || !*p ) 
		return 0;

	// Read the name, the '=' and the value.
	p = ReadName( p, &szName );
	if ( !p || !*p )
	{
		// TIXML_ERROR_READING_ATTRIBUTES
		return 0;
	}
	p = SkipWhiteSpace( p );
	if ( !p || !*p || *p != '=' )
	{
		// TIXML_ERROR_READING_ATTRIBUTES
		return 0;
	}

	p = SkipWhiteSpace( p + 1 ); // skip '='
	if ( !p || !*p )
	{
		// TIXML_ERROR_READING_ATTRIBUTES
		return 0;
	}
	
	if ( *p == '\'' )
	{
		++p;
		static const string szEndTag = "\'";
		p = ReadText( &szValue, p, pszEnd, szEndTag, false );
	}
	else if ( *p == '"' )
	{
		++p;
		static const string szEndTag = "\"";
		p = ReadText( &szValue, p, pszEnd, szEndTag, false );
	}
	else
	{
		// All attribute values should be in single or double quotes.
		// But this is such a common error that the parser will try its best, even without them.
		szValue.clear();
		while ( p && *p	&&								// existence
				    !IsWhiteSpace(*p) &&			// whitespace
						*p != '/' && *p != '>' )	// tag end
		{
			szValue += *p;
			++p;
		}
	}
	return p;
}

void CXMLAttribute::Store( NLXML_STREAM &stream ) const
{
	if ( szValue.find('\"') != string::npos )
	{
		stream.WriteChecked( szName );
		stream << "='";
		stream.WriteChecked( szValue );
		stream << "'";
	}
	else
	{
		stream.WriteChecked( szName );
		stream << "=\"";
		stream.WriteChecked( szValue );
		stream << "\"";
	}
}

// ************************************************************************************************************************ //
// **
// ** XML node general functions
// **
// **
// **
// ************************************************************************************************************************ //

CXMLNode::CXMLNode( const EType _eType )
: eType( _eType ), dwHashCode( 0 )
{
}

CXMLNode::~CXMLNode()
{
}

// ************************************************************************************************************************ //
// **
// ** XML multi-node
// **
// **
// **
// ************************************************************************************************************************ //

static const char* pszXMLHeader = "<?xml";
static const char* pszCommentHeader = "<!--";

bool IsHeader( const char *pszBegin, const char *pszEnd )
{
	// min node <?xml?>
	return (pszEnd - pszBegin >= 7) && ( *((DWORD*)pszBegin) == *((DWORD*)pszXMLHeader) ) && ( pszBegin[4] == 'l' );
}
bool IsComment( const char *pszBegin, const char *pszEnd )
{
	// min node <!---->
	return (pszEnd - pszBegin >= 7) && ( *((DWORD*)pszBegin) == *((DWORD*)pszCommentHeader) );
}

CXMLMultiNode::~CXMLMultiNode() 
{  
	for ( CNodesList::iterator it = children.begin(); it != children.end(); ++it )
		delete *it;
}

CXMLNode* CXMLMultiNode::Identify( const char *pszBegin, const char *pszEnd )
{
	const char *p = SkipWhiteSpace( pszBegin );
	if( !p || !*p || *p != '<' )
		return 0;

	// What is this thing? 
	// - Elements start with a letter or underscore, but xml is reserved.
	// - Comments: <!--
	// - Declaration: <?xml
	// - Everthing else is unknown to tinyxml.
	//
	CXMLNode *pNode = 0;
	if ( IsHeader(p, pszEnd) )
		pNode = new CXMLDeclaration();
	else if ( isalpha(*(p + 1)) || *(p + 1) == '_' )
		pNode = new CXMLElement();
	else if ( IsComment(p, pszEnd) )
		pNode = new CXMLComment();
	else
		pNode = new CXMLUnknown();
	return pNode;
}

void CXMLMultiNode::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	for ( CNodesList::const_iterator it = children.begin(); it != children.end(); ++it )
	{
		stream << szIndention;
		(*it)->Store( stream, szIndention );
		stream << szEndOfLine;
	}
}

CXMLNode* CXMLMultiNode::FindChild( const string &_szValue ) const
{
	const DWORD dwChildHashCode = hash<string>()( _szValue );
	// try optimized search from optimal search position
	for ( CNodesList::const_iterator it = posOptimal; it != children.end(); ++it )
	{
		if ( (*it)->IsMatch(_szValue, dwChildHashCode) ) 
		{
			posOptimal = it;
			++posOptimal;
			return (*it);
		}
	}
	// search from begin to optimal search position...
	for ( CNodesList::const_iterator it = children.begin(); it != posOptimal; ++it )
	{
		if ( (*it)->IsMatch(_szValue, dwChildHashCode) ) 
		{
			posOptimal = it;
			++posOptimal;
			return (*it);
		}
	}
	// can't find. reset optimal search position
	ResetOptimalPos();
	//
	return 0;
}

// ************************************************************************************************************************ //
// **
// ** XML element
// **
// **
// **
// ************************************************************************************************************************ //

CXMLElement::CXMLElement()
: CXMLMultiNode( CXMLNode::ELEMENT )
{
}
CXMLElement::~CXMLElement()
{
}

void CXMLElement::SetAttributeLocal( const string &szName, const string &szValue )
{
	CAttributesMap::iterator pos = attrmap.find( szName );
	if ( pos != attrmap.end() ) 
		pos->second->SetValue( szValue );
	else
	{
		attributes.push_back( CXMLAttribute(szName, szValue) );
		attrmap[szName] = &( attributes.back() );
	}
}

void CXMLElement::SetAttribute( const CXMLAttribute &attr )
{
	SetAttributeLocal( attr.GetName(), attr.GetValue() );
}
void CXMLElement::SetAttribute( const string &szName, const CToStringConvertor &value )
{
	SetAttributeLocal( szName, value );
}

const CXMLAttribute* CXMLElement::GetAttribute( const string &szName ) const
{
	CAttributesMap::const_iterator pos = attrmap.find( szName );
	return pos != attrmap.end() ? pos->second : 0;
}

void CXMLElement::RemoveAttribute( const string &szName )
{
	CAttributesMap::iterator pos = attrmap.find( szName );	
	if ( pos != attrmap.end() ) 
	{
		attrmap.erase( pos );
		for ( CAttributesList::iterator it = attributes.begin(); it != attributes.end(); ++it )
		{
			if ( it->GetName() == szName ) 
			{
				attributes.erase( it );
				return;
			}
		}
	}
}

const char* CXMLElement::ReadValue( const char *pszBegin, const char *pszEnd )
{
	// Read in text and elements in any order.
	const char *p = SkipWhiteSpace( pszBegin );
	while ( p && *p )
	{
		if ( *p != '<' )
		{
			// Take what we have, make a text element.
			CXMLText *pTextNode = new CXMLText();
			p = pTextNode->Parse( p, pszEnd );

			if ( !pTextNode->IsBlank() )
				szText = pTextNode->GetValue();

			delete pTextNode;
		} 
		else 
		{
			// We hit a '<'
			// Have we hit a new element or an end tag? (p == "</")
			// HINT: check only *(p + 1) == '/', because here we know, that *p == '<'
			if ( *(p + 1) == '/' )
				return p;
			else
			{
				if ( CXMLNode *pNode = Identify(p, pszEnd) )
				{
					p = pNode->Parse( p, pszEnd );
					AddChild( pNode );
				}				
				else
					return 0;
			}
		}
		p = SkipWhiteSpace( p );
	}

	return p;
}

const char* CXMLElement::Parse( const char *pszBegin, const char *pszEnd )
{
	ResetOptimalPos();
	//
	const char *p = SkipWhiteSpace( pszBegin );

	if ( !p || !*p || *p != '<' )
		return 0;
	p = SkipWhiteSpace( p + 1 );
	// Read the name.
  p = ReadName( p, &szValue );
	if ( !p || !*p )
		return 0;

	const string szEndTag = "</" + szValue + ">";
	// Check for and read attributes. Also look for an empty tag or an end tag.
	while ( p && *p )
	{
		p = SkipWhiteSpace( p );
		if ( !p || !*p )
			return 0;
		if ( *p == '/' )
		{
			++p;
			// Empty tag.
			if ( *p  != '>' )
				return 0;
			return p + 1;
		}
		else if ( *p == '>' )
		{
			// Done with attributes (if there were any.) 
			// Read the value -- which can include other elements - read the end tag, and return.
			++p;
			p = ReadValue( p, pszEnd );		// Note this is an Element method
			if ( !p || !*p )
				return 0;

			// We should find the end tag now
			if ( IsEqualSubstring( szEndTag, p, pszEnd) )
			{
				p += szEndTag.length();
				return p;
			}
			else
				return 0;
		}
		else
		{
			// Try to read an attribute:
			CXMLAttribute attrib;
			p = attrib.Parse( p, pszEnd );
			if ( !p || !*p )
				return 0;
			SetAttribute( attrib );
		}
	}
	return p;
}

void CXMLElement::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	stream << "<" << szValue;
	for ( CAttributesList::const_iterator it = attributes.begin(); it != attributes.end(); ++it )
	{
		stream << " ";
		it->Store( stream );
	}
	// If this node has children or text, give it a closing tag. Else make it an empty tag.
	if ( HasChildren() ) 
	{
		string szNextIndention = szIndention + szTab;
		stream << ">";
		stream << szEndOfLine;
		if ( !szText.empty() )
		{
			stream << szNextIndention;
			stream.WriteChecked( szText );
			stream << szEndOfLine;
		}
		CXMLMultiNode::Store( stream, szNextIndention );
		stream << szIndention;
		stream << "</" << szValue << ">";
	}
	else
		if ( !szText.empty() ) 
		{
			stream << ">";
			stream.WriteChecked( szText );
			stream << "</" << szValue << ">";
		}
		else
			stream << " />";
}
// ************************************************************************************************************************ //
// **
// ** text
// **
// **
// **
// ************************************************************************************************************************ //

CXMLText::CXMLText()
: CXMLNode( CXMLNode::TEXT )
{
}
CXMLText::~CXMLText()
{
}

const char* CXMLText::Parse( const char *pszBegin, const char *pszEnd )
{
	szValue.clear();
	static const string szEndTag = "<";
	const char *p = ReadText( &szValue, pszBegin, pszEnd, szEndTag, false );
	//
	return p != 0 ? p - 1 : 0; // don't truncate the '<'
}

void CXMLText::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	stream.WriteChecked( szValue );
}

// ************************************************************************************************************************ //
// **
// ** XML Comment
// **
// **
// **
// ************************************************************************************************************************ //

CXMLComment::CXMLComment()
: CXMLNode( CXMLNode::COMMENT )
{
}
CXMLComment::~CXMLComment()
{
}

const char* CXMLComment::Parse( const char *pszBegin, const char *pszEnd )
{
	szValue.clear();

	const char *p = SkipWhiteSpace( pszBegin );

	if ( !IsComment(p, pszEnd) )
	{
		// TIXML_ERROR_PARSING_COMMENT
		return 0;
	}
	p += 4; // strlen( "<!--" )
	static string szCommentEndTag = "-->";
	p = ReadText( &szValue, p, pszEnd, szCommentEndTag, false );
	return p;
}

void CXMLComment::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	stream << "<!--";
	stream.WriteChecked( szValue );
	stream << "-->";
}

// ************************************************************************************************************************ //
// **
// ** declaration
// **
// **
// **
// ************************************************************************************************************************ //

CXMLDeclaration::CXMLDeclaration()
: CXMLNode( CXMLNode::DECLARATION ), szVersion( "1.0" ), szEncoding( "UTF-8" )
{
}
CXMLDeclaration::~CXMLDeclaration()
{
}

const char* CXMLDeclaration::Parse( const char *pszBegin, const char *pszEnd )
{
	const char *p = SkipWhiteSpace( pszBegin );
	// Find the beginning, find the end, and look for the stuff in-between.
	if ( !p || !*p || !IsHeader(p, pszEnd) )
	{
		// TIXML_ERROR_PARSING_DECLARATION
		return 0;
	}

	p += 5;

	szVersion.clear();
	szEncoding.clear();
	szStandalone.clear();

	while ( p && *p )
	{
		if ( *p == '>' )
		{
			++p;
			return p;
		}

		p = SkipWhiteSpace( p );
		if ( IsEqualSubstringIC("version", p) )
		{
			// p += 7
			CXMLAttribute attrib;
			p = attrib.Parse( p, pszEnd );		
			szVersion = attrib.GetValue();
		}
		else if ( IsEqualSubstringIC("encoding", p) )
		{
			// p += 8
			CXMLAttribute attrib;
			p = attrib.Parse( p, pszEnd );		
			szEncoding = attrib.GetValue();
		}
		else if ( IsEqualSubstringIC("standalone", p) )
		{
			// p += 10;
			CXMLAttribute attrib;
			p = attrib.Parse( p, pszEnd );		
			szStandalone = attrib.GetValue();
		}
		else
		{
			// Read over whatever it is.
			while( p && *p && *p != '>' && !isspace( BYTE(*p) ) )
				++p;
		}
	}
	return 0;
}

void CXMLDeclaration::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	stream << "<?xml ";
	//
	if ( !szVersion.empty() )
	{
		stream << "version=\"";
		stream.WriteChecked( szVersion );
		stream << "\" ";
	}
	if ( !szEncoding.empty() )
	{
		stream << "encoding=\"";
		stream.WriteChecked( szEncoding );
		stream << "\" ";
	}
	if ( !szStandalone.empty() )
	{
		stream << "standalone=\"";
		stream.WriteChecked( szStandalone );
		stream << "\" ";
	}
	//
	stream << "?>";
}

// ************************************************************************************************************************ //
// **
// ** XML Unknown
// **
// **
// **
// ************************************************************************************************************************ //

CXMLUnknown::CXMLUnknown()
: CXMLNode( CXMLNode::UNKNOWN )
{
}
CXMLUnknown::~CXMLUnknown()
{
}

const char* CXMLUnknown::Parse( const char *pszBegin, const char *pszEnd )
{
	const char *p = SkipWhiteSpace( pszBegin );
	if ( !p || !*p || *p != '<' )
		return 0;
	++p;
  szValue.clear();

	while ( p && *p && *p != '>' )
	{
		szValue += *p;
		++p;
	}

	if ( !p )
	{
		// TIXML_ERROR_PARSING_UNKNOWN
	}
	//
	return *p == '>' ? p + 1 : p;
}

void CXMLUnknown::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	stream << "<" << szValue << ">";		// Don't use entities hear! It is unknown.
}

// ************************************************************************************************************************ //
// **
// ** XML Document
// **
// **
// **
// ************************************************************************************************************************ //

CXMLDocument::CXMLDocument()
: CXMLMultiNode( CXMLNode::DOCUMENT )
{
}
CXMLDocument::~CXMLDocument()
{
}

const char* CXMLDocument::Parse( const char *pszBegin, const char *pszEnd )
{
	ResetOptimalPos();
	// Parse away, at the document level. Since a document contains nothing but other tags, most of what happens
	// here is skipping white space.
	// In this variant (as opposed to stream and Parse) we read everything we can.
	if ( !pszBegin || !*pszBegin )
		return 0;
  const char *p = SkipWhiteSpace( pszBegin );
	if ( !p )
		return 0;

	while ( *p )
	{
		if ( CXMLNode *pNode = Identify(p, pszEnd) )
		{
			p = pNode->Parse( p, pszEnd );
			AddChild( pNode );
		}
		else
			break;
		p = SkipWhiteSpace( p );
	}
	// All is well.
	return p;
}

void CXMLDocument::Store( NLXML_STREAM &stream, const string &szIndention ) const
{
	for ( CXMLMultiNode::const_iterator it = begin(); it != end(); ++it )
	{
		(*it)->Store( stream, szIndention );
		stream << szEndOfLine;
		// Special rule for streams: stop after the root element.
		// The stream in code will only read one element, so don't write more than one.
		if ( (*it)->GetType() == CXMLNode::ELEMENT )
			return;
	}
}

CXMLElement *CXMLDocument::GetRootElement()
{
	const int nNumChildren = CountChildren();
	NLXML::CXMLNode *pRootNode = 0;
	for ( int i = 0; i < nNumChildren; ++i )
	{
		pRootNode = GetChild( i );
		if ( pRootNode->GetType() != NLXML::CXMLNode::ELEMENT )
			pRootNode = 0;
		else
			break;
	}
	//
	return pRootNode != 0 ? static_cast<CXMLElement *>( pRootNode ) : 0;
}

}

