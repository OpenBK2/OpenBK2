#include "StdAfx.h"
#include "SpellChecker.h"
#include "..\MapEditorLib\Tools_Registry.h"
#include "MLParser.h"

const CSpellEngine::SLidInfo CSpellEngine::LID_NUM[]=
{
	{ 0x0409, _T( "AM" ), _T( "American English" )		},
	{ 0x0c09, _T( "EA" ), _T( "English Australian" )	},
	{ 0x0809, _T( "BR" ), _T( "English" )							},
	{ 0x0403, _T( "CT" ), _T( "Catalan" )							},
	{ 0x0406, _T( "DA" ), _T( "Danish" )							},
	{ 0x0413, _T( "NL" ), _T( "Dutch" )								},
	{ 0x040b, _T( "FI" ), _T( "Finish" )							},
	{ 0x040c, _T( "FR" ), _T( "French" )							},
	{ 0x0c0c, _T( "FC" ), _T( "French Canadian" )			},
	{ 0x0407, _T( "GE" ), _T( "German" )							},
	{ 0x0410, _T( "IT" ), _T( "Italian" )							},
	{ 0x0414, _T( "NO" ), _T( "Norwegian Bokmal" )		},
	{ 0x0814, _T( "NN" ), _T( "Norwegian Nynorsk" )		},
	{ 0x0416, _T( "PB" ), _T( "Portuguese Brazil" )		},
	{ 0x0816, _T( "PT" ), _T( "Portuguese Iberian" )	},
	{ 0x040a, _T( "SP" ), _T( "Spanish" )							},
	{ 0x041d, _T( "SW" ), _T( "Swedish" )							},
	{ 0x0419, _T( "RU" ), _T( "Russian" )							},
	{ 0x0422, _T( "UA" ), _T( "Ukrainian" )						},
	{ 0x0405, _T( "CZ" ), _T( "Czech" )								},
	{ 0x040e, _T( "HU" ), _T( "Hungarian" )						},
	{ 0x0415, _T( "PL" ), _T( "Polish" )							},
	{ 0x0408, _T( "EL" ), _T( "Greek" )								},
	{ 0x041b, _T( "SK" ), _T( "Slovak" )							},
	{ 0x0424, _T( "SL" ), _T( "Slovenian" )						},
	{ 0x0426, _T( "LV" ), _T( "Latvian" )							},
	{ 0x0807, _T( "SG" ), _T( "Swiss German" )				},
	{ 0x080C, _T( "BE" ), _T( "Belgian" )							},
	{ 0x100C, _T( "SF" ), _T( "Swiss French" )				},
	{ 0x140C, _T( "LF" ), _T( "Luxembourg French" )		},
	{ 0,			_T( "??" ), _T( "Unknown" )							}
};


int CSpellEngine::OpenUserDict( const string &rszPath )
{
	if ( rszPath.empty() )
	{
		nState = SE_CDR_NOT_VERIF;
		sib.cUdr = 0;
		sib.lrgUdr = 0;
		bUseUserDict = false;
		return -1;
	}

	int nResult = -1;
	if ( SpellOpenUdr )
	{
		nResult = SpellOpenUdr( id,
														LPTSTR( rszPath.empty() ? szUserDictPath.c_str() : rszPath.c_str() ),
														1,
														udrIgnoreAlways,
														&uDr,
														&nUdrRO );
	}
	if ( nResult != 0 )
	{
		nState = SE_CDR_NOT_VERIF;
		sib.cUdr  = 0;
		bUseUserDict = false;
	}
	else
	{
		sib.cUdr = 1;
		bUseUserDict = true;
	}
	sib.lrgUdr = ( sib.cUdr > 0 ) ? &uDr : 0;
	return nResult;
}


int CSpellEngine::AddWord( LPCTSTR pWord )
{
	int nResult = ( SpellAddUdr ) ? SpellAddUdr( id, uDr, LPTSTR(pWord) ) : -1;
	if( SpellCloseUdr )
	{ 
		SpellCloseUdr( id, uDr, 1 );
		OpenUserDict( szUserDictPath );
	}
	return nResult;
}

CSpellEngine::CSpellEngine()
	: bOpened( false ), bUseUserDict( false ), nState( SE_NOT_INITIALISED ), nLanguage( 0 ), nWordsCount( 0 ), hLib( 0 )
{
}


