#include "stdafx.h"
#include "XMLSAXParser.h"
#include "Misc/StrProc.h"

namespace NLXML
{

enum EXmlNodeType
{
	XML_NODE_TYPE_NONE,
	XML_NODE_TYPE_UNKNOWN,
	XML_NODE_TYPE_HEADER,
	XML_NODE_TYPE_COMMENT,
	XML_NODE_TYPE_ELEMENT,
};

static bool IsWhiteSpace( const char chr )
{
	return ( (chr >= 0x09 && chr <= 0x0D) || chr == 0x20 );
}

// ************************************************************************************************************************ //
// **
// ** buffered stream helper class
// **
// **
// **
// ************************************************************************************************************************ //

class CBufferedStream
{
	enum { BUFFER_SIZE = 4096 };
	char buffer[BUFFER_SIZE];
	int nStreamSize;
	int nBytesRead;
	int nCurrPos;
	int nBytesInBuffer;
	CDataStream *pStream;
	//
	bool IsBufferEmpty() const { return nCurrPos >= nBytesInBuffer; }
	void ReadPortion()
	{
		const int nToRead = Min( int(BUFFER_SIZE), nStreamSize - nBytesRead );
		if ( nToRead == 0 )
			return;
		pStream->Read( buffer, nToRead );
		nBytesRead += nToRead;
		nBytesInBuffer = nToRead;
		nCurrPos = 0;
	}
public:
	CBufferedStream( CDataStream *_pStream )
		: nStreamSize( _pStream->GetSize() ), nBytesRead( 0 ), nCurrPos( 0 ), nBytesInBuffer( 0 ), pStream( _pStream )
	{
		ReadPortion();
	}
	//
	bool SkipWhiteSpaces()
	{
		while ( 1 )
		{
			if ( IsEnd() )
				return false;
			if ( !IsWhiteSpace(buffer[nCurrPos]) )
				return true;
			Next();
		}
	}
	//
	bool ReadUntilNonspace( string *pszBuff )
	{
		while ( 1 )
		{
			if ( IsEnd() )
				return false;
			if ( !IsWhiteSpace(buffer[nCurrPos]) )
				return true;
			(*pszBuff) += buffer[nCurrPos];
			Next();
		}
	}
	//
	int GetNumCharsToEnd() const { return ( nStreamSize - nBytesRead ) + ( nBytesInBuffer - nCurrPos ); }
	//
	void Next()
	{
		++nCurrPos;
		if ( IsBufferEmpty() )
			ReadPortion();
	}
	char GetChar() const { return buffer[nCurrPos]; }
	bool CheckNextChar( const char cCheckChar )
	{
		++nCurrPos;
		if ( IsBufferEmpty() )
		{
			--nCurrPos;
			// read one symbol from the stream and check it
			char chr;
			pStream->Read( &chr, 1 );
			pStream->Seek( pStream->GetPosition() - 1 );
			return chr == cCheckChar;
		}
		--nCurrPos;
		return buffer[nCurrPos + 1] == cCheckChar;
	}
	//
	bool IsEnd() const { return nBytesRead >= nStreamSize && IsBufferEmpty(); }
};

// ************************************************************************************************************************ //
// **
// ** name and text reading functions
// **
// **
// **
// ************************************************************************************************************************ //

bool ParseXMLImpl( IXmlSaxVisitor *pVisitor, CBufferedStream &stream );
bool ParseElement( IXmlSaxVisitor *pVisitor, CBufferedStream &stream );

bool ReadName( string *pszName, CBufferedStream &stream )
{
	pszName->clear();
	pszName->reserve( 32 );
	// Names start with letters or underscores. After that, they can be letters, underscores, numbers, hyphens, or colons. 
	// (Colons are valid only for namespaces, but this parser can't tell namespaces from names.)
	if ( !stream.IsEnd() && ( isalpha((unsigned char) stream.GetChar()) || stream.GetChar() == '_' ) )
	{
		while ( !stream.IsEnd() && ( isalnum((unsigned char ) stream.GetChar()) || 
			                           stream.GetChar() == '_' || 
																 stream.GetChar() == '-' || 
																 stream.GetChar() == ':' ) )
		{
			*pszName += stream.GetChar();
			stream.Next();
		}
		return true;
	}
	return false;
}

// "amp;"		'&'
// "lt;"		'<'
// "gt;"		'>'
// "quot;"	'\"'
// "apos;"	'\''
char ReadSysChar( string *pszBuff, CBufferedStream &stream )
{
	char chr = 0;
	stream.Next();
	switch ( stream.GetChar() )
	{
	// &apos; &amp;
	case 'a':
		stream.Next();
		if ( stream.GetChar() == 'm' )
		{
			stream.Next();
			if ( stream.GetChar() != 'p' )
			{
				pszBuff->resize( 3 );
				(*pszBuff)[0] = '&';
				(*pszBuff)[1] = 'a';
				(*pszBuff)[2] = 'm';
				return char( 0xff );
			}
			stream.Next();
			if ( stream.GetChar() != ';' )
			{
				pszBuff->resize( 4 );
				(*pszBuff)[0] = '&';
				(*pszBuff)[1] = 'a';
				(*pszBuff)[2] = 'm';
				(*pszBuff)[3] = 'p';
				return char( 0xff );
			}
			stream.Next();
			return '&';
		}
		else if ( stream.GetChar() == 'p' )
		{
			stream.Next();
			if ( stream.GetChar() != 'o' )
			{
				pszBuff->resize( 3 );
				(*pszBuff)[0] = '&';
				(*pszBuff)[1] = 'a';
				(*pszBuff)[2] = 'p';
				return char( 0xff );
			}
			stream.Next();
			if ( stream.GetChar() != 's' )
			{
				pszBuff->resize( 4 );
				(*pszBuff)[0] = '&';
				(*pszBuff)[1] = 'a';
				(*pszBuff)[2] = 'p';
				(*pszBuff)[3] = 'o';
				return char( 0xff );
			}
			stream.Next();
			if ( stream.GetChar() != ';' )
			{
				pszBuff->resize( 5 );
				(*pszBuff)[0] = '&';
				(*pszBuff)[1] = 'a';
				(*pszBuff)[2] = 'p';
				(*pszBuff)[3] = 'o';
				(*pszBuff)[4] = 's';
				return char( 0xff );
			}
			stream.Next();
			return '\'';
		}
		else
		{
			pszBuff->resize( 2 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'a';
			return char( 0xff );
		}
	// &lt;
	case 'l':
		stream.Next();
		if ( stream.GetChar() != 't' )
		{
			pszBuff->resize( 2 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'l';
			return char( 0xff );
		}
		stream.Next();
		if ( stream.GetChar() != ';' )
		{
			pszBuff->resize( 3 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'l';
			(*pszBuff)[2] = 't';
			return char( 0xff );
		}
		stream.Next();
		return '<';
	// &gt;
	case 'g':
		stream.Next();
		if ( stream.GetChar() != 't' )
		{
			pszBuff->resize( 2 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'g';
			return char( 0xff );
		}
		stream.Next();
		if ( stream.GetChar() != ';' )
		{
			pszBuff->resize( 3 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'g';
			(*pszBuff)[2] = 't';
			return char( 0xff );
		}
		stream.Next();
		return '>';
	// &quot;
	case 'q':
		stream.Next();
		if ( stream.GetChar() != 'u' )
		{
			pszBuff->resize( 2 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'q';
			return char( 0xff );
		}
		stream.Next();
		if ( stream.GetChar() != 'o' )
		{
			pszBuff->resize( 3 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'q';
			(*pszBuff)[2] = 'u';
			return char( 0xff );
		}
		stream.Next();
		if ( stream.GetChar() != 't' )
		{
			pszBuff->resize( 4 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'q';
			(*pszBuff)[2] = 'u';
			(*pszBuff)[3] = 'o';
			return char( 0xff );
		}
		stream.Next();
		if ( stream.GetChar() != ';' )
		{
			pszBuff->resize( 5 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = 'q';
			(*pszBuff)[2] = 'u';
			(*pszBuff)[3] = 'o';
			(*pszBuff)[4] = 't';
			return char( 0xff );
		}
		stream.Next();
		return '\"';
	// &#code
	case '#':
		stream.Next();
		if ( stream.GetChar() != 'x' )
		{
			pszBuff->resize( 3 );
			(*pszBuff)[0] = '&';
			(*pszBuff)[1] = '#';
			(*pszBuff)[2] = 'x';
			return char( 0xff );
		}
		stream.Next();
		while ( !stream.IsEnd() && stream.GetChar() != ';' )
		{
			chr = (chr << 4) | NStr::HexSymbolToHalfByte( stream.GetChar() );
			stream.Next();
		}
		if ( stream.IsEnd() || stream.GetChar() != ';' )
			return char( 0xff );
		else
		{
			stream.Next();
			return chr;
		}
	// non-tag
	default:
		pszBuff->resize( 1 );
		(*pszBuff)[0] = '&';
		return char(0xff);
	}
}

bool ReadText( string *pszText, CBufferedStream &stream, const char *pszEndTag )
{
	while ( !stream.IsEnd() )
	{
		char chr = stream.GetChar();
		if ( chr == '&' )
		{
			if ( stream.GetNumCharsToEnd() < 3 )
				return false;
			string szSysChar;
			chr = ReadSysChar( &szSysChar, stream );
			if ( chr == char(0xff) )
			{
				if ( szSysChar.empty() )
					return false;
				(*pszText) += szSysChar;
			}
			else
				(*pszText) += chr;
		}
		else if ( chr == pszEndTag[0] )
		{
			// parse end tag
			char buff[8];
			memset( buff, 0, sizeof(buff) );
			buff[0] = chr;
			stream.Next();
			for ( int i = 1; 1; ++i, stream.Next() )
			{
				if ( pszEndTag[i] == 0 )
					return true;
				if ( stream.IsEnd() )
					return false;
				chr = stream.GetChar();
				if ( chr != pszEndTag[i] )
				{
					(*pszText) += buff;
					break;
				}
			}
		}
		else
		{
			(*pszText) += chr;
			stream.Next();
		}
	}
	return false;
}

// ************************************************************************************************************************ //
// **
// ** XML node type identification
// **
// **
// **
// ************************************************************************************************************************ //

// What is this thing? 
// - Elements start with a letter or underscore, but xml is reserved (min element node <a/>)
// - Comments: <!-- (min comment node is <!---->)
// - Declaration: <?xml (min declaration node is <?xml?>)
// - Everthing else is unknown
EXmlNodeType Identify( CBufferedStream &stream )
{
	if ( stream.SkipWhiteSpaces() == false )
		return XML_NODE_TYPE_NONE;
	const int nNumChars = stream.GetNumCharsToEnd();
	if ( nNumChars < 4 /*|| stream.GetChar() != '<'*/ )
		return XML_NODE_TYPE_NONE;
	//
	if ( stream.GetChar() == '<' )
		stream.Next();
	char chr = stream.GetChar();
	switch ( chr )
	{
	case '?':	// header: <?xml
		if ( nNumChars < 7 )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		if ( stream.GetChar() != 'x' )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		if ( stream.GetChar() != 'm' )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		if ( stream.GetChar() != 'l' )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		return XML_NODE_TYPE_HEADER;

	case '!': // comment: <!--
		if ( nNumChars < 7 )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		if ( stream.GetChar() != '-' )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		if ( stream.GetChar() != '-' )
			return XML_NODE_TYPE_NONE;
		stream.Next();
		return XML_NODE_TYPE_COMMENT;

	default:
		if ( nNumChars < 4 )
			return XML_NODE_TYPE_NONE;
		return XML_NODE_TYPE_ELEMENT;
	}
}

// ************************************************************************************************************************ //
// **
// ** different component parsers
// **
// **
// **
// ************************************************************************************************************************ //

// attribute parser
bool ParseAttribute( string *pszName, string *pszText, CBufferedStream &stream )
{
	if ( stream.SkipWhiteSpaces() == false )
		return false;
	// read attribute name
	if ( ReadName( pszName, stream ) == false )
		return false;
	// skip '='
	if ( stream.SkipWhiteSpaces() == false || stream.GetChar() != '=' )
		return false;
	stream.Next();
	if ( stream.SkipWhiteSpaces() == false )
		return false;
	// read attribute value
	if ( stream.GetChar() == '\'' )	// try text in single quotes
	{
		stream.Next();
		if ( ReadText( pszText, stream, "\'" ) == false )
			return false;
	}
	else if ( stream.GetChar() == '\"' )	// try text in double quotes
	{
		stream.Next();
		if ( ReadText( pszText, stream, "\"" ) == false )
			return false;
	}
	else
	{
		// All attribute values should be in single or double quotes.
		// But this is such a common error that the parser will try its best, even without them.
		pszText->clear();
		pszText->reserve( 16 );
		while ( !stream.IsEnd() && !isspace(stream.GetChar()) && stream.GetChar() != '/' && stream.GetChar() != '>' )
		{
			(*pszText) += stream.GetChar();
			stream.Next();
		}
	}
	return !stream.IsEnd();
}

// header parse
bool ParseHeader( IXmlSaxVisitor *pVisitor, CBufferedStream &stream )
{
	string szVersion = "1.0", szEncoding = "UTF-8", szStandalone = "";
	while ( stream.SkipWhiteSpaces() != false && stream.GetChar() != '?' )
	{
		string szName, szText;
		if ( ParseAttribute(&szName, &szText, stream) == false )
			return false;
		if ( szName == "version" )
			szVersion = szText;
		else if ( szName == "encoding" )
			szEncoding = szText;
		else if ( szName == "standalone" )
			szStandalone = szText;
	}
	//
	if ( stream.IsEnd() || pVisitor->VisitHeader( szVersion, szEncoding, szStandalone ) == false )
		return false;
	// skip close tag: ?>
	if ( stream.GetChar() != '?' )
		return false;
	stream.Next();
	if ( stream.IsEnd() )
		return false;
	if ( stream.GetChar() != '>' )
		return false;
	stream.Next();
	//
	return !stream.IsEnd();
}

// comment block
bool ParseComment( IXmlSaxVisitor *pVisitor, CBufferedStream &stream )
{
	string szText;
	if ( ReadText( &szText, stream, "-->" ) == false )
		return false;
	return pVisitor->VisitComment( szText );
}

bool ParseElementValue( IXmlSaxVisitor *pVisitor, CBufferedStream &stream )
{
	// read in text and elements in any order.
	string szBuffer;
	while ( stream.ReadUntilNonspace(&szBuffer) != false )
	{
		if ( stream.GetChar() == '/' )
			return true;
		else if ( stream.GetChar() != '<' )
		{
			// take what we have - read an element's text
			string szText;
			if ( ReadText(&szText, stream, "<") == false || pVisitor->VisitText(szBuffer + szText) == false )
				return false;
		}
		else
		{
			// check for terminator tag
			if ( stream.CheckNextChar('/') == true )
			{
				stream.Next();
				return true;
			}
			// have we hit a new element or an end tag? ("</")
			const EXmlNodeType eType = Identify( stream );
			// subelement can be a comment or an element
			if ( eType == XML_NODE_TYPE_ELEMENT )
			{
				if ( ParseElement(pVisitor, stream) == false )
					return false;
			}
			else if ( eType == XML_NODE_TYPE_COMMENT )
			{
				if ( ParseComment(pVisitor, stream) == false )
					return false;
			}
			else
				return false;
		}
		//
		szBuffer.resize( 0 );
	}
	return !stream.IsEnd();
}

bool ParseElement( IXmlSaxVisitor *pVisitor, CBufferedStream &stream )
{
	if ( stream.SkipWhiteSpaces() == false )
		return false;
	// read chunk start name
	string szName;
	if ( ReadName(&szName, stream) == false || pVisitor->VisitChunkStart(szName) == false )
		return false;
	// read attributes
	while ( stream.SkipWhiteSpaces() != false )
	{
		// check for chunk end
		if ( stream.GetChar() == '/' )
		{
			stream.Next();
			if ( stream.IsEnd() )
				return false;
			if ( stream.GetChar() != '>' || pVisitor->VisitChunkFinish(szName) == false )
				return false;
			stream.Next();
			return !stream.IsEnd();
		} 
		else if ( stream.GetChar() == '>' )
		{
			stream.Next();
			if ( ParseElementValue(pVisitor, stream) == false || stream.IsEnd() || stream.GetChar() != '/' )
				return false;
			// check for the end tag
			stream.Next();
			string szEndName;
			if ( stream.IsEnd() || ReadName(&szEndName, stream) == false || pVisitor->VisitChunkFinish(szEndName) == false )
				return false;
			stream.Next();
			//
			return szEndName == szName;
		}
		else
		{
			string szName, szText;
			if ( ParseAttribute(&szName, &szText, stream) == false || pVisitor->VisitAttribute(szName, szText) == false )
				return false;
		}
	}
	return !stream.IsEnd();
}

// ************************************************************************************************************************ //
// **
// ** general parser method
// **
// **
// **
// ************************************************************************************************************************ //

bool ParseXMLImpl( IXmlSaxVisitor *pVisitor, CBufferedStream &stream )
{
	while ( !stream.IsEnd() )
	{
		const EXmlNodeType eType = Identify( stream );
		switch ( eType )
		{
		case XML_NODE_TYPE_HEADER:
			if ( ParseHeader( pVisitor, stream ) == false )
				return false;
			break;

		case XML_NODE_TYPE_COMMENT:
			if ( ParseComment( pVisitor, stream ) == false )
				return false;
			break;

		case XML_NODE_TYPE_ELEMENT:
			if ( ParseElement( pVisitor, stream ) == false )
				return false;
			break;

		default:
			return false;
		}
	}
	return true;
}

bool ParseXML( IXmlSaxVisitor *pVisitor, CDataStream *pStream )
{
	CBufferedStream stream( pStream );
	//
	ParseXMLImpl( pVisitor, stream );
	//
	return true;
}

}


