#ifndef __SPELLCHECKER_H__
#define __SPELLCHECKER_H__

#pragma ONCE


#include"csapi.h"

#define BUFFER_LEN ( 0xFFFF )
#define W_LIST_LEN ( 100 )

#define RUSSIAN_CODE	( 0x0419 )
#define USA_CODE			( 0x0409 )
#define BRITISH_CODE	( 0x0809 )


typedef GLOBALSEC ( *TSpellVer )					( WORD  FAR *, WORD FAR *, WORD FAR * );
typedef GLOBALSEC ( *TSpellInit )					( SPLID FAR *, WSC FAR * );
typedef GLOBALSEC ( *TSpellOptions )			( SPLID splid, long nSpellOptions );
typedef GLOBALSEC ( *TSpellCheck )				( SPLID splid, SCCC, LPSIB, LPSRB );
typedef GLOBALSEC ( *TSpellTerminate )		( SPLID splid, BOOL bForce );
typedef GLOBALSEC ( *TSpellOpenMdr )			( SPLID splid, LPSPATH lpspathMain, LPSPATH lpspathExc, BOOL bCreateUdrExc, BOOL bCache, LID lidExpected, LPMDRS lpMdrs );
typedef GLOBALSEC ( *TSpellCloseMdr )			( SPLID splid, LPMDRS lpMdrs );
typedef GLOBALSEC ( *TSpellOpenUdr )			( SPLID splid, LPSPATH lpspathUdr, BOOL bCreateUdr, WORD udrpropType, UDR FAR *lpUdr, BOOL FAR *lpbReadonly );
typedef GLOBALSEC ( *TSpellAddUdr )				( SPLID splid, UDR udr, CHAR FAR *lpszAdd );
typedef GLOBALSEC ( *TSpellCloseUdr )			( SPLID splid, UDR udr, BOOL bForce );
typedef GLOBALSEC ( *TSpellDebugCommand )	( SPLID splid, unsigned short wDebCom );
typedef GLOBALSEC ( *TSpellAddChangeUdr )	( SPLID splid, UDR udr, CHAR FAR *lpszAdd, CHAR FAR *lpszChange );
typedef GLOBALSEC ( *TSpellDelUdr )				( SPLID splid, UDR udr, CHAR FAR *lpszDel );
typedef GLOBALSEC ( *TSpellClearUdr )			( SPLID splid, UDR udr );
typedef GLOBALSEC ( *TSpellGetSizeUdr )		( SPLID splid, UDR udr, WORD FAR *lpcWords );
typedef GLOBALSEC ( *TSpellGetListUdr )		( SPLID splid, UDR udr, WORD iszStart, LPSRB lpSrb );
typedef GLOBALSEC ( *TSpellVerifyMdr )		( LPSPATH lpspathMdr, LID lidExpected, LID FAR *lpLid );

// SpellEngine state values.

class CSpellEngine
{
	friend class CSpellChecker;
	
	struct SLidInfo
	{
		int  nLid;
		TCHAR SN[3];
		LPTSTR pName;
	};
	static const SLidInfo LID_NUM[];

	inline int SelectLidPos( int _nLid )
	{
		int nIndex;
		for	( nIndex = 0; LID_NUM[nIndex].nLid != 0; ++nIndex )
		{
			if ( LID_NUM[nIndex].nLid == _nLid )
			{
				return nIndex;
			}
		}
		return nIndex;
	}

	enum SPELL_ENGINE_STATE
	{
		SE_NOT_INITIALISED,
		SE_NOT_VERIF,
		SE_MDR_NOT_VERIF,
		SE_CDR_NOT_VERIF,
		SE_OK
	};

	int bOpened;
	int bUseUserDict;
	int nState;
	string szEnginePath;
	string szDictPath;
	string szUserDictPath;

	int nWordsCount;
	TCHAR pWords[BUFFER_LEN];
	LPTSTR ppWords[W_LIST_LEN];

	HINSTANCE hLib;
	SPLID id;
	WORD wVer;
	WORD wEng;
	WORD wMode;
	MDRS mdrs;
	UDR uDr;

	int nLanguage;
	int nUdrRO;
	
	BYTE pRatings[BUFFER_LEN];
	SIB	sib;
	SRB	srb;

	TSpellVer						SpellVer;
	TSpellInit					SpellInit;
	TSpellOptions				SpellOptions;
	TSpellCheck					SpellCheck;
	TSpellTerminate			SpellTerminate;
	TSpellOpenMdr				SpellOpenMdr;
	TSpellCloseMdr			SpellCloseMdr;
	TSpellOpenUdr				SpellOpenUdr;
	TSpellAddUdr				SpellAddUdr;
	TSpellCloseUdr			SpellCloseUdr;
	TSpellAddChangeUdr	SpellAddChangeUdr;
	TSpellDelUdr				SpellDelUdr;
	TSpellClearUdr			SpellClearUdr;
	TSpellGetSizeUdr		SpellGetSizeUdr;
	TSpellGetListUdr		SpellGetListUdr;
	TSpellVerifyMdr			SpellVerifyMdr;

	CSpellEngine( int _nLanguage, const string &rszEnginePath, const string &rszDictPath, const string &rszUserDictPath );
	CSpellEngine();
	~CSpellEngine();

	void Open( int _nLanguage, const string &rszEnginePath, const string &rszDictPath, const string &rszUserDictPath );
	void Close();

	int OpenUserDict( const string &rszPath );

	int Suggest( LPCTSTR pWord );		//Get suggestion list
	int SuggestMore();							//Get additional suggestions 
	int Check( LPCTSTR pWord );			//Check word, return 0 - if no errors, 1-14 if any errors exist
	int AddWord( LPCTSTR pWord );		//Add word to custom dictionary
	int CreateList();								//Parse Suggest and SuggestMore output and create words list.
	int Ignore( LPCTSTR pWord );
};

class CSpellChecker
{
	static const TCHAR SPELLING_ENGINE_REGISTRY_SHORT_PATH[];
	static const TCHAR SPELLING_ENGINE_REGISTRY_PATH[];
	static const TCHAR SPELLING_ENGINE_REGISTRY_KEY[];
	static const TCHAR SPELLING_DICTIONARY_REGISTRY_KEY[];
	static const TCHAR SPELLING_CUSTOM_DICTIONARY_REGISTRY_PATH[];
	static const TCHAR SPELLING_CUSTOM_DICTIONARY_REGISTRY_KEY[];
	static const TCHAR WORD_DELIMITERS[];
	static const TCHAR IGNORE_SYMBOLS[];
	static const int DEFAULT_LANGUAGE_INDEX;

	CSpellEngine spellEngine;
	
	int SearchForLanguages( vector<int> *pLanguages );

	static int RemoveFirstDelimiter_CashVersion( CString *pstrText );
	static int GetWord_CashVersion( CString *pstrText, CString *pstrWord );

public:	
	static const char SPELLING_WORD_DELIMITERS[];
	int nCharIndex;

	static int RemoveFirstDelimiter( CString *pstrText );
	static int GetWord( CString *pstrText, CString *pstrWord );
	
	static int GetMBCSSymbolsCount( const CString &rstrText );
	static void GetTextCounts( const CString &rstrText, int nCodePage, int *pWordsCount, int *pWordSymbolsCount, int *pSymbolsCount );
	
	CSpellChecker();
	~CSpellChecker();
	bool IsAvailiable();
	bool Check( const CString &rstrText );
	void Ignore( const CString &rstrText );
	int GetVariants( const CString &rstrText, vector<CString> *pWords );
};

#endif // __SPELLCHECKER_H__