CSpellEngine::CSpellEngine( int _nLanguage, const string &rszEnginePath, const string &rszDictPath, const string &rszUserDictPath )
	: bOpened( false ), bUseUserDict( false ), nState( SE_NOT_INITIALISED ), nLanguage( 0 ), nWordsCount( 0 ), hLib( 0 )
{ 
	if ( _nLanguage > 0 )
	{
		Open( _nLanguage, rszEnginePath, rszDictPath, rszDictPath );
	}
}


void CSpellEngine::Close()
{
	if( bOpened )
	{
		if( SpellCloseUdr && ( bUseUserDict > 0 ) )
		{
			SpellCloseUdr( id, uDr, 1 );
		}
		//
		if( SpellCloseMdr )
		{
			SpellCloseMdr( id, &mdrs );
		}
		//
		if( SpellTerminate )
		{
			SpellTerminate( id, true );
		}
		//
		if ( hLib )
		{
			::FreeLibrary( hLib );
			hLib = 0;
		}
		nState=SE_NOT_INITIALISED;
		bOpened = false;
	}
}


void CSpellEngine::Open( int _nLanguage, const string &rszEnginePath, const string &rszDictPath, const string &rszUserDictPath )
{
	Close();

	int nResult = -1;

	nLanguage = _nLanguage;

	szEnginePath = rszEnginePath;
	szDictPath = rszDictPath;
	szUserDictPath = rszUserDictPath;

	nWordsCount = 0;

	if ( hLib )
	{
		::FreeLibrary( hLib );
		hLib = 0;
	}
	if ( szEnginePath.empty() )
	{
		return;
	}
	hLib = LoadLibrary( szEnginePath.c_str() );
	if( !hLib )
	{
		return; 
	}

	SpellVer					= ( TSpellVer )						GetProcAddress( hLib, _T( "SpellVer" ) );     //OnERR(SpellVer,"SpellVer" );
	SpellInit					= ( TSpellInit )					GetProcAddress( hLib, _T( "SpellInit" ) );     //OnERR(SpellInit,"SpellInit");
	SpellOptions			= ( TSpellOptions )				GetProcAddress( hLib, _T( "SpellOptions" ) );  //OnERR(SpellOptions,"SpellOptions");
	SpellCheck				= ( TSpellCheck )					GetProcAddress( hLib, _T( "SpellCheck" ) );    //OnERR(SpellCheck,"SpellCheck");
	SpellTerminate		= ( TSpellTerminate )			GetProcAddress( hLib, _T( "SpellTerminate" ) );//OnERR(SpellTerminate,"SpellTerminate" );
	SpellOpenMdr			= ( TSpellOpenMdr )				GetProcAddress( hLib, _T( "SpellOpenMdr" ) );  //OnERR(SpellOpenMdr,"SpellOpenMdr");
	SpellCloseMdr			= ( TSpellCloseMdr )			GetProcAddress( hLib, _T( "SpellCloseMdr" ) ); //OnERR(SpellCloseMdr,"SpellCloseMdr" );
	SpellCloseUdr			= ( TSpellCloseUdr )			GetProcAddress( hLib, _T( "SpellCloseUdr" ) ); //OnERR(SpellCloseMdr,"SpellCloseMdr" );
	SpellOpenUdr			= ( TSpellOpenUdr )				GetProcAddress( hLib, _T( "SpellOpenUdr" ) );  //OnERR(SpellOpenMdr,"SpellOpenMdr");
	SpellAddUdr				= ( TSpellAddUdr )				GetProcAddress( hLib, _T( "SpellAddUdr" ) );
	SpellAddChangeUdr	= ( TSpellAddChangeUdr )	GetProcAddress( hLib, _T( "SpellAddChangeUdr" ) );
	SpellDelUdr				= ( TSpellDelUdr )				GetProcAddress( hLib, _T( "SpellDelUdr" ) );
	SpellClearUdr			= ( TSpellClearUdr )			GetProcAddress( hLib, _T( "SpellClearUdr" ) );
	SpellGetSizeUdr		= ( TSpellGetSizeUdr )		GetProcAddress( hLib, _T( "SpellGetSizeUdr" ) );
	SpellGetListUdr		= ( TSpellGetListUdr )		GetProcAddress( hLib, _T( "SpellGetListUdr" ) );
	SpellVerifyMdr		= ( TSpellVerifyMdr )			GetProcAddress( hLib, _T( "SpellVerifyMdr" ) );

	nResult = -1;
	if ( SpellVer )
	{
		nResult = SpellVer( &wVer, &wEng, &wMode );
	}

	nResult = -1;
	if ( SpellInit )
	{
		WSC wsc = { 0, 0, 0, 0, 0, 0, 0, { 0, 0 }, { 0, 0 } };
		nResult = SpellInit( &id, &wsc );
	}
	if ( nResult != 0 )
	{ 
		nState = SE_NOT_VERIF;
		return;
	}

	nResult = -1;
	if ( SpellOptions )
	{
		nResult = SpellOptions( id, ( soRateSuggestions | soSuggestFromUserDict | soUseAllOpenUdr ) );
	}

	nResult = -1;
	if ( SpellOpenMdr )
	{
		nResult = SpellOpenMdr( id, LPTSTR( szDictPath.c_str() ), 0, 0, 1, nLanguage, &mdrs );
	}
	if ( nResult != 0 )
	{
		nState = SE_MDR_NOT_VERIF;
		return;
	};

	nState = SE_OK;

	nResult = OpenUserDict( szUserDictPath );
	sib.cMdr  = 1; // MDR - count
	sib.wSpellState = 0;
	sib.lrgMdr = &mdrs.mdr;
	sib.lrgUdr = ( sib.cUdr > 0) ? &uDr : 0;
	sib.lrgch = 0; // bufer to check
	sib.cch = 0; // bufer len

	srb.cch = BUFFER_LEN;
	srb.lrgsz = pWords;
	srb.lrgbRating = pRatings;
	srb.cbRate = BUFFER_LEN;

	bOpened = true;
}


