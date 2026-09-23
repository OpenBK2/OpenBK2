#include "stdafx.h"
#include <fmt/format.h>

#include "Tools_Resources.h"
#include "Misc/StrProc.h"
#include "System/VFS.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/WinVFS.h"
#include "System/VFSOperations.h"

#include "port/unicode.h"

#include <cstdint>
#include "port/process.h"

namespace
{
	// Whether this names a file on disk rather than an entry in the VFS.
	//
	// The test used to be "does it contain a colon", which is a drive letter and
	// nothing else on Windows: "C:\bk2\Editor\..." is a file, "bin\Textures\17"
	// is a VFS name. Off Windows an absolute path has no colon in it, so every
	// one of them came out a VFS name and was then taken as relative to the VFS
	// root: the editor's icon cache and its dialog states went to
	// Data/home/<user>/bk2/... instead of to the install, and nothing said so.
	//
	// Asking whether the path is absolute says the same thing on Windows, where
	// only a drive letter or a leading separator makes one, and says it off
	// Windows too.
	bool IsFilePath( const std::string &rszPath )
	{
		return !rszPath.empty() && !NFile::IsPathRelative( rszPath );
	}
}


void OpenStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath )
{
	if ( pStreamHolder )
	{
		if ( !IsFilePath( rszTextPath ) && NVFS::GetMainVFS() )
		{
			pStreamHolder->pStream = new CFileStream( NVFS::GetMainVFS(), rszTextPath );
		}
		else
		{
			pStreamHolder->pStream = new CFileStream( rszTextPath, CFileStream::WIN_READ_ONLY );
		}
	}
}


void CreateStreamHolder( SFileStreamHolder *pStreamHolder, const std::string &rszTextPath )
{
	if ( pStreamHolder )
	{
		if ( !IsFilePath( rszTextPath ) && NVFS::GetMainFileCreator() )
		{
			pStreamHolder->pStream = new CFileStream( NVFS::GetMainFileCreator(), rszTextPath );
		}
		else
		{
			pStreamHolder->pStream = new CFileStream( rszTextPath, CFileStream::WIN_CREATE );
		}
	}
}


