#include "stdafx.h"
#include "Text.h"
#include "FilePath.h"
#include "VFSOperations.h"

namespace NText
{
typedef std::unordered_map<NFile::CFilePath, std::wstring> CUnicodeTextMap;
static CUnicodeTextMap unicodeTextMap;

bool LoadUnicodeText( std::wstring *pwszRes, CDataStream *pStream )
{
	if ( pStream->IsOk() )
	{
		WORD wSignature = 0;
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
			pwszRes->resize( nSize / sizeof(wchar_t) );
			pStream->Read( &((*pwszRes)[0]), nSize );
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