CSpellEngine::~CSpellEngine()
{
	Close();
}


int CSpellEngine::Check( LPCTSTR pWord )
{
	int nResult = -1;
	if( !bOpened )
	{
		return 1;
	}

	sib.wSpellState = 0;
	sib.lrgch = LPTSTR( pWord );
	sib.cch = strlen( pWord );
	srb.scrs = -1;

	if ( SpellCheck )
	{
		nResult = SpellCheck( id, sccVerifyWord, &sib, &srb );
	}
	return srb.scrs;
}


int CSpellEngine::CreateList()
{
	int nZeroPos = 0;
	nWordsCount = 0;
	
	for ( nWordsCount = 0; nWordsCount < srb.csz; ++nWordsCount ) 
	{
		if ( nWordsCount >= W_LIST_LEN )
		{
			break;
		}
		if ( pWords[nZeroPos] != 0 )
		{
			ppWords[nWordsCount] = pWords + nZeroPos;
		}
		for( ; ( nZeroPos < BUFFER_LEN ) && ( pWords[nZeroPos] != 0 ); ++nZeroPos );
		{
			if ( nZeroPos < BUFFER_LEN )
			{
				nZeroPos++;
			}
			else
			{
				break;
			}
		}
	}
	return nWordsCount;
}


int CSpellEngine::Suggest( LPCTSTR pWord )
{
	int nResult = -1;
	if ( !bOpened )
	{
		return nResult;
	}
	nWordsCount = 0;
	//sib.wSpellState = 0;
	sib.lrgch = LPTSTR( pWord );
	sib.cch = strlen( pWord );
	nResult = SpellCheck( id, sccSuggest, &sib, &srb );
	CreateList();
	return nResult;
}



int CSpellEngine::SuggestMore()
{
	int nResult = -1;
	if ( !bOpened )
	{
		return nResult;
	}
	nWordsCount = 0;
	nResult = SpellCheck( id, sccSuggestMore, &sib, &srb );
	CreateList();
	return nResult;
}


int CSpellEngine::Ignore( LPCTSTR pWord )
{
	int nResult = -1;
	if ( !bOpened )
	{
		return nResult;
	}
	if ( SpellAddUdr )
	{
		nResult = SpellAddUdr( id, udrIgnoreAlways, LPTSTR( pWord ) );
	}
	return nResult;
}

//HKEY_LOCAL_MACHINE
const TCHAR CSpellChecker::SPELLING_ENGINE_REGISTRY_SHORT_PATH[] = _T( "Software\\Microsoft\\Shared Tools\\Proofing Tools\\Spelling\\" );
const TCHAR CSpellChecker::SPELLING_ENGINE_REGISTRY_PATH[] = _T( "Software\\Microsoft\\Shared Tools\\Proofing Tools\\Spelling\\%d\\Normal" );
const TCHAR CSpellChecker::SPELLING_ENGINE_REGISTRY_KEY[] = _T( "Engine" );
const TCHAR CSpellChecker::SPELLING_DICTIONARY_REGISTRY_KEY[] = _T( "Dictionary" );

