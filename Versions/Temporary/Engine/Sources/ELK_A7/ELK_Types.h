#if !defined(__ELK_TYPES__)
#define __ELK_TYPES__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\MapEditorLib\Tools_Resources.h"
#include "..\MapEditorLib\Tools_Registry.h"
#include "..\3dMotor\DBscene.h"
//#include "..\Formats\fmtFont.h"

typedef hash_map<WORD, DWORD> CSymbolSet;
typedef vector<WORD> CSymbolList;
struct SSymbolSet
{
	CSymbolSet symbolSet;
	string szFileName;
};
typedef	hash_map<string, SSymbolSet> CSymbolSetMap;
//
struct SELKTextProperty
{
	enum STATE
	{
		STATE_NOT_TRANSLATED		= 0,
		STATE_OUTDATED					= 1,
		STATE_TRANSLATED				= 2,
		STATE_APPROVED					= 3,
		STATE_COUNT							= 4,
	};
	static LPCTSTR STATE_LABELS[STATE_COUNT];
	static LPCTSTR STATE_NAMES[STATE_COUNT];
	int nState;
	bool bTranslated;
	
	SELKTextProperty() : nState( STATE_NOT_TRANSLATED ), bTranslated( false ) {}
	SELKTextProperty( const SELKTextProperty &rELKTextProperty ) : nState( rELKTextProperty.nState ), bTranslated( rELKTextProperty.bTranslated ) {}
	SELKTextProperty& operator=( const SELKTextProperty &rELKTextProperty )
	{
		if( &rELKTextProperty != this )
		{
			nState = rELKTextProperty.nState;
			bTranslated = rELKTextProperty.bTranslated;
		}
		return *this;
	}	
	
	// serializing...
	int operator&( IXmlSaver &xs );
};

struct SELKDescription
{
	string szName;				// имя в дереве
	string szPAKName;			// относительное имя файла, куда необходимо автоматичесаи запаковывать файл
	string szUPDPrefix;		// префикс в именах UPD файлов
	bool bGenerateFonts;	// генерить или нет фонты
	bool bUsedChars;

	SELKDescription() : bGenerateFonts( false ), bUsedChars( false ) {}
	// serializing...
	int operator&( IXmlSaver &xs );
};


struct SELKElement
{
public:
	static const TCHAR DATA_BASE_FOLDER[];
	static const TCHAR DATA_BASE_RESERVE_FOLDER[];

	//получить каталог с базой по статистике
	static void GetDataBaseFolder( const string &rszELKPath, string *pszDataBaseFolder );
	static void GetDataBaseReserveFolder( const string &rszELKPath, string *pszDataBaseReserveFolder );
	void GetDataBaseFolder( string *pszDataBaseFolder ) const;
	void GetDataBaseReserveFolder( string *pszDataBaseReserveFolder ) const;

	SELKDescription description;
	string szPath;					//без расширения!
	string szVersion;
	int nLastUpdateNumber;

	SELKElement() : nLastUpdateNumber( -2 ){}
	SELKElement( const SELKElement &rELKElement )
		: description( rELKElement.description ),
			szPath( rELKElement.szPath ),
			szVersion( rELKElement.szVersion ),
			nLastUpdateNumber( rELKElement.nLastUpdateNumber ) {}
	SELKElement& operator=( const SELKElement &rELKElement )
	{
		if( &rELKElement != this )
		{
			description = rELKElement.description;
			szPath = rELKElement.szPath;
			szVersion = rELKElement.szVersion;
			nLastUpdateNumber = rELKElement.nLastUpdateNumber;
		}
		return *this;
	}	
	int operator&( IXmlSaver &xs );
};


struct SELKElementStatistic
{
	struct SState 
	{
		int nTextsCount;
		int nWordsCount;
		int nWordSymbolsCount;
		int nSymbolsCount;