bool NormalizePath( std::string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, const std::string &rszPathPrefix, bool *pbAbsolutePath )
{
	bool bResult = true;
	NI_ASSERT( pszPath != 0, "NormalizePath() pszPath == 0" );
	if ( pszPath )
	{
		// Every separator in here was a backslash, written out four times: the
		// fold below, the one appended to the prefix, the one stripped off the
		// front of the name and the one appended to a directory. The name this
		// builds is opened as a file, and off Windows a backslash in it is an
		// ordinary character, so the prefix and the name were joined by one and
		// the whole thing named nothing: "/home/user/bk2/\Editor\MapInfoEditor".
		//
		// Folding towards PATH_SEPARATOR keeps the normalising, which is still
		// needed because a name can arrive with either separator in it, and
		// points the result the way FilePath.h says paths go in this tree. The
		// one caller that turns the result back into a database name rather than
		// a path is unaffected: CDBID hashes and compares with a backslash and a
		// forward slash folded together, as does NFile::ComparePathEq.
		NStr::ReplaceAllChars( pszPath, '\\', NFile::PATH_SEPARATOR );
		// Проверяем наличие слеша в конце пути
		std::string szPathPrefix = rszPathPrefix;
		NFile::AppendSlash( &szPathPrefix, NFile::PATH_SEPARATOR );
		// Проверяем наличие отсутствия слеша в начале имени
		std::string szPath = ( *pszPath );
		if ( ( szPath.size() > 0 ) && NFile::IsFolderSeparator( szPath[0] ) )
		{
			szPath = szPath.substr( 1 );
		}
		// Если это каталог - необходимо проверить наличие последнего слеша
		if ( !bFile )
		{
			NFile::AppendSlash( &szPath, NFile::PATH_SEPARATOR );
		}

		// Расширяем имя файла до полного или отрезаем ненужное
		std::string szLCPathPrefix = szPathPrefix;
		std::string szLCPath = szPath;
		NStr::ToLowerASCII( &szLCPathPrefix );
		NStr::ToLowerASCII( &szLCPath );
		// Имя по которому будет проверятся наличие фала на диске
		std::string szCheckPath;
		//
		if ( !szPathPrefix.empty() ) 
		{
			if ( szLCPath.compare( 0, szLCPathPrefix.size(), szLCPathPrefix ) == 0 )
			{
				szCheckPath = szPath;
				if ( !bReturnAbsolutePath )
				{
					szPath = szPath.substr( szPathPrefix.size() );
				}
			}
			else
			{
				if ( ( szPath.size() < 2 ) || ( szPath[1] != ':' ) )
				{
					szCheckPath = szPathPrefix + szPath;
					if ( bReturnAbsolutePath )
					{
						szPath = szPathPrefix + szPath;
					}
				}
				else
				{
					szCheckPath = szPath;
				}
			}
		}
		//
		if ( bExists )
		{
			if ( szCheckPath.empty() )
			{
				bResult = false;
			}
			else
			{
				if ( bFile )
				{
					// A file and not a directory, which is what the attribute test
					// spelled out: DoesFileExist is is_regular_file.
					if ( !NFile::DoesFileExist( szCheckPath ) )
					{
						bResult = false;
					}
				}
				else
				{
					szCheckPath = szCheckPath.substr( 0, szCheckPath.size() - 1 );
					if ( !NFile::DoesFolderExist( szCheckPath ) )
					{
						bResult = false;
					}
				}
			}
		}
		( *pszPath ) = szPath;
		//
		if ( pbAbsolutePath )
		{
			// IsPathRelative, which knows both kinds of absolute path: a drive
			// letter and a leading separator. What stood here tested for a drive
			// letter and then, through a precedence slip, tested szPath[2] again
			// on its own - past the end of any path shorter than three
			// characters, and for a backslash this function no longer produces.
			// No caller asks for this today; all three pass a null pointer.
			( *pbAbsolutePath ) = !szPath.empty() && !NFile::IsPathRelative( szPath );
		}
	}
	return bResult;
}