//HKEY_CURRENT_USER
const TCHAR CSpellChecker::SPELLING_CUSTOM_DICTIONARY_REGISTRY_PATH[] = _T( "Software\\Microsoft\\Shared Tools\\Proofing Tools\\Custom Dictionaries" );
const TCHAR CSpellChecker::SPELLING_CUSTOM_DICTIONARY_REGISTRY_KEY[] = _T( "1" );
const TCHAR CSpellChecker::SPELLING_WORD_DELIMITERS[] = _T( " \t\n\r`~!@#$%^&*()-_=+\\|[{]};:'\",<.>/?" );
const TCHAR CSpellChecker::WORD_DELIMITERS[] = _T( " \t\n\r" );
const TCHAR CSpellChecker::IGNORE_SYMBOLS[] = _T( "\n\r" );

const int CSpellChecker::DEFAULT_LANGUAGE_INDEX = 0;


CSpellChecker::CSpellChecker() : nCharIndex( 0 )
{
	vector<int> languages;
	if ( SearchForLanguages( &languages ) > 0 )
	{
		int nLanguage = languages[DEFAULT_LANGUAGE_INDEX];
		string szEnginePath;
		string szDictPath;
		string szCustomDictPath;
		
		{
			string szEngineRegistryKey = StrFmt( SPELLING_ENGINE_REGISTRY_PATH, nLanguage );
			CRegistrySection registrySection( HKEY_LOCAL_MACHINE, KEY_READ, szEngineRegistryKey.c_str() );
			registrySection.LoadString( SPELLING_ENGINE_REGISTRY_KEY, &szEnginePath, "" );
			registrySection.LoadString( SPELLING_DICTIONARY_REGISTRY_KEY, &szDictPath, "" );
		}		
		{
			CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_READ, SPELLING_CUSTOM_DICTIONARY_REGISTRY_PATH );
			registrySection.LoadString( SPELLING_CUSTOM_DICTIONARY_REGISTRY_KEY, &szCustomDictPath, "" );
		}
		
		spellEngine.Open( nLanguage, szEnginePath, szDictPath, szCustomDictPath );
	}
}


CSpellChecker::~CSpellChecker()
{
}


int CSpellChecker::SearchForLanguages( vector<int> *pLanguages )
{
	if ( pLanguages )
	{
		pLanguages->clear();

		LONG result = ERROR_SUCCESS;
		DWORD dwDisposition;
		HKEY hRegistrySection;
		result = ::RegCreateKeyEx( HKEY_LOCAL_MACHINE,
															 SPELLING_ENGINE_REGISTRY_SHORT_PATH,
															 0,
															 0,
															 REG_OPTION_NON_VOLATILE,
															 KEY_READ,
															 0,
															 &hRegistrySection,
															 &dwDisposition );
		if ( result != ERROR_SUCCESS )
		{
			hRegistrySection = 0;
			return 0;
		}
		TCHAR pKeyName[128];
		DWORD nNameSize = 128;
		for ( int nLanguage = 0; nLanguage < 0xFFFF; ++nLanguage )
		{
			result = ::RegEnumKeyEx( hRegistrySection, nLanguage, pKeyName, &nNameSize, 0, 0, 0, 0 );
			if ( result == ERROR_SUCCESS )
			{
				pLanguages->push_back( atoi( pKeyName ) );
			}
			else if ( result == ERROR_NO_MORE_ITEMS )
			{
				break;
			}
		}
		::RegCloseKey( hRegistrySection );
		return pLanguages->size();
	}
	return 0;
}


bool CSpellChecker::IsAvailiable()
{ 
	return spellEngine.bOpened;
}


bool CSpellChecker::Check( const CString &rstrText )
{
	return ( spellEngine.Check( rstrText ) == 0 );
}


void CSpellChecker::Ignore( const CString &rstrText )
{
	spellEngine.Ignore( rstrText );
}



int CSpellChecker::GetVariants( const CString &rstrText, vector<CString> *pWords )
{
	if ( pWords )
	{
		pWords->clear();
	}
	spellEngine.Suggest( rstrText );

	int nWordsCount = 0;
	for ( int nWordIndex = 0; nWordIndex < spellEngine.nWordsCount; ++nWordIndex )
	{
		// Spell API works in ANSI codepage, so words must be converted before printf.
		if ( pWords )
		{
			pWords->push_back( spellEngine.ppWords[nWordIndex] );
			++nWordsCount;
		}
	}

	//SuggestMore expand the list of suggestions
	spellEngine.SuggestMore();
	for ( int nWordIndex = 0; nWordIndex < spellEngine.nWordsCount; ++nWordIndex )
	{
		// Spell API works in ANSI codepage, so words must be converted before printf.
		if ( pWords )
		{
			pWords->push_back( spellEngine.ppWords[nWordIndex] );
			++nWordsCount;
		}
	}
	return nWordsCount;
}


