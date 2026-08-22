#include "stdafx.h"

#include "Tools_Resources.h"
#include "Misc/StrProc.h"
#include "System/VFS.h"
#include "System/WinVFS.h"
#include "System/VFSOperations.h"

#include "port/unicode.h"

#include <cstdint>

void OpenStreamHolder( SFileStreamHolder *pStreamHolder, const string &rszTextPath )
{
	if ( pStreamHolder )
	{
		if ( ( rszTextPath.find( ':' ) == string::npos ) && NVFS::GetMainVFS() )
		{
			pStreamHolder->pStream = new CFileStream( NVFS::GetMainVFS(), rszTextPath );
		}
		else
		{
			pStreamHolder->pStream = new CFileStream( rszTextPath, CFileStream::WIN_READ_ONLY );
		}
	}
}


void CreateStreamHolder( SFileStreamHolder *pStreamHolder, const string &rszTextPath )
{
	if ( pStreamHolder )
	{
		if ( ( rszTextPath.find( ':' ) == string::npos ) && NVFS::GetMainFileCreator() )
		{
			pStreamHolder->pStream = new CFileStream( NVFS::GetMainFileCreator(), rszTextPath );
		}
		else
		{
			pStreamHolder->pStream = new CFileStream( rszTextPath, CFileStream::WIN_CREATE );
		}
	}
}


bool NormalizePath( string *pszPath, bool bFile, bool bExists, bool bReturnAbsolutePath, const string &rszPathPrefix, bool *pbAbsolutePath )
{
	bool bResult = true;
	NI_ASSERT( pszPath != 0, "NormalizePath() pszPath == 0" );
	if ( pszPath )
	{
		NStr::ReplaceAllChars( pszPath, '/', '\\' );
		// Проверяем наличие слеша в конце пути
		string szPathPrefix = rszPathPrefix;
		if ( ( szPathPrefix.size() > 0 ) && ( szPathPrefix[szPathPrefix.size() - 1] != '\\' ) )
		{
			szPathPrefix += "\\";
		}
		// Проверяем наличие отсутствия слеша в начале имени
		string szPath = ( *pszPath );
		if ( ( szPath.size() > 0 ) && ( szPath[0] == '\\' ) )
		{
			szPath = szPath.substr( 1 );
		}
		// Если это каталог - необходимо проверить наличие последнего слеша
		if ( !bFile )
		{
			if ( ( szPath.size() > 0 ) && ( szPath[szPath.size() - 1] != '\\' ) )
			{
				szPath += "\\";
			}
		}
		
		// Расширяем имя файла до полного или отрезаем ненужное
		string szLCPathPrefix = szPathPrefix;
		string szLCPath = szPath;
		NStr::ToLowerASCII( &szLCPathPrefix );
		NStr::ToLowerASCII( &szLCPath );
		// Имя по которому будет проверятся наличие фала на диске
		string szCheckPath;
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
					uint32_t dwAttributes = GetFileAttributes( szCheckPath.c_str() );
					if ( ( dwAttributes == INVALID_FILE_ATTRIBUTES ) ||
							 ( ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY ) )
					{
						bResult = false;
					}
				}
				else
				{
					szCheckPath = szCheckPath.substr( 0, szCheckPath.size() - 1 );
					uint32_t dwAttributes = GetFileAttributes( szCheckPath.c_str() );
					if ( ( dwAttributes == INVALID_FILE_ATTRIBUTES ) ||
							 ( ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) != FILE_ATTRIBUTE_DIRECTORY ) )
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
			( *pbAbsolutePath ) = ( ( szPath.size() > 2 ) && ( szPath[1] == ':' ) && ( szPath[2] == '\\' ) || ( szPath[2] == '\\' ) );
		}
	}
	return bResult;
}


