#include "stdafx.h"

#include "XdbValidation.h"

#include <fmt/format.h>
#include <wx/mstream.h>
#include <wx/xml/xml.h>

#include <cstring>

// The XDB check in wx: wxXmlDocument, and under it expat, which reads the whole
// document and stops at the first thing that makes it malformed. It replaces
// MSXML's DOMDocument60, run as it was: no validation against a schema, no DTD,
// and the bytes taken as UTF-8.

namespace NXdbValidation
{
	bool CheckDocument( const char *pData, int nSize, std::string *pszRootName, std::string *pszError )
	{
		// The engine reads XDB text as UTF-8 whatever the file says, so anything
		// else is refused before parsing, as it was. A NUL means UTF-16 or some
		// other wide encoding; FromUTF8 gives back an empty string for bytes that
		// are not UTF-8, where MSXML's caller used MultiByteToWideChar with
		// MB_ERR_INVALID_CHARS.
		if ( nSize <= 0 || memchr( pData, 0, nSize ) != nullptr || wxString::FromUTF8( pData, nSize ).empty() )
		{
			*pszError = "XDB files must use UTF-8 text encoding.";
			return false;
		}
		// wx parses the file's own bytes, a byte-order mark included; MSXML was
		// handed converted text and needed the mark cut off first.
		wxMemoryInputStream stream( pData, nSize );
		wxXmlDocument document;
		wxXmlParseError error;
		if ( !document.Load( stream, wxXMLDOC_NONE, &error ) )
		{
			*pszError = fmt::format( "Invalid XDB/XML at line {}: {}", error.line, std::string( error.message.utf8_str() ) );
			return false;
		}
		// expat, unlike MSXML's loadXML, decodes by the encoding the declaration
		// names. Every shipped XDB declares UTF-8; one that names another
		// encoding would be read one way here and another by the engine.
		if ( document.GetFileEncoding().CmpNoCase( "UTF-8" ) != 0 )
		{
			*pszError = "XDB files must use UTF-8 text encoding.";
			return false;
		}
		// MSXML ran with ProhibitDTD, which fails the parse at a DOCTYPE. expat
		// has no such switch, so the DOCTYPE it found is refused afterwards. Its
		// built-in limits on entity expansion (expat 2.4 and later) cover what
		// ProhibitDTD protected against in the meantime.
		if ( document.GetDoctype().IsValid() )
		{
			*pszError = "Invalid XDB/XML: a DOCTYPE declaration is not allowed.";
			return false;
		}
		const wxXmlNode *pRoot = document.GetRoot();
		if ( pRoot == nullptr || pRoot->GetName().empty() )
		{
			*pszError = "The XDB has no resource root element.";
			return false;
		}
		*pszRootName = std::string( pRoot->GetName().utf8_str() );
		return true;
	}
}