int CSpellChecker::RemoveFirstDelimiter( CString *pstrText )
{
	int nCount = 0;
	if ( pstrText )
	{
		while ( ( pstrText->GetLength() > 0 ) &&
						( ( pstrText->FindOneOf( SPELLING_WORD_DELIMITERS ) == 0 ) ) )
		{
			( *pstrText ) = pstrText->Mid( 1 );
			++nCount;
		}
	}
	return nCount;
}


int CSpellChecker::GetWord( CString *pstrText, CString *pstrWord )
{
	int nCount = 0;
	if ( pstrText )
	{
		nCount = RemoveFirstDelimiter( pstrText );
		int nDelimiterIndex = pstrText->FindOneOf( SPELLING_WORD_DELIMITERS );
		if ( nDelimiterIndex > 0 )
		{
			if ( pstrWord )
			{
				( *pstrWord ) = pstrText->Left( nDelimiterIndex );
			}
			( *pstrText ) = pstrText->Mid( nDelimiterIndex );
		}
		else
		{
			( *pstrWord ) = ( *pstrText );
			pstrText->Empty();
		}
	}
	return nCount;
}


int CSpellChecker::RemoveFirstDelimiter_CashVersion( CString *pstrText )
{
	int nCount = 0;
	if ( pstrText )
	{
		while ( ( pstrText->GetLength() > 0 ) &&
						( ( pstrText->FindOneOf( WORD_DELIMITERS ) == 0 ) ) )
		{
			( *pstrText ) = pstrText->Mid( 1 );
			++nCount;
		}
	}
	return nCount;
}


int CSpellChecker::GetWord_CashVersion( CString *pstrText, CString *pstrWord )
{
	int nCount = 0;
	if ( pstrText )
	{
		nCount = RemoveFirstDelimiter( pstrText );
		int nDelimiterIndex = pstrText->FindOneOf( WORD_DELIMITERS );
		if ( nDelimiterIndex > 0 )
		{
			if ( pstrWord )
			{
				( *pstrWord ) = pstrText->Left( nDelimiterIndex );
			}
			( *pstrText ) = pstrText->Mid( nDelimiterIndex );
		}
		else
		{
			( *pstrWord ) = ( *pstrText );
			pstrText->Empty();
		}
	}
	return nCount;
}



int CSpellChecker::GetMBCSSymbolsCount( const CString &rstrText )
{
	int nSymbols = 0;
	if ( !rstrText.IsEmpty() )
	{
		LPTSTR pSymbols = (LPTSTR)(LPCTSTR)( rstrText );
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
			++nSymbols;
		}
	}
	return nSymbols;
}


void CSpellChecker::GetTextCounts( const CString &rstrText, int nCodePage, int *pWordsCount, int *pWordSymbolsCount, int *pSymbolsCount )
{
	if ( pWordsCount )
	{
		( *pWordsCount ) = 0;
	}
	if ( pWordSymbolsCount )
	{
		( *pWordSymbolsCount ) = 0;
	}
	if ( pSymbolsCount )
	{
		( *pSymbolsCount ) = 0;
	}
	NML::CMLMBCSText mlMBCSText;
	const int nTextCount = NML::Parse( &mlMBCSText, rstrText, true, nCodePage );
	if ( nTextCount > 0 )
	{
		for ( NML::CMLMBCSText::const_iterator itMLMBCSTextPart = mlMBCSText.begin(); itMLMBCSTextPart != mlMBCSText.end(); ++itMLMBCSTextPart )
		{
			CString strText = itMLMBCSTextPart->strText;
			CString strWord;
			while ( !strText.IsEmpty() )
			{
				CSpellChecker::GetWord_CashVersion( &strText, &strWord );
				if ( pWordsCount )
				{
					( *pWordsCount ) += 1;
				}
				if ( pWordSymbolsCount )
				{
					( *pWordSymbolsCount ) += GetMBCSSymbolsCount( strWord );
				}
			}
			//
			if ( pSymbolsCount )
			{
				CString szIgnoreSymbols( IGNORE_SYMBOLS );
				strText = itMLMBCSTextPart->strText;
				for ( int nSymbolIndex = 0; nSymbolIndex < szIgnoreSymbols.GetLength(); ++nSymbolIndex )
				{
					strText.Remove( szIgnoreSymbols[nSymbolIndex] );
				}
				( *pSymbolsCount ) += GetMBCSSymbolsCount( strText );
			}
		}
	}
}