bool IsValidFileName( const string &rszFileName, bool bAbsolutePath )
{
	if ( rszFileName.find_first_of( "*?<>|" ) != string::npos )
	{
		return false;
	}
	int nPos = rszFileName.find( ':' );
	if ( ( nPos != string::npos ) && ( nPos != string::npos != 1 ) )
	{
		return false;			
	}
	else if ( nPos == 1 )
	{
		if ( bAbsolutePath )
		{
			nPos = rszFileName.find( nPos, ':' );
			if ( nPos != string::npos )
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

bool CheckLatestBINResource( const string &rszResourceFileName, const string &rszXMLExtention, const string &rszBINExtention )
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


bool SEnumFolderStructureParameter::IsFolderRelative( const string &rszFolder, const string &rszRelativeFolder )
{
	return IsFolderRelative( enumFolderMap, rszFolder, rszRelativeFolder );
}


void SEnumFolderStructureParameter::SetRelativeFolder( const string &rszFolder, const string &rszRelativeFolder )
{
	SetRelativeFolder( &enumFolderMap, rszFolder, rszRelativeFolder );
}


bool SEnumFolderStructureParameter::IsFolderRelative( const CEnumFolderMap &rEnumFolderMap, const string &rszFolder, const string &rszRelativeFolder )
{
	CEnumFolderMap::const_iterator folderIterator = rEnumFolderMap.find( rszFolder );
	if ( folderIterator != rEnumFolderMap.end() )
	{
		return ( folderIterator->second.find( rszRelativeFolder ) != folderIterator->second.end() );
	}
	return false;
}


void SEnumFolderStructureParameter::SetRelativeFolder( CEnumFolderMap *pEnumFolderMap, const string &rszFolder, const string &rszRelativeFolder )
{
	NI_ASSERT( pEnumFolderMap != 0, StrFmt( "Wrong parameter: %x\n", pEnumFolderMap ) );
	if ( pEnumFolderMap )
	{
		( *pEnumFolderMap )[rszFolder][rszRelativeFolder] = 0;
	}
}


void EnumFilesInDataStorage( vector<SEnumFilesInDataStorageParameter> *pParameters, SEnumFolderStructureParameter *pEnumFolderStructureParameter )
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
		vector<string> fileNameList;
		NVFS::GetMainVFS()->GetAllFileNames( &fileNameList, string() );
		vector<string> stringList;
		int nCount = 0;
		for ( vector<string>::const_iterator itFileName = fileNameList.begin(); itFileName != fileNameList.end(); ++itFileName )
		{
			const string szFileName = ( *itFileName );
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
								if ( nDotPositon == string::npos )
								{
									rParameter.fileNameList.push_back( szFileName );
								}
								else if ( (  nSlashPositon != string::npos ) && ( nDotPositon < nSlashPositon ) )
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
		StrFmt( "Count: %d", nCount );
	}
}


bool ExecuteProcess( const string &rszCommand, const string &rszCmdLine, const string &rszDirectory, bool bWait )
{
	char pszCommandLine[2048];
	strcpy( pszCommandLine, rszCmdLine.c_str() );
	//
	STARTUPINFO startinfo;
	PROCESS_INFORMATION procinfo;
	Zero( startinfo );
	Zero( procinfo );
	startinfo.cb = sizeof( startinfo );
	BOOL bRetVal = CreateProcess( rszCommand.c_str(), pszCommandLine, 0, 0, FALSE, 0, 0, rszDirectory.c_str(), &startinfo, &procinfo );
	if ( bRetVal == FALSE ) 
	{
		return false;
	}
	if ( bWait )
	{
		WaitForSingleObject( procinfo.hProcess, INFINITE );
	}
	return true;
}


void Unicode2MBSC( CString *pstrText, const wstring &rwszText, int nCodePage )
{
	if ( pstrText )
	{
		pstrText->Empty();
		if ( !rwszText.empty() )
		{
			*pstrText = WideToUTF8( rwszText ).c_str();
		}
	}
}


void MBSC2Unicode( wstring *pwszText, const CString &rstrText, int nCodePage )
{
	if ( pwszText )
	{
		pwszText->clear();
		if ( !rstrText.IsEmpty() )
		{
			*pwszText = UTF8ToWide( std::string( (const char *)rstrText ) );
		}
	}
}


void File2String( CString *pstrText, bool *pbUnicode, const vector<uint8_t> &rBuffer, int nCodePage, bool bRemove_0D )
{
	if ( pstrText )
	{
		pstrText->Empty();
		if ( ( pstrText != 0 ) &&
				 ( rBuffer.size() > 1 ) &&
				 ( rBuffer[0] == 0xFF ) &&
				 ( rBuffer[1] == 0xFE ) )
		{
			if ( rBuffer.size() < 3 )
			{
				return;
			}
			wstring wszText;
			wszText.resize( ( rBuffer.size() - 2 ) / sizeof( wchar_t ) );
			memcpy( &( wszText[0] ), &( rBuffer[0] ) + 2, wszText.size() * sizeof( wchar_t ) );
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
			Unicode2MBSC( pstrText, wszText, nCodePage );
			if ( pbUnicode )
			{
				( *pbUnicode ) = true;
			}
		}
		else if ( pstrText != 0 )
		{
			string szText;
			szText.resize( rBuffer.size() );
			memcpy( &( szText[0] ), &( rBuffer[0] ), szText.size() );
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
			if ( !szText.empty() )
			{
				( *pstrText ) = szText.c_str();
			}
			if ( pbUnicode )
			{
				( *pbUnicode ) = false;
			}
		}
	}
}


void File2String( CString *pstrText, bool *pbUnicode, const string &rszTextPath, int nCodePage, bool bRemove_0D )
{
	if ( pstrText != 0 )
	{
		pstrText->Empty();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, rszTextPath );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
		{
			if ( streamHolder.pStream->GetSize() > 0 )
			{
				vector<uint8_t> fileBuffer;
				fileBuffer.resize( streamHolder.pStream->GetSize() );
				streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
				//
				File2String( pstrText, pbUnicode, fileBuffer, nCodePage, bRemove_0D );
			}
		}
	}	
}


void File2String( string *pszText, bool *pbUnicode, const string &rszTextPath, int nCodePage, bool bRemove_0D )
{
	if ( pszText != 0 )
	{
		CString strText;
		File2String( &strText, pbUnicode, rszTextPath, nCodePage, bRemove_0D );
		( *pszText ) = strText;
	}
}


void File2String( wstring *pwszText, const vector<uint8_t> &rBuffer, bool bRemove_0D )
{
	if ( pwszText != 0 )
	{
		pwszText->clear();
		if ( ( rBuffer.size() > 1 ) &&
				 ( rBuffer[0] == 0xFF ) &&
				 ( rBuffer[1] == 0xFE ) )
		{
			if ( rBuffer.size() < 3 )
			{
				return;
			}
			pwszText->resize( ( rBuffer.size() - 2 ) / sizeof( wchar_t ) );
			memcpy( &( ( *pwszText )[0] ), &( rBuffer[0] ) + 2, pwszText->size() * sizeof( wchar_t ) );
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


void File2String( wstring *pwszText, const string &rszTextPath, bool bRemove_0D )
{
	if ( pwszText != 0 )
	{
		pwszText->empty();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, rszTextPath );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
		{
			if ( streamHolder.pStream->GetSize() > 0 )
			{
				vector<uint8_t> fileBuffer;
				fileBuffer.resize( streamHolder.pStream->GetSize() );
				streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
				//
				File2String( pwszText, fileBuffer, bRemove_0D );
			}
		}
	}	
}


void String2File( vector<uint8_t> *pBuffer, const CString &rstrText, bool bUnicode, int nCodePage, bool bAdd_0D )
{
	if ( pBuffer != 0 )
	{
		pBuffer->clear();
		if ( bUnicode )
		{
			wstring wszText;
			MBSC2Unicode( &wszText, rstrText, nCodePage );
			if ( !wszText.empty() )
			{
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
				pBuffer->resize( 2 + wszText.size() * sizeof( wchar_t ) );
				( *pBuffer )[0] = 0xFF;
				( *pBuffer )[1] = 0xFE;
				if ( !wszText.empty() )
				{
					memcpy( &( ( *pBuffer )[2] ), &( wszText[0] ), wszText.size() * sizeof( wchar_t ) );
				}
			}
		}
		else
		{
			string szText = rstrText;
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


void String2File( const CString &rstrText, bool bUnicode, const string &rszTextPath, int nCodePage, bool bAdd_0D )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszTextPath );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		vector<uint8_t> fileBuffer;
		String2File( &fileBuffer, rstrText, bUnicode, nCodePage, bAdd_0D );
		if ( fileBuffer.size() > 0 )
		{
			streamHolder.pStream->Write( &( fileBuffer[0] ), fileBuffer.size() );
		}
	}
}


void String2File( const string &rszText, bool bUnicode, const string &rszTextPath, int nCodePage, bool bAdd_0D )
{
	CString strText( rszText.c_str() );
	String2File( strText, bUnicode, rszTextPath, nCodePage, bAdd_0D );
}


void String2File( vector<uint8_t> *pBuffer, const wstring &rwszText, bool bAdd_0D )
{
	if ( pBuffer != 0 )
	{
		pBuffer->clear();
		wstring wszText = rwszText;
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
		pBuffer->resize( 2 + wszText.size() * sizeof( wchar_t ) );
		( *pBuffer )[0] = 0xFF;
		( *pBuffer )[1] = 0xFE;
		if ( !wszText.empty() )
		{
			memcpy( &( ( *pBuffer )[2] ), &( wszText[0] ), wszText.size() * sizeof( wchar_t ) );
		}
	}
}


void String2File( const wstring &rwszText, const string &rszTextPath, bool bAdd_0D )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, rszTextPath );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk() )
	{
		vector<uint8_t> fileBuffer;
		String2File( &fileBuffer, rwszText, bAdd_0D );
		if ( fileBuffer.size() > 0 )
		{
			streamHolder.pStream->Write( &( fileBuffer[0] ), fileBuffer.size() );
		}
	}
}


// basement storage  


