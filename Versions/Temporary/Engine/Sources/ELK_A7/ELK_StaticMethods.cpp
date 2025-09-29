#include "StdAfx.h"

#include "ELK_Types.h"
#include "../MapEditorLib/Tools_Resources.h"
#include "../MapEditorLib/Tools_Registry.h"
#include "../MapEditorLib/StringManager.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../Misc/StrProc.h"
#include "ProgressDialog.h"
#include "ELK_TreeWindow.h"
#include "SpellChecker.h"
#include "../Image/Image.h"
#include "../System/FileUtils.h"
#include "../System/WinVFS.h"
#include "../System/VFSOperations.h"

#include <afxdb.h> 
#include <odbcinst.h> 
#include "BlitzkriegELKDatabase.h"
#include "MLParser.h"

#include "../libdb/ResourceManager.h"
#include "../libdb/EditorDb.h"
#include "UserDataContainer.h"
#include "ExporterContainer.h"
#include "../ED_Common/FontExporter.h"
#include "../ED_Common/TextureExporter.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "..\System\FilePath.h"

#ifdef _DEBUG

#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//REGISTER_EXPORTER_IN_EXE( Font, CFontExporter )
//REGISTER_EXPORTER_IN_EXE( Texture, CTextureExporter )

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NDb::SFont::ECharset CELK::GetCharset( int nCodePage )
{
	DWORD dwTemp = nCodePage;
	CHARSETINFO sCharSetInfo;
	memset( &sCharSetInfo, 0, sizeof( CHARSETINFO ) );
	if ( TranslateCharsetInfo( (DWORD*)dwTemp, &sCharSetInfo, TCI_SRCCODEPAGE ) )
	{
		if  ( sCharSetInfo.ciCharset == ANSI_CHARSET )
		{
			return NDb::SFont::ANSI;
		}
		else if  ( sCharSetInfo.ciCharset == BALTIC_CHARSET )
		{
			return NDb::SFont::BALTIC;
		}
		else if  ( sCharSetInfo.ciCharset == CHINESEBIG5_CHARSET )
		{
			return NDb::SFont::CHINESEBIG5;
		}
		else if  ( sCharSetInfo.ciCharset == EASTEUROPE_CHARSET )
		{
			return NDb::SFont::EASTEUROPE;
		}
		else if  ( sCharSetInfo.ciCharset == GB2312_CHARSET )
		{
			return NDb::SFont::GB2312;
		}
		else if  ( sCharSetInfo.ciCharset == GREEK_CHARSET )
		{
			return NDb::SFont::GREEK;
		}
		else if  ( sCharSetInfo.ciCharset == HANGUL_CHARSET )
		{
			return NDb::SFont::HANGUL;
		}
		else if  ( sCharSetInfo.ciCharset == RUSSIAN_CHARSET )
		{
			return NDb::SFont::RUSSIAN;
		}
		else if  ( sCharSetInfo.ciCharset == SHIFTJIS_CHARSET )
		{
			return NDb::SFont::SHIFTJIS;
		}
		else if  ( sCharSetInfo.ciCharset == SYMBOL_CHARSET )
		{
			return NDb::SFont::SYMBOL;
		}
		else if  ( sCharSetInfo.ciCharset == TURKISH_CHARSET )
		{
			return NDb::SFont::TURKISH;
		}
		else if  ( sCharSetInfo.ciCharset == HEBREW_CHARSET )
		{
			return NDb::SFont::HEBREW;
		}
		else if  ( sCharSetInfo.ciCharset == ARABIC_CHARSET )
		{
			return NDb::SFont::ARABIC;
		}
		else if  ( sCharSetInfo.ciCharset == THAI_CHARSET )
		{
			return NDb::SFont::THAI;
		}
	}
	return NDb::SFont::DEF_CHARSET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SELKElement::GetDataBaseFolder( const string &rszELKPath, string *pszDataBaseFolder )
{
	NI_ASSERT( pszDataBaseFolder != 0, StrFmt( _T( "CELK::GetDataBaseFolder() wrong parameter: pszDataBaseFolder %x" ), pszDataBaseFolder ) );
	if ( pszDataBaseFolder )
	{
		//( *pszDataBaseFolder ) = rszELKPath.substr( 0, rszELKPath.rfind( '.' ) );
		//( *pszDataBaseFolder ) +=	DATA_BASE_FOLDER;
		( *pszDataBaseFolder ) = rszELKPath + string( DATA_BASE_FOLDER );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SELKElement::GetDataBaseReserveFolder( const string &rszELKPath, string *pszDataBaseReserveFolder )
{
	NI_ASSERT( pszDataBaseReserveFolder != 0, StrFmt( _T( "CELK::GetDataBaseReserveFolder() wrong parameter: pszDataBaseReserveFolder %x" ), pszDataBaseReserveFolder ) );
	if ( pszDataBaseReserveFolder )
	{
		//( *pszDataBaseReserveFolder ) = rszELKPath.substr( 0, rszELKPath.rfind( '.' ) );
		//( *pszDataBaseReserveFolder ) +=	DATA_BASE_RESERVE_FOLDER;
		( *pszDataBaseReserveFolder ) =	rszELKPath + string( DATA_BASE_RESERVE_FOLDER );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// = _T( "MICROSOFT EXCEL DRIVER (*.XLS)" );
bool GetExcelODBCDriverName( CString *pstrExcelODBCDriverName )
{
	if ( pstrExcelODBCDriverName )
	{
		pstrExcelODBCDriverName->Empty();
		TCHAR szBuf[2001];
		const WORD cbBufMax = 2000;
		WORD cbBufOut;
		LPTSTR pszBuf = szBuf;

		if ( SQLGetInstalledDrivers( szBuf, cbBufMax, &cbBufOut ) )
		{
			do
			{
				if( strstr( pszBuf, _T( "Excel" ) ) != 0 )
				{
					( *pstrExcelODBCDriverName ) = pszBuf;
					return true;
				}
				pszBuf = strchr( pszBuf, '\0' ) + 1;
			}
			while( pszBuf[1] != '\0' );
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::GetOriginalText( const string &rszTextPath, CString *pstrText, int nCodePage, bool bRemove_0D )
{
	NI_ASSERT( pstrText != 0, StrFmt( _T( "CELK::GetOriginalText() wrong parameter: pstrText %x" ), pstrText ) );
	if ( pstrText )
	{
		pstrText->Empty();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, StrFmt( _T( "%s%s" ), rszTextPath.c_str(), ELK_EXTENTION ) );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() && ( streamHolder.pStream->GetSize() > 0 ) )
		{
			vector<BYTE> fileBuffer;
			fileBuffer.resize( streamHolder.pStream->GetSize() );
			streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
			File2String( pstrText, 0, fileBuffer, nCodePage, bRemove_0D );
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::GetTranslatedText( const string &rszTextPath, CString *pstrText,  int nCodePage, bool bRemove_0D )
{
	NI_ASSERT( pstrText != 0, StrFmt( _T( "CELK::GetTranslatedText() wrong parameter: pstrText %x" ), pstrText ) );
	if ( pstrText )
	{
		pstrText->Empty();
		SFileStreamHolder streamHolder;
		OpenStreamHolder( &streamHolder, StrFmt( _T( "%s%s" ), rszTextPath.c_str(), TXT_EXTENTION ) );
		if ( streamHolder.pStream && streamHolder.pStream->IsOk() && ( streamHolder.pStream->GetSize() > 0 ) )
		{
			vector<BYTE> fileBuffer;
			fileBuffer.resize( streamHolder.pStream->GetSize() );
			streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
			File2String( pstrText, 0, fileBuffer, nCodePage, bRemove_0D );
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::GetDescription( const string &rszTextPath, CString *pstrText, int nCodePage, bool bRemove_0D )
{
	NI_ASSERT( pstrText != 0, StrFmt( _T( "CELK::GetDescription() wrong parameter: pstrText %x" ), pstrText ) );
	if ( pstrText )
	{
		pstrText->Empty();
		string szFileName = StrFmt( _T( "%s%s" ), rszTextPath.c_str(), DSC_EXTENTION );
		string szFileFolder = szFileName.substr( 0, szFileName.rfind( '\\' ) );
		while ( !NFile::DoesFileExist( szFileName.c_str() ) )
		{
			int nPosition = szFileFolder.rfind( '\\' );
			if ( nPosition == string::npos )
			{
				szFileName.clear();
				break;
			}
			szFileFolder = szFileFolder.substr( 0, nPosition );
			szFileName = szFileFolder + string( _T( "\\" ) ) + string( FOLDER_DESC_FILE_NAME ) + DSC_EXTENTION;
		}
		if ( !szFileName.empty() )
		{
			SFileStreamHolder streamHolder;
			OpenStreamHolder( &streamHolder, szFileName );
			if ( streamHolder.pStream && streamHolder.pStream->IsOk() && ( streamHolder.pStream->GetSize() > 0 ) )
			{
				vector<BYTE> fileBuffer;
				fileBuffer.resize( streamHolder.pStream->GetSize() );
				streamHolder.pStream->Read( &( fileBuffer[0] ), fileBuffer.size() );
				File2String( pstrText, 0, fileBuffer, nCodePage, bRemove_0D );
			}
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CELK::GetState( const string &rszTextPath, bool *pbTranslated )
{
	SELKTextProperty textProperty;
	LoadTypedSuperXMLResource( rszTextPath, XML_EXTENTION, textProperty );
	//
	if ( pbTranslated )
	{
		( *pbTranslated ) = textProperty.bTranslated;
	}
	return textProperty.nState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::SetTranslatedText( const string &rszTextPath, const CString &rstrText, int nCodePage, bool bAdd_0D )
{
	SFileStreamHolder streamHolder;
	CreateStreamHolder( &streamHolder, StrFmt( _T( "%s%s" ), rszTextPath.c_str(), TXT_EXTENTION ) );
	if ( streamHolder.pStream && streamHolder.pStream->IsOk()  )
	{
		vector<BYTE> fileBuffer;
		String2File( &fileBuffer, rstrText, true, nCodePage, bAdd_0D );
		streamHolder.pStream->Write( &( fileBuffer[0] ), fileBuffer.size() );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CELK::SetState( const string &rszTextPath, int nState, bool *pbTranslated )
{
	SELKTextProperty textProperty;
	LoadTypedSuperXMLResource( rszTextPath, XML_EXTENTION, textProperty );
	//
	int nPreviousState = textProperty.nState;
	textProperty.nState = nState;
	if ( nState != SELKTextProperty::STATE_NOT_TRANSLATED )
	{
		textProperty.bTranslated = true;
	}
	
	if ( nPreviousState != nState )
	{
		SaveTypedSuperXMLResource( rszTextPath, XML_EXTENTION, textProperty );
	}
	if ( pbTranslated )
	{
		( *pbTranslated ) = textProperty.bTranslated;
	}
	
	return nPreviousState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::CreatePAK( const string &rszGamePath, const string &rszFilePath, const string &rszZIPToolPath, CProgressDialog* pwndProgressDialog )
{
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( StrFmt( _T ( "Create PAK file: %s" ), rszFilePath.c_str() ) );
	}

	const string szTempPath = StrFmt( _T( "%s%s" ), rszFilePath.c_str(), TEMP_FOLDER );
	{
		NFile::DeleteDirectory( szTempPath.c_str() );
	}

	{
		CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( StrFmt( _T( "%s*.pak" ), rszGamePath.c_str() ) );
		NVFS::SetMainVFS( pVFS );
		
		vector<SEnumFilesInDataStorageParameter> enumFilesInDataStorageParameterList;
		vector<SEnumFilesInDataStorageParameter>::iterator posEnumFilesInDataStorageParameter;
		if ( enumFilesInDataStorageParameterList.empty() )
		{
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = TXT_EXTENTION;
			//
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = DSC_EXTENTION;
			//
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = PAK_DESCRIPTION_EXTENTION;
			//
			EnumFilesInDataStorage( &enumFilesInDataStorageParameterList );
		}
		
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressRange( 0,
																						enumFilesInDataStorageParameterList[0].fileNameList.size() + 
																						enumFilesInDataStorageParameterList[1].fileNameList.size() +
																						enumFilesInDataStorageParameterList[2].fileNameList.size() );
			pwndProgressDialog->SetProgressPosition( 0 );
		}
		
		for ( int nParameterIndex = 0; nParameterIndex < enumFilesInDataStorageParameterList.size(); ++nParameterIndex )
		{
			for ( list<string>::const_iterator itName = enumFilesInDataStorageParameterList[nParameterIndex].fileNameList.begin(); itName != enumFilesInDataStorageParameterList[nParameterIndex].fileNameList.end(); ++itName )
			{
				if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
				{
					pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Copying: %s..." ), itName->c_str() ) );
				}
				CFileStream stream( NVFS::GetMainVFS(), ( *itName ) );
				if ( stream.IsOk() )
				{
					CFileStream fileStream( szTempPath + ( *itName ), CFileStream::WIN_CREATE );
					if ( fileStream.IsOk() )
					{
						stream.ReadTo( &fileStream, stream.GetSize() );
					}
				}
				if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
				{
					pwndProgressDialog->IterateProgressPosition();
				}
			}
		}
		//
		{
			vector<string> fileNameList;
			NVFS::GetMainVFS()->GetAllFileNames( &fileNameList, FONT_XDB_SUB_FOLDER );
			int nCount = 0;
			for ( vector<string>::const_iterator itFileName = fileNameList.begin(); itFileName != fileNameList.end(); ++itFileName )
			{
				CFileStream stream( NVFS::GetMainVFS(), ( *itFileName ) );
				if ( stream.IsOk() )
				{
					CFileStream fileStream( szTempPath + ( *itFileName ), CFileStream::WIN_CREATE );
					if ( fileStream.IsOk() )
					{
						stream.ReadTo( &fileStream, stream.GetSize() );
					}
				}
			}
		}
		//
		{
			vector<string> fileNameList;
			NVFS::GetMainVFS()->GetAllFileNames( &fileNameList, FONT_BIN_SUB_FOLDER );
			int nCount = 0;
			for ( vector<string>::const_iterator itFileName = fileNameList.begin(); itFileName != fileNameList.end(); ++itFileName )
			{
				CFileStream stream( NVFS::GetMainVFS(), ( *itFileName ) );
				if ( stream.IsOk() )
				{
					CFileStream fileStream( szTempPath + ( *itFileName ), CFileStream::WIN_CREATE );
					if ( fileStream.IsOk() )
					{
						stream.ReadTo( &fileStream, stream.GetSize() );
					}
				}
			}
		}
		//
		{
			CFileStream stream( NVFS::GetMainVFS(), DBTYPES_FILE_NAME );
			if ( stream.IsOk() )
			{
				CFileStream fileStream( szTempPath + DBTYPES_FILE_NAME, CFileStream::WIN_CREATE );
				if ( fileStream.IsOk() )
				{
					stream.ReadTo( &fileStream, stream.GetSize() );
				}
			}
		}
	}
	//
	DeleteFile( ( szTempPath + DBINDEX_FILE_NAME ).c_str() );
	//CELK::DBIndex( szTempPath );
	//
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Creating PAK: %s..." ), rszFilePath.c_str() ) );
	}
	ExecuteProcess( rszZIPToolPath, StrFmt( _T( " -9 -r -D \"%s\" *.*" ), rszFilePath.c_str() ), szTempPath.c_str(), true );
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Finalizing..." ) ) );
	}
	NFile::DeleteDirectory( szTempPath.c_str() );
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::ExportToPAK( const string &rszELKPath,
												const string &rszPAKPath,
												const string &rszZIPToolPath,
												class CELKTreeWindow *pwndELKTreeWindow,
												bool bOnlyFilled,
												bool bGenerateFonts,
												bool bUsedChars,
												int nCodePage,
												class CProgressDialog* pwndProgressDialog,
												const struct SSimpleFilter *pELKFilter )
{
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( StrFmt( _T( "Create PAK file: %s" ), rszPAKPath.c_str() ) );
	}

	::DeleteFile( rszPAKPath.c_str() );
	string szTempPath = StrFmt( _T( "%s%s" ), rszPAKPath.c_str(), TEMP_FOLDER );
	{
		NFile::DeleteDirectory( szTempPath.c_str() );
	}

	string szDataBaseFolder;
	SELKElement::GetDataBaseFolder( rszELKPath, &szDataBaseFolder );
	
	{
		CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( szDataBaseFolder + "*.pak" );
		NVFS::SetMainVFS( pVFS );

		vector<SEnumFilesInDataStorageParameter> enumFilesInDataStorageParameterList;
		vector<SEnumFilesInDataStorageParameter>::iterator posEnumFilesInDataStorageParameter;
		if ( enumFilesInDataStorageParameterList.empty() )
		{
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = ELK_EXTENTION;
			//
			EnumFilesInDataStorage( &enumFilesInDataStorageParameterList );
		}
		
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressRange( 0, enumFilesInDataStorageParameterList[0].fileNameList.size() );
			pwndProgressDialog->SetProgressPosition( 0 );
		}
		
		CSymbolSetMap symbolSetMap;
		bool bSingleByte = true;
		for ( list<string>::const_iterator itName = enumFilesInDataStorageParameterList[0].fileNameList.begin(); itName != enumFilesInDataStorageParameterList[0].fileNameList.end(); ++itName )
		{
			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Copying: %s..." ), itName->c_str() ) );
			}
			const string szFileName = itName->substr( 0, itName->rfind( '.' ) );
			const string szTempFileName = StrFmt( _T( "%s%s" ), szTempPath.c_str(), szFileName.c_str() );
			SELKTextProperty textProperty;

			LoadTypedSuperXMLResource( StrFmt( _T( "%s%s" ), szDataBaseFolder.c_str(), szFileName.c_str() ), XML_EXTENTION, textProperty );

			bool bTranslationIsValid = true;
			if ( pELKFilter )
			{
				bTranslationIsValid = pELKFilter->Check( szFileName, textProperty.bTranslated, textProperty.nState );
			}
			if ( bTranslationIsValid )
			{
				bTranslationIsValid = false;
				CFileStream fileStream( StrFmt( _T( "%s%s" ), szTempFileName.c_str(), TXT_EXTENTION ), CFileStream::WIN_CREATE );
				if ( fileStream.IsOk() )
				{
					if ( ( textProperty.nState != SELKTextProperty::STATE_NOT_TRANSLATED ) )// || textProperty.bTranslated )
					{
						CFileStream stream( NVFS::GetMainVFS(), StrFmt( _T( "%s%s" ), szFileName.c_str(), TXT_EXTENTION ) );
						if ( stream.IsOk() )
						{
							if ( !bOnlyFilled || stream.GetSize() > 2 )
							{
								stream.ReadTo( &fileStream, stream.GetSize() );
								bTranslationIsValid = true;
							}
						}
					}
					if ( !bTranslationIsValid )
					{
						CFileStream stream( NVFS::GetMainVFS(), StrFmt( _T( "%s%s" ), szFileName.c_str(), ELK_EXTENTION ) );
						if ( stream.IsOk() )
						{
							stream.ReadTo( &fileStream, stream.GetSize() );
						}
					}
				}
			}
			if ( bGenerateFonts )
			{
				CString strTranslatedText;
				GetTranslatedText( szTempFileName, &strTranslatedText, nCodePage, true );
				NML::CMLMBCSText mlMBCSText;
				const int nTextCount = NML::Parse( &mlMBCSText, strTranslatedText, true, nCodePage );
				if ( nTextCount > 0 )
				{
					string szFont = DEFAULT_FONT_NAME;
					for ( NML::CMLMBCSText::const_iterator itMLMBCSTextPart = mlMBCSText.begin(); itMLMBCSTextPart != mlMBCSText.end(); ++itMLMBCSTextPart )
					{
						string szTag = itMLMBCSTextPart->strTag;
						//NStr::ToLower( &szTag );
						int nFontPos = szTag.find( FONT_TAG );
						while ( nFontPos != string::npos )
						{
							CStringManager::GetStringValueFromString( szTag, FONT_TAG, nFontPos, "> ", DEFAULT_FONT_NAME, &szFont );
							nFontPos = szTag.find( FONT_TAG, nFontPos + 1 );
							NStr::TrimInside( szFont, "\"" );
						}
						CString strText = itMLMBCSTextPart->strText;
						SSymbolSet &rSymbolSet = symbolSetMap[szFont];
						if ( !strText.IsEmpty() )
						{
							LPTSTR pSymbols = (LPTSTR)(LPCTSTR)( strText );
							const int nSymbolsCount = _mbstrlen( pSymbols );
							for ( int nSymbolIndex = 0; nSymbolIndex < nSymbolsCount; ++nSymbolIndex )
							{
								LPCTSTR pSymbol = pSymbols;
								pSymbols = _tcsinc( pSymbols );
								WORD wSymbol = 0;
								if ( ( pSymbols - pSymbol ) > 1 )
								{
									const TCHAR b0 = ( *pSymbol );
									const TCHAR b1 = ( *( pSymbol + 1 ) );

									wSymbol = ( ( b0 << 8 ) & 0xFF00 ) + ( b1 & 0xFF );
									bSingleByte = false;
								}
								else
								{
									const TCHAR b0 = ( *pSymbol );
									wSymbol = b0 & 0xFF;
								}
								if ( wSymbol == 0 )
								{
									break;
								}
								rSymbolSet.symbolSet[wSymbol] = 0;
							}
						}
					}
				}
			}

			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->IterateProgressPosition();
			}
		}

		if ( bGenerateFonts )
		{
			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Creating Font Images..." ) ) );
			}
			GenerateFonts( szDataBaseFolder, szTempPath, &symbolSetMap, nCodePage, bUsedChars );
		}
	}

	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{		
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Creating PAK: %s..." ), rszPAKPath.c_str() ) );
	}
	
	ExecuteProcess( rszZIPToolPath, StrFmt( _T( " -9 -r -D \"%s\" *.*" ), rszPAKPath.c_str() ), szTempPath.c_str(), true );
	
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{		
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Finalizing..." ) ) );
	}
	{
		NFile::DeleteDirectory( szTempPath.c_str() );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCopyFileFunctional
{
	string szSourceFolder;
	int nSourceFolderFimeNameSize;
	string szDestinationFolder;
public:
	CCopyFileFunctional( const string &rszSourceFolder, const string &rszDestinationFolder ) 
		: szSourceFolder( rszSourceFolder ),
			nSourceFolderFimeNameSize( rszSourceFolder.size() ),
			szDestinationFolder( rszDestinationFolder ) {}
			void operator()( const NFile::CFileIterator &it )
	{
		if ( !it.IsDirectory() )
		{
			const string szSourceFileName = it.GetFullName();
			if ( szSourceFileName.compare( 0, nSourceFolderFimeNameSize, szSourceFolder ) == 0 )
			{
				const string szDestinationFileName = szDestinationFolder + szSourceFileName.substr( nSourceFolderFimeNameSize );
				NFile::CopyFile( szSourceFileName, szDestinationFileName );
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::GenerateFonts( const string &rszDataBaseFolder,
													const string &rszPAKPath,
													CSymbolSetMap *pSymbolMap,
													int nCodePage,
													bool bUsedChars )
{
	if ( pSymbolMap == 0 )
	{
		return;
	}
	NDb::SFont::ECharset eFontCharset = GetCharset( nCodePage );
	const string szFontCharset = SKnownEnum<NDb::SFont::ECharset>::ToString( eFontCharset );
	if ( bUsedChars )
	{
		// Создаем файлы с наборами символов
		NFile::DeleteDirectory( NFile::GetTempPath() + string( TEMP_FONT_SUB_FOLDER ) );
		for ( CSymbolSetMap::iterator itSymbolSet = pSymbolMap->begin(); itSymbolSet != pSymbolMap->end(); ++itSymbolSet )
		{
			if ( !itSymbolSet->second.symbolSet.empty() )
			{
				itSymbolSet->second.szFileName = NFile::GetTempPath() + string( TEMP_FONT_SUB_FOLDER ) + itSymbolSet->first + string( TXT_EXTENTION );
				if ( ::IsValidFileName( itSymbolSet->second.szFileName, true ) )
				{
					if ( itSymbolSet->first == DEFAULT_FONT_NAME )
					{
						for ( WORD wSymbol = 32; wSymbol < 256; ++wSymbol ) 
						{
							itSymbolSet->second.symbolSet[wSymbol] = 0;
						}
					}
					CSymbolList symbolList;
					if ( itSymbolSet->second.symbolSet.size() > 0 )
					{
						symbolList.resize( itSymbolSet->second.symbolSet.size() );
						int nSymbolIndex = 0;
						for ( CSymbolSet::const_iterator itSymbol = itSymbolSet->second.symbolSet.begin(); itSymbol != itSymbolSet->second.symbolSet.end(); ++itSymbol )
						{
							symbolList[nSymbolIndex] = itSymbol->first;
							++nSymbolIndex;
						}
					}
					sort( symbolList.begin(), symbolList.end() );
					CFileStream fileStream( itSymbolSet->second.szFileName, CFileStream::WIN_CREATE );
					if ( fileStream.IsOk() )
					{
						fileStream.Write( &( symbolList[0] ), symbolList.size() * 2 );
					}
				}
			}
		}
	}
	// Копируем Шрифты
	NFile::EnumerateFiles( rszDataBaseFolder + FONT_XDB_SUB_FOLDER, "*.*", CCopyFileFunctional( rszDataBaseFolder + FONT_XDB_SUB_FOLDER, rszPAKPath + FONT_XDB_SUB_FOLDER ), true );
	NFile::EnumerateFiles( rszDataBaseFolder + FONT_BIN_SUB_FOLDER, "*.*", CCopyFileFunctional( rszDataBaseFolder + FONT_BIN_SUB_FOLDER, rszPAKPath + FONT_BIN_SUB_FOLDER ), true );
	NFile::CopyFile( rszDataBaseFolder + DBINDEX_FILE_NAME, rszPAKPath + DBINDEX_FILE_NAME );
	NFile::CopyFile( rszDataBaseFolder + DBTYPES_FILE_NAME, rszPAKPath + DBTYPES_FILE_NAME );

	// инициализируем ресурсную систему для работы экспортеров
	NSingleton::RegisterSingleton( new CUserDataContainer(), IUserDataContainer::tidTypeID );
	if( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		pUserData->constUserData.szDataStorageFolder = rszPAKPath;
		pUserData->constUserData.szExportSourceFolder = rszPAKPath;
		pUserData->constUserData.szExportDestinationFolder = rszPAKPath;

		IResourceManager::InitSingleton();
		CObj<NVFS::IVFS> pMainVFS = NVFS::CreateWinVFS( pUserData->constUserData.szDataStorageFolder );
		NVFS::SetMainVFS( pMainVFS );
		CObj<NVFS::IFileCreator> pMainFileCreator = NVFS::CreateWinFileCreator( pUserData->constUserData.szDataStorageFolder );
		NVFS::SetMainFileCreator( pMainFileCreator );
		if ( NDb::OpenDatabase( pMainVFS, pMainFileCreator, NDb::DATABASE_MODE_EDITOR ) )
		{
			Singleton<IResourceManager>()->SetDataDir( pUserData->constUserData.szDataStorageFolder );
			NSingleton::RegisterSingleton( new CExporterContainer(), IExporterContainer::tidTypeID );
			Singleton<IExporterContainer>()->Create( "Font" );
			Singleton<IExporterContainer>()->Create( "Texture" );
			// Экспортируем шрифты
			if ( CPtr<IManipulator> pFontFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( FONT_TYPE ) )
			{
				if ( CPtr<IManipulatorIterator> pFontIterator = pFontFolderManipulator->Iterate( true, ECT_NO_CACHE ) )
				{
					Singleton<IExporterContainer>()->StartExport( FONT_TYPE, true, false, true );	
					while ( !pFontIterator->IsEnd() )
					{
						string szName;
						if ( pFontIterator->GetName( &szName ) && !szName.empty() )
						{
							if ( CPtr<IManipulator> pFontManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( FONT_TYPE, szName ) )
							{
								string szFontName;
								if ( !bUsedChars ||
										 ( CManipulatorManager::GetValue( &szFontName, pFontManipulator, "Name" ) &&
											 ( pSymbolMap->find( szFontName ) != pSymbolMap->end() ) ) )
								{
									CManipulatorManager::SetValue( szFontCharset, pFontManipulator, "Charset", false );
									Singleton<IExporterContainer>()->ExportObject( pFontManipulator, FONT_TYPE, szName, true, true );
								}
							}
						}
						pFontIterator->Next();
					}
					Singleton<IExporterContainer>()->FinishExport( FONT_TYPE, true, false, true );	
				}
			}
			NSingleton::UnRegisterSingleton( IExporterContainer::tidTypeID );
		}
		NDb::SaveChanges();
		NDb::CloseDatabase();
		IResourceManager::UninitSingleton();
	}
	NSingleton::UnRegisterSingleton( IUserDataContainer::tidTypeID );
	NLog::ClearLog();
	// удаляем временные файлы
	NFile::DeleteDirectory( NFile::GetTempPath() + string( TEMP_FONT_SUB_FOLDER ) );
	DeleteFile( ( rszPAKPath + DBINDEX_FILE_NAME ).c_str() );
	DeleteFile( ( rszPAKPath + DBTYPES_FILE_NAME ).c_str() );
	NFile::DeleteFiles( rszPAKPath.c_str(), "*.tga", true );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImportFromPAKEraseFile
{
	hash_set<string> *pUsedPaths;

public:
	CImportFromPAKEraseFile( hash_set<string> *_pUsedPaths ) : pUsedPaths( _pUsedPaths )
	{
		NI_ASSERT( pUsedPaths != 0,
								 StrFmt( _T( "CImportFromPAKEraseFile, invalid parameters: %x" ), pUsedPaths ) );
	}

	void operator()( const NFile::CFileIterator &rFileIterator )
	{
		if ( !rFileIterator.IsDirectory() && !rFileIterator.IsDots() )
		{
			string szBaseFilePath = rFileIterator.GetFullName();
			NStr::ToLower( &szBaseFilePath );
			CStringManager::CutFileExtention( &szBaseFilePath, CELK::ELK_EXTENTION );
			if ( pUsedPaths->find( szBaseFilePath ) == pUsedPaths->end() )
			{
				::DeleteFile( rFileIterator.GetFullName().c_str() );
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::ImportFromPAK( const string &rszPAKPath, const string &rszELKPath, bool bAbsolute, string *pszNewVersion, CProgressDialog* pwndProgressDialog )
{
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( StrFmt( _T( "New ELK Update found: %s" ), rszPAKPath.c_str() )  );
	}

	string szDataBaseFolder;
	SELKElement::GetDataBaseFolder( rszELKPath, &szDataBaseFolder );

	NVFS::SFileStats statsPAK;
	{
		Zero( statsPAK );
		NVFS::GetWinFileStats( &statsPAK, rszPAKPath );
	}	

	{
		CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( rszPAKPath );
		NVFS::SetMainVFS( pVFS );

		vector<SEnumFilesInDataStorageParameter> enumFilesInDataStorageParameterList;
		vector<SEnumFilesInDataStorageParameter>::iterator posEnumFilesInDataStorageParameter;
		if ( enumFilesInDataStorageParameterList.empty() )
		{
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = TXT_EXTENTION;

			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = DSC_EXTENTION;
			
			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = PAK_DESCRIPTION_EXTENTION;

			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = XDB_EXTENTION;

			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = XML_EXTENTION;

			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = DDS_EXTENTION;

			posEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.insert( enumFilesInDataStorageParameterList.end(), SEnumFilesInDataStorageParameter() );
			posEnumFilesInDataStorageParameter->szExtention = string();

			EnumFilesInDataStorage( &enumFilesInDataStorageParameterList );
		}
		
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			int nFileCount = 0;
			for ( vector<SEnumFilesInDataStorageParameter>::const_iterator itEnumFilesInDataStorageParameter = enumFilesInDataStorageParameterList.begin();
						itEnumFilesInDataStorageParameter != enumFilesInDataStorageParameterList.end();
						++itEnumFilesInDataStorageParameter )
			{
				nFileCount += itEnumFilesInDataStorageParameter->fileNameList.size();
			}
			pwndProgressDialog->SetProgressRange( 0, nFileCount );
			pwndProgressDialog->SetProgressPosition( 0 );
			pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Updating ELK Database..." ) ) );
		}

		hash_set<string> usedPaths;

		//переводим TXT в ELK
		for ( list<string>::const_iterator itName = enumFilesInDataStorageParameterList[0].fileNameList.begin(); itName != enumFilesInDataStorageParameterList[0].fileNameList.end(); ++itName )
		{
			CFileStream stream( NVFS::GetMainVFS(), ( *itName ) );
			if ( stream.IsOk() )
			{
				const string szFileName = itName->substr( 0, itName->rfind( '.' ) );
				string szBaseFilePath = szDataBaseFolder + szFileName;
				NStr::ToLower( &szBaseFilePath );
				usedPaths.insert( szBaseFilePath );

				vector<BYTE> buffer0;
				if ( stream.GetSize() > 0 )
				{
					buffer0.resize( stream.GetSize() );
					stream.Read( &( buffer0[0] ), buffer0.size() );
					stream.Seek( 0 );
				}

				SELKTextProperty textProperty;
				LoadTypedSuperXMLResource( szBaseFilePath, XML_EXTENTION, textProperty );

				if ( ( textProperty.nState < 0 ) || ( textProperty.nState >= SELKTextProperty::STATE_COUNT ) )
				{
					textProperty.nState = SELKTextProperty::STATE_NOT_TRANSLATED;
				}

				vector<BYTE> buffer1;
				bool bFileExists = false;
				//
				{
					CFileStream fileStream( StrFmt( _T( "%s%s" ), szBaseFilePath.c_str(), ELK_EXTENTION ), CFileStream::WIN_READ_ONLY );
					if ( fileStream.IsOk()  )
					{
						if ( fileStream.GetSize() > 0 )
						{
							buffer1.resize( fileStream.GetSize() );
							fileStream.Read( &( buffer1[0] ), buffer1.size() );
						}
						bFileExists = true;
					}
				}
				//
				if ( !bFileExists || ( buffer1.size() != buffer0.size() ) || ( memcmp( &( buffer1[0] ), &( buffer0[0] ), buffer1.size() > buffer0.size() ? buffer0.size() : buffer1.size() ) != 0 ) )
				{
					if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
					{
						pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Copying: %s..." ), itName->c_str() ) );
					}
					//
					{
						CFileStream fileStream( StrFmt( _T( "%s%s" ), szBaseFilePath.c_str(), ELK_EXTENTION ), CFileStream::WIN_CREATE );
						if ( fileStream.IsOk()  )
						{
							stream.ReadTo( &fileStream, stream.GetSize() );
						}
					}
					//
					if ( bFileExists && ( textProperty.nState > SELKTextProperty::STATE_OUTDATED ) )
					{
						textProperty.nState = SELKTextProperty::STATE_OUTDATED;
					}
					else
					{
						textProperty.nState = SELKTextProperty::STATE_NOT_TRANSLATED;
					}
					SaveTypedSuperXMLResource( szBaseFilePath, XML_EXTENTION, textProperty );
				}
			}
			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->IterateProgressPosition();
			}
		}

		//обновляем дескрипшны
		for ( int nParameterIndex = 1; nParameterIndex < enumFilesInDataStorageParameterList.size(); ++nParameterIndex )
		{
			for ( list<string>::const_iterator itName = enumFilesInDataStorageParameterList[nParameterIndex].fileNameList.begin(); itName != enumFilesInDataStorageParameterList[nParameterIndex].fileNameList.end(); ++itName )
			{
				if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
				{
					pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Copying: %s..." ), itName->c_str() ) );
				}
				CFileStream stream( NVFS::GetMainVFS(), ( *itName ) );
				if ( stream.IsOk() )
				{
					CFileStream fileStream( szDataBaseFolder + ( *itName ), CFileStream::WIN_CREATE );
					if ( fileStream.IsOk() )
					{
						stream.ReadTo( &fileStream, stream.GetSize() );
					}
				}
				if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
				{
					pwndProgressDialog->IterateProgressPosition();
				}
			}
		}
		//
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Finalising..." ) ) );
		}
		//
		if ( bAbsolute )
		{
			NFile::EnumerateFiles( szDataBaseFolder.c_str(), StrFmt( _T( "*%s" ), ELK_EXTENTION ), CImportFromPAKEraseFile( &usedPaths ), true );
		}
		//
		DeleteFile( ( szDataBaseFolder + DBINDEX_FILE_NAME ).c_str() );
		CELK::DBIndex( szDataBaseFolder );
		if ( pszNewVersion )
		{
			const string szPAKName = rszPAKPath.substr( rszELKPath.rfind( '\\' ) + 1 );
			( *pszNewVersion ) = StrFmt( _T( "%s, [%02d:%02d:%04d, %02d.%02d.%02d]" ),
																					szPAKName.c_str(),
																					statsPAK.mtime.day,
																					statsPAK.mtime.month,
																					statsPAK.mtime.year + 1980,
																					statsPAK.mtime.hours,
																					statsPAK.mtime.minutes,
																					statsPAK.mtime.seconds * 2 );
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::ExportToXLS( const CELK &rELK, const string &rszXLSPath, CELKTreeWindow *pwndELKTreeWindow, int nCodePage, CProgressDialog* pwndProgressDialog )
{
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( StrFmt( _T( "Export to XLS file: %s" ), rszXLSPath.c_str() ) );
	}

	::DeleteFile( rszXLSPath.c_str() );

	if ( pwndELKTreeWindow )
	{
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressRange( 0, pwndELKTreeWindow->GetItemsCountInternal() );
			pwndProgressDialog->SetProgressPosition( 0 );
		}
		
		CDatabase database;
		CString strDriver;
		GetExcelODBCDriverName( &strDriver );
		CString strSql;
    
		TRY
		{
			strSql.Format( _T( "DRIVER={%s};DSN='';FIRSTROWHASNAMES=1;READONLY=FALSE;CREATE_DB=\"%s\";DBQ=%s" ), LPCTSTR( strDriver ), rszXLSPath.c_str(), rszXLSPath.c_str() );

			if ( database.OpenEx( strSql, CDatabase::noOdbcDialog ) )
			{
				strSql = _T( "CREATE TABLE BlitzkriegELK ([Path] TEXT,[Original] LONGTEXT,[Translation] LONGTEXT,[State] TEXT, [Description] LONGTEXT)" );
				database.ExecuteSQL( strSql );
				HTREEITEM item = pwndELKTreeWindow->GetFirstItemInternal(); 
				while ( item )
				{
					string szFileRelPath;
					string szFilePath;
					pwndELKTreeWindow->GetXLSPathInternal( item, &szFileRelPath );
					pwndELKTreeWindow->GetItemPathInternal( item, &szFilePath, true );

					if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
					{
						pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Exporting: %s..." ), szFileRelPath.c_str() ) );
					}
					
					CString strFileRelPath( szFileRelPath.c_str() );
					strFileRelPath.Replace( '\'', '*');

					if ( strFileRelPath.IsEmpty() )
					{
						DebugTrace( _T( "EMPTY PATH!" ) );
					}
					else
					{
						CString strOriginalText;
						GetOriginalText( szFilePath, &strOriginalText, nCodePage, true );
						strOriginalText.Replace( '\'', '`');
						strOriginalText.TrimRight( "\t\r\n " );

						if ( strOriginalText.IsEmpty() )
						{
							DebugTrace( _T( "EMPTY ORIGINAL! %s" ), szFileRelPath );
						}
						else
						{
							CString strDescription;
							GetDescription( szFilePath, &strDescription, nCodePage, true );
							strDescription.Replace( '\'', '`');
							strDescription.TrimRight( "\t\r\n " );
							
							CString strTranslatedText;
							GetTranslatedText( szFilePath, &strTranslatedText, nCodePage, true );
							strTranslatedText.Replace( '\'', '`');
							strTranslatedText.TrimRight( "\t\r\n " );

							bool bTranslated = false;
							int nState = GetState( szFilePath, &bTranslated );
							
							strSql.Format( _T( "INSERT INTO BlitzkriegELK ([Path], [Original], [Translation], [State], [Description]) VALUES (\'%s\',\'%s\',\'%s\',\'%s\',\'%s\')" ),
														LPCTSTR( strFileRelPath ),
														LPCTSTR( strOriginalText ),
														LPCTSTR( strTranslatedText ),
														SELKTextProperty::STATE_NAMES[nState],
														LPCTSTR( strDescription ) );
							database.ExecuteSQL(strSql);
						}
					}
					if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
					{
						pwndProgressDialog->IterateProgressPosition();
					}
					item = pwndELKTreeWindow->GetNextItemInternal( item );
				}
			}
		}
		CATCH(CDBException , pEx)
		{
			pEx->ReportError();
		}
		AND_CATCH(CMemoryException, pEx)
		{
			pEx->ReportError();
		}
		END_CATCH
		database.Close();
	}
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::ImportFromXLS( const CELK &rELK, const string &rszXLSPath, string *pszNewVersion, int nCodePage, CProgressDialog* pwndProgressDialog )
{
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( StrFmt( _T( "Import from XLS file: %s" ), rszXLSPath.c_str() ) );
	}

	NVFS::SFileStats statsXLS;
	{
		Zero( statsXLS );
		NVFS::GetWinFileStats( &statsXLS, rszXLSPath );
	}	
		
	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->SetProgressRange( 0, 1 );
		pwndProgressDialog->SetProgressPosition( 0 );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Updating ELK Database..." ) ) );
	}

  CDatabase database;
	CString strDriver;
	GetExcelODBCDriverName( &strDriver );
	//CString strSql;
	CString strDsn;

	TRY
	{
		strDsn.Format( _T( "ODBC;DRIVER={%s};DSN='';DBQ=%s;MAXSCANROWS=0" ), LPCTSTR( strDriver ), rszXLSPath.c_str() );
		database.Open( NULL, false, false, strDsn );
		CBlitzkriegELKRecordset recset( &database );
		//recset.m_strSort = "Path";
		recset.Open( CRecordset::forwardOnly, 0, CRecordset::readOnly );

		int nOverallStatesCount = 0;
		while( !recset.IsEOF() )
		{
			++nOverallStatesCount;
			recset.MoveNext();
		}

		recset.Close();
		recset.Open( CRecordset::forwardOnly, 0, CRecordset::readOnly );

		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressRange( 0, nOverallStatesCount );
			pwndProgressDialog->SetProgressPosition( 0 );
		}

		int nElementIndex = 0;
		while( !recset.IsEOF() )
		{
			CString strFileName = recset.m_Path;
			strFileName.Replace( '*', '\'' );

			CString strOriginalText = recset.m_Original;
			//recset.GetFieldValue( _T( "Original" ), strOriginalText );
			strOriginalText.Replace( '`', '\'' );
			strOriginalText.TrimRight( "\t\r\n " );

			if ( strOriginalText.IsEmpty() )
			{
				DebugTrace( _T( "EMPTY ORIGINAL! %s\n" ), strFileName );
			}
			
			CString strTranslatedText = recset.m_Translation;
			//recset.GetFieldValue( _T( "Translation" ), strTranslatedText );
			strTranslatedText.Replace( '`', '\'' );

			if ( !strTranslatedText.IsEmpty() )
			{
				string szELKElementName = strFileName;
				szELKElementName = szELKElementName.substr( 0, szELKElementName.find( '\\' ) );
				hash_map<string, int>::const_iterator itElkElementName = rELK.elementNameMap.find( szELKElementName );
				if ( itElkElementName != rELK.elementNameMap.end() )
				{
					nElementIndex = itElkElementName->second; 
					if ( nElementIndex >= 0 && nElementIndex < rELK.elementList.size() )
					{
						string szELKElementPath = strFileName;
						szELKElementPath = szELKElementPath.substr( szELKElementPath.find( '\\' ) + 1 );

						string szDataBaseFolder;
						rELK.elementList[nElementIndex].GetDataBaseFolder( &szDataBaseFolder );
						string szBaseFilePath = szDataBaseFolder + szELKElementPath;

						CString strOriginalTextOnDisk;
						GetOriginalText( szBaseFilePath, &strOriginalTextOnDisk, nCodePage, true );
						strOriginalTextOnDisk.TrimRight( "\t\r\n " );

						bool bOrifginal = false;
						int nState = SELKTextProperty::STATE_TRANSLATED;

						vector<BYTE> buffer0;
						vector<BYTE> buffer1;

						if ( strOriginalTextOnDisk != strOriginalText )
						{
							nState = SELKTextProperty::STATE_OUTDATED;
							bOrifginal = true;
						}

						bool bTranslation = false;
				
						buffer0.clear();
						String2File( &buffer0, strTranslatedText, true, nCodePage, true );
						buffer1.clear();

						bool bWrite = false;
						{
							CFileStream fileStream( StrFmt( _T( "%s%s" ), szBaseFilePath.c_str(), TXT_EXTENTION ), CFileStream::WIN_READ_ONLY );
							if ( fileStream.IsOk() )
							{
								if ( fileStream.GetSize() > 0 )
								{
									buffer1.resize( fileStream.GetSize() );
									fileStream.Read( &( buffer1[0] ), fileStream.GetSize() );
								}
							}
							bWrite = ( buffer0.size() != buffer1.size() ) || 
											 ( memcmp( &( buffer0[0] ), &( buffer1[0] ), buffer0.size() > buffer1.size() ? buffer1.size() : buffer0.size() ) != 0 );
						}
						if ( bWrite )
						{
							if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
							{
								pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Copying: %s..." ), LPCTSTR( strFileName ) ) );
							}

							CFileStream fileStream( StrFmt( _T( "%s%s" ), szBaseFilePath.c_str(), TXT_EXTENTION ), CFileStream::WIN_CREATE );
							if ( fileStream.IsOk() )
							{
								fileStream.Write( &( buffer0[0] ), buffer0.size() );
								bTranslation = true;
							}
						}

						SELKTextProperty textProperty;
						LoadTypedSuperXMLResource( szBaseFilePath, XML_EXTENTION, textProperty );

						if ( ( textProperty.nState < 0 ) || ( textProperty.nState >= SELKTextProperty::STATE_COUNT ) )
						{
							textProperty.nState = SELKTextProperty::STATE_NOT_TRANSLATED;
						}
						if ( bTranslation || bOrifginal )
						{
							textProperty.nState = nState;
							SaveTypedSuperXMLResource( szBaseFilePath, XML_EXTENTION, textProperty );
						}
					}
				}
			}
			recset.MoveNext();
			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->IterateProgressPosition();
			}
		}
	}
	CATCH(CDBException , pEx)
	{
		pEx->ReportError();
	}
	AND_CATCH(CMemoryException, pEx)
	{
		pEx->ReportError();
	}
	END_CATCH
	database.Close();

	if ( pszNewVersion )
	{
		string szXLSName = rszXLSPath.substr( rszXLSPath.rfind( '\\' ) + 1 );
		( *pszNewVersion )	= StrFmt( _T( "%s, [%02d:%02d:%04d, %02d.%02d.%02d]" ),
																				szXLSName.c_str(),
																				statsXLS.mtime.month,
																				statsXLS.mtime.year + 1980,
																				statsXLS.mtime.hours,
																				statsXLS.mtime.minutes,
																				statsXLS.mtime.seconds * 2 );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::CreateStatistic( SELKStatistic *pStatistic, class CELKTreeWindow *pwndELKTreeWindow, const string &rszParentName, int nCodePage, CProgressDialog* pwndProgressDialog )
{
	NI_ASSERT( pStatistic != 0, StrFmt( _T( "CELK::CreateStatistic() wrong parameter: pStatistic %x" ), pStatistic ) );

	if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
	{
		pwndProgressDialog->ShowWindow( SW_SHOW );
		pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Getting file structure..." ) ) );
		pwndProgressDialog->SetWindowText( _T( "Creating Statistics" ) );
	}

	const int nParentNameSize = rszParentName.size();
	pStatistic->Clear();
	if ( pwndELKTreeWindow )
	{
		if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
		{
			pwndProgressDialog->SetProgressRange( 0, pwndELKTreeWindow->GetItemsCountInternal() );
			pwndProgressDialog->SetProgressPosition( 0 );
		}
		HTREEITEM item = pwndELKTreeWindow->GetFirstItemInternal(); 
		while ( item )
		{
			int nELKElementNumber = pwndELKTreeWindow->GetELKElementNumberInternal( item );
			if ( nELKElementNumber >= 0 )
			{
				while ( pStatistic->original.size() <= nELKElementNumber )
				{
					pStatistic->original.push_back( SELKElementStatistic() );	
				}
				while ( pStatistic->translation.size() <= nELKElementNumber )
				{
					pStatistic->translation.push_back( SELKElementStatistic() );	
				}
				SELKElementStatistic &rOriginal = pStatistic->original[nELKElementNumber];
				SELKElementStatistic &rTranslation = pStatistic->translation[nELKElementNumber];

				string szFileRelPath;
				string szFilePath;
				pwndELKTreeWindow->GetXLSPathInternal( item, &szFileRelPath );
				pwndELKTreeWindow->GetItemPathInternal( item, &szFilePath, true );

				if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
				{
					pwndProgressDialog->SetProgressMessage( StrFmt( _T( "Processing Item: %s..." ), szFileRelPath.c_str() ) );
				}
				if ( ( nParentNameSize == 0 ) || ( szFilePath.compare( 0, nParentNameSize, rszParentName ) == 0 ) )
				{
					bool bTranslated = false;
					int nState = GetState( szFilePath, &bTranslated );

					CString strOriginalText;
					GetOriginalText( szFilePath, &strOriginalText, nCodePage, true );

					if ( strOriginalText.IsEmpty() )
					{
						DebugTrace( _T( "EMPTY ORIGINAL! %s\n" ), szFilePath );
					}
					int nWordsCount = 0;
					int nWordSymbolsCount = 0;
					int nSymbolsCount = 0;
					CSpellChecker::GetTextCounts( strOriginalText, nCodePage, &nWordsCount, &nWordSymbolsCount, &nSymbolsCount );

					rOriginal.states[nState].nTextsCount += 1;
					rOriginal.states[nState].nWordsCount += nWordsCount;
					rOriginal.states[nState].nWordSymbolsCount += nWordSymbolsCount;
					rOriginal.states[nState].nSymbolsCount += nSymbolsCount;

					CString strTranslatedText;
					GetTranslatedText( szFilePath, &strTranslatedText, nCodePage, true  );
					CSpellChecker::GetTextCounts( strTranslatedText, nCodePage, &nWordsCount, &nWordSymbolsCount, &nSymbolsCount );
					
					rTranslation.states[nState].nTextsCount += 1;
					rTranslation.states[nState].nWordsCount += nWordsCount;
					rTranslation.states[nState].nWordSymbolsCount += nWordSymbolsCount;
					rTranslation.states[nState].nSymbolsCount += nSymbolsCount;
				}
			}
			
			if ( pwndProgressDialog && ( pwndProgressDialog->GetSafeHwnd() != 0 ) )
			{
				pwndProgressDialog->IterateProgressPosition();
			}
			item = pwndELKTreeWindow->GetNextItemInternal( item );
		}
		pStatistic->bValid = true;
	}
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CELK::UpdateELK( const string &rszPath, const string &rszPAKFileName, class CProgressDialog* pwndProgressDialog )
{
	CELK elk;
	elk.Open( rszPath, false );
	bool bUpdateFileIsUPD = false;

	string szFolder = rszPath.substr( 0, rszPath.rfind( '\\' ) + 1 );
	
	hash_map<string, int> fileMap;
	hash_map<string, string> fileTitleMap;
	
	if ( !rszPAKFileName.empty() )
	{
		string szFileTitle = rszPAKFileName.substr( rszPAKFileName.rfind( '\\' ) + 1 );
		if ( CStringManager::CutFileExtention( &szFileTitle, UPD_EXTENTION ) )
		{
			bUpdateFileIsUPD = true;
		}
		else if ( CStringManager::CutFileExtention( &szFileTitle, PAK_EXTENTION ) )
		{
			bUpdateFileIsUPD = false;
		}
		int nNumber = -1;
		int nPosition = szFileTitle.rfind( '_' );
		if ( nPosition != string::npos )
		{
			string szNumber = szFileTitle.substr( nPosition + 1 );
			szFileTitle = szFileTitle.substr( 0, nPosition );
			if ( sscanf( szNumber.c_str(), _T( "%d" ), &nNumber ) <= 0 )
			{
				nNumber = -1;
			}
		}
		string szFileKey = szFileTitle;
		NStr::ToLower( &szFileKey );
		fileMap[szFileKey] = nNumber;
		fileTitleMap[szFileKey] = szFileTitle;
	}
	else
	{
		//ищем все новые апдейты и их последние версии
		for ( NFile::CFileIterator _NFileIterator( StrFmt( _T( "%s*%s" ), szFolder.c_str(), UPD_EXTENTION ) ); !_NFileIterator.IsEnd(); ++_NFileIterator )
		{
			int nNumber = -1;
			if ( !_NFileIterator.IsDirectory() && !_NFileIterator.IsDots() )
			{
				string szFileTitle = NFile::GetFileTitle( _NFileIterator.GetFileName() );
				int nPosition = szFileTitle.rfind( '_' );
				if ( nPosition != string::npos )
				{
					string szNumber = szFileTitle.substr( nPosition + 1 );
					szFileTitle = szFileTitle.substr( 0, nPosition );
					if ( sscanf( szNumber.c_str(), _T( "%d" ), &nNumber ) <= 0 )
					{
						nNumber = -1;
					}
				}
				string szFileKey = szFileTitle;
				NStr::ToLower( &szFileKey );
				hash_map<string, int>::const_iterator posFile = fileMap.find( szFileKey );
				if ( posFile != fileMap.end() )
				{
					if ( posFile->second < nNumber )
					{
						fileMap[szFileKey] = nNumber;
						fileTitleMap[szFileKey] = szFileTitle;
					}
				}
				else
				{
					fileMap[szFileKey] = nNumber;
					fileTitleMap[szFileKey] = szFileTitle;
				}
			}
		}
	}

	//Update Existing ELK Elements
	for ( vector<SELKElement>::iterator itElement = elk.elementList.begin(); itElement != elk.elementList.end(); )
	{
		string szFileKey = itElement->szPath.substr( itElement->szPath.rfind( '\\' ) + 1 );
		NStr::ToLower( &szFileKey );
		hash_map<string, int>::iterator posFile = fileMap.find( szFileKey );
		if ( posFile != fileMap.end() )
		{
			string szFileTitle =  fileTitleMap[posFile->first];
			if ( szFileTitle.empty() )
			{
				szFileTitle = posFile->first;
			}
			if ( ( !rszPAKFileName.empty() ) || ( itElement->nLastUpdateNumber < posFile->second ) )
			{
				//Update ELKElement
				itElement->nLastUpdateNumber = posFile->second;
				string szPAKFile;
				bool bUPDFile = true;
				if ( !rszPAKFileName.empty() )
				{
					szPAKFile = rszPAKFileName;
					bUPDFile = bUpdateFileIsUPD;
				}
				else
				{
					if (  posFile->second < 0 )
					{
						szPAKFile = StrFmt( _T( "%s%s%s" ), szFolder.c_str(), szFileTitle.c_str(), UPD_EXTENTION );
					}
					else
					{
						szPAKFile = StrFmt( _T( "%s%s_%d%s" ), szFolder.c_str(), szFileTitle.c_str(), posFile->second, UPD_EXTENTION );
					}
				}
				ImportFromPAK( szPAKFile,
											 itElement->szPath,
											 bUPDFile,
											 &( itElement->szVersion ),
											 pwndProgressDialog );
				//Update ELKElement Description
				{
					string szDataBaseFolder;
					itElement->GetDataBaseFolder( &szDataBaseFolder );
					LoadTypedSuperXMLResource( StrFmt( _T( "%s%s" ), szDataBaseFolder.c_str(), CELK::PAK_DESCRIPTION ), CELK::PAK_DESCRIPTION_EXTENTION, itElement->description );
				}
			}
			fileMap.erase( posFile );
			++itElement;
		}
		else
		{
			if ( rszPAKFileName.empty() )
			{
				itElement = elk.elementList.erase( itElement );
			}
			else
			{
				++itElement;
			}
		}
	}

	//Add New ELK Elements
	for ( hash_map<string, int>::const_iterator itFile = fileMap.begin(); itFile != fileMap.end(); ++itFile )
	{
		string szFileTitle =  fileTitleMap[itFile->first];
		if ( szFileTitle.empty() )
		{
			szFileTitle = itFile->first;
		}
		vector<SELKElement>::iterator posELKElement = elk.elementList.insert( elk.elementList.end(), SELKElement() );
		posELKElement->szPath = szFolder + szFileTitle;
		posELKElement->nLastUpdateNumber = itFile->second;
		string szPAKFile;
		bool bUPDFile = true;
		if ( !rszPAKFileName.empty() )
		{
			szPAKFile = rszPAKFileName;
			bUPDFile = bUpdateFileIsUPD;
		}
		else
		{
			if (  itFile->second < 0 )
			{
				szPAKFile = StrFmt( _T( "%s%s%s" ), szFolder.c_str(), szFileTitle.c_str(), UPD_EXTENTION );
			}
			else
			{
				szPAKFile = StrFmt( _T( "%s%s_%d%s" ), szFolder.c_str(), szFileTitle.c_str(), itFile->second, UPD_EXTENTION );
			}
		}
		ImportFromPAK( szPAKFile,
									 posELKElement->szPath,
									 bUPDFile,
									 &( posELKElement->szVersion ),
									 pwndProgressDialog );
		//Update ELKElement Description
		{
			string szDataBaseFolder;
			posELKElement->GetDataBaseFolder( &szDataBaseFolder );
			LoadTypedSuperXMLResource( StrFmt( _T( "%s%s" ), szDataBaseFolder.c_str(), CELK::PAK_DESCRIPTION ), CELK::PAK_DESCRIPTION_EXTENTION, posELKElement->description );
		}
	}

	elk.szPath = rszPath;
	if ( !elk.elementList.empty() )
	{
		elk.Save(); 
	}
	return ( !elk.elementList.empty() );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::UpdateGame( const CELK &rELK,
											 const string &rszZIPToolPath,
											 class CELKTreeWindow *pwndELKTreeWindow,
											 bool bRunGame,
											 int nCodePage,
											 class CProgressDialog* pwndProgressDialog )
{
	string szGameFolder;
	CRegistrySection registrySection( HKEY_LOCAL_MACHINE, KEY_READ, GAME_REGISTRY_FOLDER );
	registrySection.LoadString( GAME_REGISTRY_KEY, &szGameFolder, "" );
	if ( !szGameFolder.empty() )
	{
		CStringManager::CutFileExtention( &szGameFolder, CELK::GAME_FILE_NAME );
		if ( ( !szGameFolder.empty() ) && ( szGameFolder[szGameFolder.size() - 1] != '\\' ) )
		{
			szGameFolder += "\\";
		}
		
		for ( vector<SELKElement>::const_iterator itElement = rELK.elementList.begin(); itElement != rELK.elementList.end(); ++itElement )
		{
			string szFileName = itElement->szPath.substr( itElement->szPath.rfind( '\\' ) + 1 );

			string szPAKPath = szGameFolder + TEXTS_PAK_FILE_NAME + PAK_EXTENTION;
			ExportToPAK( itElement->szPath,
									 szPAKPath,
									 rszZIPToolPath,
									 pwndELKTreeWindow,
									 true,
									 itElement->description.bGenerateFonts,
									 itElement->description.bUsedChars,
									 nCodePage,
									 pwndProgressDialog, 0 );
		}
		ExecuteProcess( ( szGameFolder + string( GAME_FILE_NAME ) ).c_str(), GAME_PARAMETERS,  szGameFolder.c_str(), false );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NDb
{
	void SaveChanges();
	bool RegisterResourceFile( const string &szFileName );
}

static int s_nNumCollectedObjects = 0;
class CFileIndexCollector
{
	const int nCutSize;
public:
	CFileIndexCollector( const string &szBasePath ): nCutSize( szBasePath.size() ) {}
	//
	void operator()( const NFile::CFileIterator &it )
	{
		if ( !it.IsDirectory() )
		{
			NDb::RegisterResourceFile( it.GetFullName().c_str() + nCutSize );
			++s_nNumCollectedObjects;
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELK::DBIndex( const string &rszDBFolder )
{
	CObj<NVFS::IVFS> pMainVFS = NVFS::CreateWinVFS( rszDBFolder );
	CObj<NVFS::IFileCreator> pMainFileCreator = NVFS::CreateWinFileCreator( rszDBFolder );
	NVFS::SetMainVFS( pMainVFS );
	NVFS::SetMainFileCreator( pMainFileCreator );

	NDb::OpenDatabase( pMainVFS, pMainFileCreator, NDb::DATABASE_MODE_EDITOR );
	//
	vector<string> filenames;
	pMainVFS->GetAllFileNames( &filenames, string() );
	for ( vector<string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it )
	{
		const int nSize = it->size();
		if ( it->size() < 4 )
		{
			continue;
		}
		//
		if ( (*it)[nSize - 4] == '.' && 
			   NStr::ASCII_tolower((*it)[nSize - 3]) == 'x' &&
				 NStr::ASCII_tolower((*it)[nSize - 2]) == 'd' &&
				 NStr::ASCII_tolower((*it)[nSize - 1]) == 'b' )
		{
			NDb::RegisterResourceFile( *it );
			++s_nNumCollectedObjects;
		}
	}
	NDb::SaveChanges();
	//
	NDb::CloseDatabase();
	pMainVFS = 0;
	pMainFileCreator = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