		SState() : nTextsCount( 0 ), nWordsCount( 0 ), nWordSymbolsCount( 0 ), nSymbolsCount( 0 ) {}
		SState( const SState &rState )
			: nTextsCount( rState.nTextsCount ),
				nWordsCount( rState.nWordsCount ),
				nWordSymbolsCount( rState.nWordSymbolsCount ),
				nSymbolsCount( rState.nSymbolsCount ) {}
		SState& operator=( const SState &rState )
		{
			if( &rState != this )
			{
				nTextsCount = rState.nTextsCount;
				nWordsCount = rState.nWordsCount;
				nWordSymbolsCount = rState.nWordSymbolsCount;
				nSymbolsCount = rState.nSymbolsCount;
			}
			return *this;
		}	

		int operator&( IXmlSaver &xs );
	};

	vector<SState> states; //по стейтам

	SELKElementStatistic()
	{
		states.resize( SELKTextProperty::STATE_COUNT );
	}
	SELKElementStatistic( const SELKElementStatistic &rELKElementStatistic )
		: states( rELKElementStatistic.states )
	{
		states.resize( SELKTextProperty::STATE_COUNT );
	}
	SELKElementStatistic& operator=( const SELKElementStatistic &rELKElementStatistic )
	{
		if( &rELKElementStatistic != this )
		{
			states = rELKElementStatistic.states;
			states.resize( SELKTextProperty::STATE_COUNT );
		}
		return *this;
	}	
	int operator&( IXmlSaver &xs );
};


struct SELKStatistic
{
	vector<SELKElementStatistic> original; //по элементам
	vector<SELKElementStatistic> translation; //по элементам
	bool bValid;

	SELKStatistic() : bValid( false ) {}
	SELKStatistic( const SELKStatistic &rELKStatistic )
		: original( rELKStatistic.original ),
			translation( rELKStatistic.translation ),
			bValid( rELKStatistic.bValid ) {}
	SELKStatistic& operator=( const SELKStatistic &rELKStatistic )
	{
		if( &rELKStatistic != this )
		{
			original = rELKStatistic.original;
			translation = rELKStatistic.translation;
			bValid = rELKStatistic.bValid;
		}
		return *this;
	}	

	int operator&( IXmlSaver &xs );

	void Clear()
	{
		original.clear();
		translation.clear();
		bValid = false;
	}
};


class CELK
{
public:
	static const int DEFAULT_CODE_PAGE;
	static const TCHAR	PAK_FILE_NAME[];
	static const TCHAR	ELK_FILE_NAME[];
	static const TCHAR FOLDER_DESC_FILE_NAME[];

	static const TCHAR GAME_REGISTRY_FOLDER[];
	static const TCHAR GAME_REGISTRY_KEY[];
	static const TCHAR TEXTS_PAK_FILE_NAME[];
	static const TCHAR GAME_FILE_NAME[];
	static const TCHAR GAME_PARAMETERS[];
	static const TCHAR TEMP_FOLDER[];
	static const TCHAR TEMP_FONT_SUB_FOLDER[];
	static const TCHAR FONT_XDB_SUB_FOLDER[];
	static const TCHAR FONT_BIN_SUB_FOLDER[];
	static const TCHAR DEFAULT_FONT_NAME[];
	static const TCHAR FONT_TAG[];
	static const TCHAR GAME_DATA_FOLDER[];
	static const TCHAR FONT_TYPE[];
	static const TCHAR DBINDEX_FILE_NAME[];
	static const TCHAR DBTYPES_FILE_NAME[];

	static const TCHAR PAK_DESCRIPTION[];

	static const TCHAR PAK_EXTENTION[];
	static const TCHAR UPD_EXTENTION[];
	static const TCHAR XML_EXTENTION[];
	static const TCHAR ELK_EXTENTION[];
	static const TCHAR TXT_EXTENTION[];
	static const TCHAR DSC_EXTENTION[];
	static const TCHAR XDB_EXTENTION[];
	static const TCHAR DDS_EXTENTION[];
	static const TCHAR PAK_DESCRIPTION_EXTENTION[];

