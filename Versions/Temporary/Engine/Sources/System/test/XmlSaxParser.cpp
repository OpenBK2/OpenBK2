// NLXML::ParseXML, the SAX parser libdb reads each object's header with.
//
// The document and the events expected from it are Nival's, from TestDB's
// TestXmlSaxParser. The document is inline rather than a file so that a checkout
// that converts line endings cannot change what the comment spans.

// The standard headers the engine's stdafx.h prelude would supply, which
// Streams.h and XMLSAXParser.h rely on without including
#include <cstdint>
#include <cstring>
#include <list>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/System.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/XMLSAXParser.h"

#include <gtest/gtest.h>

namespace {

// Every callback, written as one line, so a mismatch shows as a diff of lines.
class CRecordingVisitor : public NLXML::IXmlSaxVisitor
{
	OBJECT_NOCOPY_METHODS( CRecordingVisitor );
public:
	std::vector<std::string> events;

	bool VisitHeader( const std::string &szVersion, const std::string &szEncoding, const std::string &szStandalone ) override
	{
		events.push_back( "header " + szVersion + " " + szEncoding + " [" + szStandalone + "]" );
		return true;
	}
	bool VisitComment( const std::string &szText ) override
	{
		events.push_back( "comment [" + szText + "]" );
		return true;
	}
	bool VisitChunkStart( const std::string &szName ) override
	{
		events.push_back( "start " + szName );
		return true;
	}
	bool VisitAttribute( const std::string &szName, const std::string &szValue ) override
	{
		events.push_back( "attr " + szName + "=[" + szValue + "]" );
		return true;
	}
	bool VisitText( const std::string &szText ) override
	{
		events.push_back( "text [" + szText + "]" );
		return true;
	}
	bool VisitChunkFinish( const std::string &szName ) override
	{
		events.push_back( "finish " + szName );
		return true;
	}
};

std::vector<std::string> Parse( const std::string &szXml, bool *pbResult )
{
	CMemoryStream stream;
	stream.Write( szXml.data(), int( szXml.size() ) );
	stream.Seek( 0 );
	CObj<CRecordingVisitor> pVisitor = new CRecordingVisitor();
	*pbResult = NLXML::ParseXML( pVisitor, &stream );
	return pVisitor->events;
}

// Covers attributes in both quote styles, a quote inside the other kind, an
// empty element with attributes, the five predefined entities and a character
// reference in text, text that is only spaces, a self-closed element, and a
// comment holding markup that must not be parsed.
const char szDocument[] =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<!-- a comment before the root -->\n"
	"<Data attr=\"wer\" zxc='as \" we' test=\"just a test\">\n"
	"\t<as attr1=\"val1\" attr2=\"val2\"/>\n"
	"\t<a as=\"we\" as1=\"we1\">\n"
	"\t\t<NewText>  this is a text with syschars &amp; &lt; &gt; &quot; &apos; &#xA9;!</NewText>\n"
	"\t\t<NewText2>   </NewText2>\n"
	"\t</a>\n"
	"\t<a>\n"
	"\t\t<NewText>  this is a text with syschars &amp; &lt; &gt; &quot; &apos; &#xA9;!</NewText>\n"
	"\t\t<NewText2>   </NewText2>\n"
	"\t</a>\n"
	"\t<a />\n"
	"    <!--\n"
	"\t<b as=\"we\" as1=\"we1\">\n"
	"& < > ' \"\n"
	"\t</b>\n"
	"    -->\n"
	"</Data>\n";

TEST( XmlSaxParser, ReportsEveryPartOfTheDocumentInOrder )
{
	// what &#xA9; decodes to
	const std::string szCopyright = "\xC2\xA9";
	const std::string szText = "text [  this is a text with syschars & < > \" ' " + szCopyright + "!]";
	const std::vector<std::string> expected = {
		"header 1.0 UTF-8 []",
		"comment [ a comment before the root ]",
		"start Data",
		"attr attr=[wer]",
		"attr zxc=[as \" we]",
		"attr test=[just a test]",
		"start as",
		"attr attr1=[val1]",
		"attr attr2=[val2]",
		"finish as",
		"start a",
		"attr as=[we]",
		"attr as1=[we1]",
		"start NewText",
		szText,
		"finish NewText",
		// whitespace-only text is not reported
		"start NewText2",
		"finish NewText2",
		"finish a",
		"start a",
		"start NewText",
		szText,
		"finish NewText",
		"start NewText2",
		"finish NewText2",
		"finish a",
		"start a",
		"finish a",
		"comment [\n\t<b as=\"we\" as1=\"we1\">\n& < > ' \"\n\t</b>\n    ]",
		"finish Data",
	};
	bool bResult = false;
	const std::vector<std::string> events = Parse( szDocument, &bResult );
	EXPECT_TRUE( bResult );
	EXPECT_EQ( events, expected );
}

// A character reference names a code point; the text gets its UTF-8 encoding,
// whatever its length.
TEST( XmlSaxParser, CharacterReferencesDecodeToUTF8 )
{
	bool bResult = false;
	const std::vector<std::string> events = Parse( "<a>&#x41;&#xA9;&#x416;&#x20AC;&#x1F600;</a>", &bResult );
	EXPECT_TRUE( bResult );
	const std::vector<std::string> expected = {
		"start a",
		"text [A\xC2\xA9\xD0\x96\xE2\x82\xAC\xF0\x9F\x98\x80]",
		"finish a",
	};
	EXPECT_EQ( events, expected );
}

} // namespace
