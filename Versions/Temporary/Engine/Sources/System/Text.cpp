#include "stdafx.h"

#include "port/unicode.h"

#include "Text.h"
#include "FilePath.h"
#include "VFSOperations.h"

#include <cstdint>

namespace NText
{
typedef std::unordered_map<NFile::CFilePath, std::wstring> CUnicodeTextMap;
static CUnicodeTextMap unicodeTextMap;

bool LoadUnicodeText( std::wstring *pwszRes, CDataStream *pStream )
{
	if ( pStream->IsOk() )
	{
		uint16_t wSignature = 0;
		pStream->Read( &wSignature, 2 );
		if ( wSignature != UNICODE_SIGNATURE )
			return false;
		const int nSize = pStream->GetSize() - 2;
		if ( nSize < 0 )
			return false;
		if ( nSize == 0 )
			pwszRes->clear();
		else
		{
			// The BOM says UTF-16LE, so the body is two bytes per character and
			// not sizeof( wchar_t ). The two are the same on Windows and this is
			// the copy it always was; off Windows wchar_t is four bytes, so the
			// old reinterpret packed two characters into each wchar_t, dropped
			// half the string, and overran the buffer by two bytes whenever the
			// character count was odd.
			std::vector<char> buffer( nSize );
			pStream->Read( &buffer[0], nSize );
			*pwszRes = UTF16LEToWide( &buffer[0], buffer.size() );
		}
		return true;
	}
	else
	{
		DebugTrace( "Can't load text from empty stream" );
		return false;
	}
}

bool LoadUnicodeText( std::wstring *pwszRes, const std::string &szFileName )
{
	if ( szFileName.empty() )
		return false;
	CFileStream stream( NVFS::GetMainVFS(), szFileName );
	return LoadUnicodeText( pwszRes, &stream );
}

const std::wstring &GetText( const std::string &szTextFileName )
{
	const NFile::CFilePath filePath = szTextFileName;
	CUnicodeTextMap::const_iterator pos = unicodeTextMap.find( filePath );
	if ( pos != unicodeTextMap.end() )
		return pos->second;
	//
	std::wstring wszText;
	if ( LoadUnicodeText(&wszText, filePath) == false )
		unicodeTextMap[filePath] = L"";
	else
		unicodeTextMap[filePath] = wszText;
	//
	return unicodeTextMap[filePath];
}

void Reload( const std::string &szTextFileName )
{
	const NFile::CFilePath filePath = szTextFileName;
	std::wstring wszText;
	if ( LoadUnicodeText(&wszText, filePath) == false )
		unicodeTextMap[filePath] = L"";
	else
		unicodeTextMap[filePath] = wszText;
}

}