	static const TCHAR ZIP_EXE[];
	static const TCHAR ELK_CHM[];

public:
	vector<SELKElement> elementList;
	hash_map<string, int> elementNameMap; //не сериализуется, заполняется в Open
	string szPath;	//с расширением
	SELKStatistic statistic; //статистика получаемая во время игры.
	SELKStatistic previousStatistic; //предыдущая статистика
	SEnumFolderStructureParameter enumFolderStructureParameter;
	
public:
	//работа со статистикой
	bool IsOpened() { return ( !szPath.empty() ); }
	bool Open( const string &rszELKPath, bool bEnumFiles );
	bool Save();
	void Close();
	
	//расширения необходимо откинуть!!
	//string szFileName = szFileName.substr( 0, szFileName.rfind( '.' ) );
	//получить различный тексты в UNICODE
	static void GetOriginalText  ( const string &rszTextPath, CString *pstrText, int nCodePage, bool bRemove_0D = false );
	static void GetTranslatedText( const string &rszTextPath, CString *pstrText, int nCodePage, bool bRemove_0D = false );
	static void GetDescription   ( const string &rszTextPath, CString *pstrText, int nCodePage, bool bRemove_0D = false );
	static int GetState( const string &rszTextPath, bool *pbTranslated );

	//расширения необходимо откинуть!!
	//string szFileName = szFileName.substr( 0, szFileName.rfind( '.' ) );
	//установить различные тексты в UNICODE
	static void SetTranslatedText( const string &rszTextPath, const CString &rstrText, int nCodePage, bool bAdd_0D = false );
	static int SetState( const string &rszTextPath, int nState, bool *pbTranslated ); //возвращает предыдущий стейт
	//
	static NDb::SFont::ECharset GetCharset( int nCodePage );
	static void GenerateFonts( const string &rszDataBaseFolder,
														 const string &rszPAKPath,
														 CSymbolSetMap *pSymbolMap,
														 int nCodePage,
														 bool bUsedChars );
	//
	//создать PAK файл
	static bool CreatePAK( const string &rszGamePath, const string &rszFilePath, const string &rszZIPToolPath, class CProgressDialog* pwndProgressDialog = 0 );
	//запаковать переведенные тексты ( APPROVED )
	//только для SELKElement
	static bool ExportToPAK( const string &rszELKPath,
													 const string &rszPAKPath,
													 const string &rszZIPToolPath,
													 class CELKTreeWindow *pwndELKTreeWindow,
													 bool bOnlyFilled,
													 bool bGenerateFonts,
													 bool bUsedChars,
													 int nCodePage,
													 class CProgressDialog* pwndProgressDialog = 0,
													 const struct SSimpleFilter *pELKFilter = 0 );
	static bool ImportFromPAK( const string &rszPAKPath, const string &rszELKPath, bool bAbsolute, string *pszNewVersion, class CProgressDialog* pwndProgressDialog = 0 );

	//распаковать PAK в ELK c апдейтом состояний
	//для всего CELK ( впереди ставится номер SELKElement )
	static bool ExportToXLS( const CELK &rELK, const string &rszXLSPath, class CELKTreeWindow *pwndELKTreeWindow, int nCodePage, class CProgressDialog* pwndProgressDialog = 0 );
	static bool ImportFromXLS( const CELK &rELK, const string &rszXLSPath, string *pszNewVersion, int nCodePage, class CProgressDialog* pwndProgressDialog = 0 );

	static bool CreateStatistic( SELKStatistic *pStatistic, class CELKTreeWindow *pwndELKTreeWindow, const string &rszParentName, int nCodePage, class CProgressDialog* pwndProgressDialog = 0 );

	//взять и проапдейтить все паки, начиная с указанного
	static bool UpdateELK( const string &rszPath, const string &rszPAKFileName, class CProgressDialog* pwndProgressDialog = 0 );
	//взять базу из указанного места и положить ее в игру, при этом запускать игру или нет
	static void UpdateGame( const CELK &rELK,
													const string &rszZIPToolPath,
													class CELKTreeWindow *pwndELKTreeWindow,
													bool bRunGame,
													int nCodePage,
													class CProgressDialog* pwndProgressDialog = 0 );