bool IsValidFileName( const std::string &rszFileName, bool bAbsolutePath )
{
	if ( rszFileName.find_first_of( "*?<>|" ) != std::string::npos )
	{
		return false;
	}
	int nPos = rszFileName.find( ':' );
	if ( ( nPos != std::string::npos ) && ( nPos != std::string::npos != 1 ) )
	{
		return false;			
	}
	else if ( nPos == 1 )
	{
		if ( bAbsolutePath )
		{
			nPos = rszFileName.find( nPos, ':' );
			if ( nPos != std::string::npos )
			{
				return false;			
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

/**

bool CheckLatestBINResource( const std::string &rszResourceFileName, const std::string &rszXMLExtention, const std::string &rszBINExtention )
{
	try
	{
		NVFS::SFileStats streamXMLStats, streamBINStats;
		{
			// get stats from Binary and XML files
			Zero( streamXMLStats );
			NVFS::GetWinFileStats( &streamXMLStats, ( rszResourceFileName + rszXMLExtention ).c_str() );
			Zero( streamBINStats );
			NVFS::GetWinFileStats( &streamBINStats, ( rszResourceFileName + rszBINExtention ).c_str() );
		}		
		return ( streamBINStats.mtime >= streamXMLStats.mtime );
	}
	catch ( ... )
	{
		return true;
	}
}
/**/


bool SEnumFolderStructureParameter::IsFolderRelative( const std::string &rszFolder, const std::string &rszRelativeFolder )
{
	return IsFolderRelative( enumFolderMap, rszFolder, rszRelativeFolder );
}


void SEnumFolderStructureParameter::SetRelativeFolder( const std::string &rszFolder, const std::string &rszRelativeFolder )
{
	SetRelativeFolder( &enumFolderMap, rszFolder, rszRelativeFolder );
}


bool SEnumFolderStructureParameter::IsFolderRelative( const CEnumFolderMap &rEnumFolderMap, const std::string &rszFolder, const std::string &rszRelativeFolder )
{
	CEnumFolderMap::const_iterator folderIterator = rEnumFolderMap.find( rszFolder );
	if ( folderIterator != rEnumFolderMap.end() )
	{
		return ( folderIterator->second.find( rszRelativeFolder ) != folderIterator->second.end() );
	}
	return false;
}


void SEnumFolderStructureParameter::SetRelativeFolder( CEnumFolderMap *pEnumFolderMap, const std::string &rszFolder, const std::string &rszRelativeFolder )
{
	NI_ASSERT( pEnumFolderMap != 0, fmt::format( "Wrong parameter: {:x}\n", pEnumFolderMap ) );
	if ( pEnumFolderMap )
	{
		( *pEnumFolderMap )[rszFolder][rszRelativeFolder] = 0;
	}
}


void EnumFilesInDataStorage( std::vector<SEnumFilesInDataStorageParameter> *pParameters, SEnumFolderStructureParameter *pEnumFolderStructureParameter )
{
	if ( pParameters || pEnumFolderStructureParameter )
	{
		if ( pParameters )
		{
			for ( int nParameterElement = 0; nParameterElement < pParameters->size(); ++nParameterElement )
			{
				SEnumFilesInDataStorageParameter &rParameter = ( *pParameters )[nParameterElement];
				NStr::ToLower( &( rParameter.szPath ) );
				NStr::ToLower( &( rParameter.szExtention ) );

				rParameter.nPathLength = rParameter.szPath.size();
				rParameter.nExtentionLength = rParameter.szExtention.size();
			}
		}
		std::vector<std::string> fileNameList;
		NVFS::GetMainVFS()->GetAllFileNames( &fileNameList, std::string() );
		std::vector<std::string> stringList;
		int nCount = 0;
		for ( std::vector<std::string>::const_iterator itFileName = fileNameList.begin(); itFileName != fileNameList.end(); ++itFileName )
		{
			const std::string szFileName = ( *itFileName );
			if ( !szFileName.empty() )
			{
				++nCount;
				if ( pParameters )
				{
					const int nStatsLength = szFileName.size();
					for ( int nParameterElement = 0; nParameterElement < pParameters->size(); ++nParameterElement )
					{
						SEnumFilesInDataStorageParameter &rParameter = ( *pParameters )[nParameterElement];

						if ( ( rParameter.nPathLength < nStatsLength ) &&
								 ( strncmp( szFileName.c_str(), rParameter.szPath.c_str(), rParameter.nPathLength ) == 0 ) )
						{
							if ( rParameter.nExtentionLength > 0 )
							{
								if ( strncmp( szFileName.c_str() + nStatsLength - rParameter.nExtentionLength, rParameter.szExtention.c_str(), rParameter.nExtentionLength ) == 0 )
								{
									rParameter.fileNameList.push_back( szFileName );
								}
							}
							else
							{
								const int nDotPositon = szFileName.rfind( '.' );
								const int nSlashPositon = szFileName.rfind( '\\' );
								if ( nDotPositon == std::string::npos )
								{
									rParameter.fileNameList.push_back( szFileName );
								}
								else if ( (  nSlashPositon != std::string::npos ) && ( nDotPositon < nSlashPositon ) )
								{
									rParameter.fileNameList.push_back( szFileName );
								}
							}
						}
					}
				}
				if ( pEnumFolderStructureParameter )
				{
					stringList.clear();
					NStr::SplitString( szFileName, &stringList, '\\' ) ;
					if ( stringList.size() > ( pEnumFolderStructureParameter->nIgnoreFolderCount + 1 ) )
					{
						for ( int nStringIndex = 0; nStringIndex < ( stringList.size() - pEnumFolderStructureParameter->nIgnoreFolderCount - 1 ); ++nStringIndex )
						{
							if ( !NStr::IsDecNumber( stringList[nStringIndex] ) )
							{
								for ( int nRelativeStringIndex = 0; nRelativeStringIndex < ( stringList.size() - pEnumFolderStructureParameter->nIgnoreFolderCount - 1 ); ++nRelativeStringIndex )
								{
									if ( !NStr::IsDecNumber( stringList[nRelativeStringIndex] ) )
									{
										pEnumFolderStructureParameter->SetRelativeFolder( stringList[nStringIndex], stringList[nRelativeStringIndex] );
									}
								}
							}
						}
					}
				}
			}
		}
		fmt::format( "Count: {}", nCount );
	}
}


bool ExecuteProcess( const std::string &rszCommand, const std::string &rszCmdLine, const std::string &rszDirectory, bool bWait )
{
	// The 2048 byte stack buffer and the strcpy into it are gone with the
	// CreateProcess call that needed a writable command line: a longer command
	// line used to overrun it.
	if ( bWait )
	{
		return RunAndWait( rszCommand, rszCmdLine, rszDirectory );
	}
	return LaunchDetachedIn( rszCommand, rszCmdLine, rszDirectory );
}


// std::string, not CString, since MFC is on its way out of the editor; the
// narrow side is UTF-8 either way. The code page these used to take was never
// read: see the note in Tools_Resources.h for why it is gone rather than fixed
// at 65001.
void Unicode2MBSC( std::string *pszText, const std::wstring &rwszText )
{
	if ( pszText )
	{
		*pszText = WideToUTF8( rwszText );
	}
}


void MBSC2Unicode( std::wstring *pwszText, const std::string &rszText )
{
	if ( pwszText )
	{
		*pwszText = UTF8ToWide( rszText );
	}
}


namespace
{
	std::wstring DecodeUnicodeResource( const std::vector<uint8_t> &buffer, bool *needsRepair = nullptr )
	{
		// Early Linux editors wrote a UTF-16 BOM followed by native UTF-32LE.
		// Recover that specific pattern for editing, but only rewrite it after
		// the user accepts the editor and confirms saving the file.
		const size_t size = buffer.size() - 2;
		bool legacy = size >= 4 && size % 4 == 0;
		bool hasPadding = false;
		std::string recovered;
		auto appendUnit = [&recovered]( uint32_t unit ) {
			recovered.push_back( char(unit & 0xff) );
			recovered.push_back( char((unit >> 8) & 0xff) );
		};
		for ( size_t i = 2; legacy && i < buffer.size(); i += 4 )
		{
			uint32_t code = uint32_t(buffer[i]) | (uint32_t(buffer[i + 1]) << 8) |
				(uint32_t(buffer[i + 2]) << 16) | (uint32_t(buffer[i + 3]) << 24);
			if ( code == 0 || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) )
			{
				legacy = false;
				break;
			}
			hasPadding |= code <= 0xffff;
			if ( code > 0xffff )
			{
				code -= 0x10000;
				appendUnit( 0xd800 + (code >> 10) );
				appendUnit( 0xdc00 + (code & 0x3ff) );
			}
			else appendUnit( code );
		}
		legacy &= hasPadding;
		if ( needsRepair ) *needsRepair = legacy;
		return legacy ? UTF16LEToWide( recovered.data(), recovered.size() ) :
			UTF16LEToWide( buffer.data() + 2, size );
	}
}

void File2String( std::string *pstrText, bool *pbUnicode, const std::vector<uint8_t> &rBuffer, bool bRemove_0D, bool *pbNeedsUnicodeRepair )
{
	if ( pbNeedsUnicodeRepair ) *pbNeedsUnicodeRepair = false;
	if ( pstrText )
	{
		pstrText->clear();
		if ( ( pstrText != 0 ) &&
				 ( rBuffer.size() > 1 ) &&
				 ( rBuffer[0] == 0xFF ) &&
				 ( rBuffer[1] == 0xFE ) )
		{
			// Disk characters are UTF-16LE, regardless of native wchar_t width.
			std::wstring wszText = DecodeUnicodeResource( rBuffer, pbNeedsUnicodeRepair );
			if ( bRemove_0D )
			{
				wszText.erase( remove( wszText.begin(), wszText.end(), wchar_t( 0x0D ) ), wszText.end() );
			}
			int nLastIndex = 0;
			for ( nLastIndex = ( wszText.size() - 1 ); nLastIndex >= 0; --nLastIndex )
			{
				if ( ( wszText[nLastIndex] != wchar_t( 0x0D ) ) && ( wszText[nLastIndex] != wchar_t( 0x0D ) ) )
				{
					break;
				}
			}
			if ( nLastIndex < 0 )
			{
				wszText.clear();
			}
			else if ( nLastIndex < ( wszText.size() - 1 ) )
			{
				wszText = wszText.substr( 0, nLastIndex + 1 );
			}
			Unicode2MBSC( pstrText, wszText );
			if ( pbUnicode )
			{
				( *pbUnicode ) = true;
			}
		}
		else if ( pstrText != 0 )
		{
			std::string szText( rBuffer.begin(), rBuffer.end() );
			if ( bRemove_0D )
			{
				szText.erase( remove( szText.begin(), szText.end(), 0x0D ), szText.end() );
			}
			int nLastIndex = 0;
			for ( nLastIndex = ( szText.size() - 1 ); nLastIndex >= 0; --nLastIndex )
			{
				if ( ( szText[nLastIndex] != 0x0A ) && ( szText[nLastIndex] != 0x0D ) )
				{
					break;
				}
			}
			if ( nLastIndex < 0 )
			{
				szText.clear();
			}
			else if ( nLastIndex < ( szText.size() - 1 ) )
			{
				szText = szText.substr( 0, nLastIndex + 1 );
			}
			( *pstrText ) = szText;
			if ( pbUnicode )
			{
				( *pbUnicode ) = false;
			}
		}
	}
}


void File2String( std::string *pstrText, bool *pbUnicode, const std::string &rszTextPath, bool bRemove_0D, bool *pbNeedsUnicodeRepair )
{
	if ( pbNeedsUnicodeRepair ) *pbNeedsUnicodeRepair = false;
	if ( pstrText != 0 )
	{
		pstrText->clear();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, rszTextPath );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
		{
			if ( streamHolder.pStream->GetSize() > 0 )
			{
				std::vector<uint8_t> fileBuffer;
				fileBuffer.resize( streamHolder.pStream->GetSize() );
				streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
				//
				File2String( pstrText, pbUnicode, fileBuffer, bRemove_0D, pbNeedsUnicodeRepair );
			}
		}
	}	
}


void File2String( std::wstring *pwszText, const std::vector<uint8_t> &rBuffer, bool bRemove_0D )
{
	if ( pwszText != 0 )
	{
		pwszText->clear();
		if ( ( rBuffer.size() > 1 ) &&
				 ( rBuffer[0] == 0xFF ) &&
				 ( rBuffer[1] == 0xFE ) )
		{
			*pwszText = DecodeUnicodeResource( rBuffer );
			if ( bRemove_0D )
			{
				pwszText->erase( remove( pwszText->begin(), pwszText->end(), wchar_t( 0x0D ) ), pwszText->end() );
			}
			int nLastIndex = 0;
			for ( nLastIndex = ( pwszText->size() - 1 ); nLastIndex >= 0; --nLastIndex )
			{
				if ( ( ( *pwszText )[nLastIndex] != wchar_t( 0x0D ) ) && ( ( *pwszText )[nLastIndex] != wchar_t( 0x0D ) ) )
				{
					break;
				}
			}
			if ( nLastIndex < 0 )
			{
				pwszText->clear();
			}
			else if ( nLastIndex < ( pwszText->size() - 1 ) )
			{
				( *pwszText ) = pwszText->substr( 0, nLastIndex + 1 );
			}
		}
	}
}


void File2String( std::wstring *pwszText, const std::string &rszTextPath, bool bRemove_0D )
{
	if ( pwszText != 0 )
	{
		pwszText->clear();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, rszTextPath );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
		{
			if ( streamHolder.pStream->GetSize() > 0 )
			{
				std::vector<uint8_t> fileBuffer;
				fileBuffer.resize( streamHolder.pStream->GetSize() );
				streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
				//
				File2String( pwszText, fileBuffer, bRemove_0D );
			}
		}
	}	
}


void String2File( std::vector<uint8_t> *pBuffer, const std::string &rstrText, bool bUnicode, bool bAdd_0D )
{
	if ( pBuffer != 0 )
	{
		pBuffer->clear();
		if ( bUnicode )
		{
			// Share the wide writer so empty files retain their BOM too.
			String2File( pBuffer, UTF8ToWide( rstrText ), bAdd_0D );
		}
		else
		{
			std::string szText = rstrText;
			if ( !szText.empty() )
			{
				if ( bAdd_0D )
				{
					for ( int nIndex = 0; nIndex < szText.size(); ++nIndex )
					{
						if ( szText[nIndex] == 0x0A )
						{
							if ( ( nIndex == 0 ) || ( szText[nIndex - 1] != 0x0D ) )
							{
								szText.insert( szText.begin() + nIndex, 0x0D );
							}
						}
					}
				}
				int nLastIndex = 0;
				for ( nLastIndex = ( szText.size() - 1 ); nLastIndex >= 0; --nLastIndex )
				{
					if ( ( szText[nLastIndex] != 0x0A ) && ( szText[nLastIndex] != 0x0D ) )
					{
						break;
					}
				}
				if ( nLastIndex < 0 )
				{
					szText.clear();
				}
				else if ( nLastIndex < ( szText.size() - 1 ) )
				{
					szText = szText.substr( 0, nLastIndex + 1 );
				}
				if ( !szText.empty() )
				{
					pBuffer->resize( szText.size() );
					memcpy( &( ( *pBuffer )[0] ), &( szText[0] ), szText.size() );
				}
			}
		}
	}
}


void String2File( const std::string &rstrText, bool bUnicode, const std::string &rszTextPath, bool bAdd_0D )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszTextPath );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		std::vector<uint8_t> fileBuffer;
		String2File( &fileBuffer, rstrText, bUnicode, bAdd_0D );
		if ( fileBuffer.size() > 0 )
		{
			streamHolder.pStream->Write( &( fileBuffer[0] ), fileBuffer.size() );
		}
	}
}