	//
	static void DBIndex( const string &rszDBFolder );

	// serializing...
	int operator&( IXmlSaver &xs );
};


typedef list<string> CSimpleFilterItem;
typedef list<CSimpleFilterItem> CSimpleFilter;
struct SSimpleFilter
{
	CSimpleFilter filter;
	bool bTranslated;
	vector<int> states;

	SSimpleFilter() : bTranslated( false )
	{
		states.resize( SELKTextProperty::STATE_COUNT, true );
	}
	SSimpleFilter( const SSimpleFilter &rSimpleFilter ) : filter( rSimpleFilter.filter ), bTranslated( rSimpleFilter.bTranslated ), states( rSimpleFilter.states )
	{
		states.resize( SELKTextProperty::STATE_COUNT, true );
	}
	SSimpleFilter& operator=( const SSimpleFilter &rSimpleFilter )
	{
		if( &rSimpleFilter != this )
		{
			filter = rSimpleFilter.filter;
			bTranslated = rSimpleFilter.bTranslated;
			states = rSimpleFilter.states;
			states.resize( SELKTextProperty::STATE_COUNT, true );
		}
		return *this;
	}	

	bool Check( const string &rszFolder, bool _bTranslated, int nState ) const;
	int operator&( IXmlSaver &xs );
};
typedef hash_map<string, SSimpleFilter> CFilterMap;


struct SKPZeroFunctional
{
  const hash_map<WORD, WORD> *pTranslate;
	SKPZeroFunctional() : pTranslate( 0 )	{}
	SKPZeroFunctional( const hash_map<WORD, WORD> *_pTranslate )
	{
		pTranslate = _pTranslate;
	}

	bool operator()( const KERNINGPAIR &kp ) const
	{ 
		if ( pTranslate->find( kp.wFirst ) == pTranslate->end() )
		{
			return true;
		}
		if ( pTranslate->find( kp.wSecond ) == pTranslate->end() )
		{
			return true;
		}
		return kp.iKernAmount == 0;
	}
};


struct SMainFrameParams
{
	struct SSearchParam
	{
		enum WINDOW_TYPE
		{
			WT_ORIGINAL			= 0,
			WT_DESCRIPTION	= 1,
			WT_TRANSLATION	= 2,
			WT_COUNT				= 3,
		};

		bool bFindDown;
		bool bFindMatchCase;
		bool bFindWholeWord;

		CString strFindText;

		int nWindowType;
		int nPosition;

		SSearchParam() : bFindDown( true ), bFindMatchCase( false ), bFindWholeWord( false ), nWindowType( WT_ORIGINAL ), nPosition( 0 ) {}
		int operator&( IXmlSaver &xs );
	};
	
	static int INVALID_FILTER_NUMBER;	
	
	string szCurrentFolder;
	string szZIPToolPath;
	string szHelpFilePath;
	bool bCollapseItem;

	string szLastOpenedELKName;
	string szLastOpenedPAKName;
	string szLastOpenedXLSName;
	string szPreviousPath;
	string szLastPath;
	int nLastELKElement;
	list<string> recentList;

	CFilterMap filterMap;
	string szCurrentFilterName;

	int nCodePage;

	bool bFullScreen;
	CTRect<int> rect;

	string szLastOpenedPAKShortName;
	string szGameFolder;

	SSearchParam searchParam;

	SMainFrameParams()
		: nLastELKElement( 0 ),
			bCollapseItem( false ),
			nCodePage( CELK::DEFAULT_CODE_PAGE ),
			bFullScreen( false )
	{}
	int operator&( IXmlSaver &xs );

	void ValidatePath( string *pszPath, bool bFolder );
	void LoadFromRegistry( const string &rszRegistryKey, bool bShortApperence );
	void SaveToRegistry( const string &rszRegistryKey, bool bShortApperence );

	const SSimpleFilter* GetCurrentFilter() const;
};

#endif // !defined(__ELK_TYPES__)