void String2File( std::vector<uint8_t> *pBuffer, const std::wstring &rwszText, bool bAdd_0D )
{
	if ( pBuffer != 0 )
	{
		pBuffer->clear();
		std::wstring wszText = rwszText;
		if ( bAdd_0D )
		{
			for ( int nIndex = 0; nIndex < wszText.size(); ++nIndex )
			{
				if ( wszText[nIndex] == wchar_t( 0x0A ) )
				{
					if ( ( nIndex == 0 ) || ( wszText[nIndex - 1] != wchar_t( 0x0D ) ) )
					{
						wszText.insert( wszText.begin() + nIndex, wchar_t( 0x0D ) );
					}
				}
			}
		}
		int nLastIndex = 0;
		for ( nLastIndex = ( wszText.size() - 1 ); nLastIndex >= 0; --nLastIndex )
		{
			if ( ( wszText[nLastIndex] != wchar_t( 0x0A ) ) && ( wszText[nLastIndex] != wchar_t( 0x0D ) ) )
			{
				break;
			}
		}
		if ( nLastIndex < 0 )
		{
			wszText.clear();
		}
		else if ( nLastIndex < ( wszText.size() - 1 ) )
		{
			wszText = wszText.substr( 0, nLastIndex + 1 );
		}
		// Serialize Windows-compatible UTF-16LE, never native wchar_t bytes.
		const std::string encoded = WideToUTF16LE( wszText );
		pBuffer->assign( { 0xFF, 0xFE } );
		pBuffer->insert( pBuffer->end(), encoded.begin(), encoded.end() );
	}
}


void String2File( const std::wstring &rwszText, const std::string &rszTextPath, bool bAdd_0D )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszTextPath );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		std::vector<uint8_t> fileBuffer;
		String2File( &fileBuffer, rwszText, bAdd_0D );
		if ( fileBuffer.size() > 0 )
		{
			streamHolder.pStream->Write( &( fileBuffer[0] ), fileBuffer.size() );
		}
	}
}


// basement storage  


