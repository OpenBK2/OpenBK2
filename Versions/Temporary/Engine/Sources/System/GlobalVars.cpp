#include "StdAfx.h"

#include "GlobalVars.h"
#include "FilePath.h"
#include "../Misc/StrProc.h"
#include "../Misc/Win32Helper.h"


// ************************************************************************************************************************ //
// **
// ** aliases
// **
// **
// **
// ************************************************************************************************************************ //

typedef hash_map<string, wstring> CAliasMap;
static CAliasMap s_AliasMap;

bool AddAlias( const string &szAliasName, const wstring &wszCommand )
{
	if ( s_AliasMap.find(szAliasName) != s_AliasMap.end() )
		return false;
	s_AliasMap[szAliasName] = wszCommand;
	return true;
}

wstring GetAlias( const string &szAliasName )
{
	CAliasMap::const_iterator pos = s_AliasMap.find( szAliasName );
	if ( pos != s_AliasMap.end() )
		return pos->second;
	return L"";
}

// TODO{ move it to strproc - after alpha version
template <class TChar, class TBrackets = SBracketsTest<TChar> >
class CBracketMulticharSeparator
{
	vector<TChar> stc;										// close brackets stack
	//
	bool IsSeparator( const TChar tChar ) const
	{
		return tChar == ' ' || tChar == '\t' || tChar == '\r';
	}
public:
	CBracketMulticharSeparator( const TChar separator ) { stc.reserve(32); }
	//
	bool operator()( const TChar chr )
	{
		if ( stc.empty() )
		{
			if ( TBrackets::IsOpen(chr) )
				stc.push_back( TBrackets::GetClose(chr) );
			else
			{
				if ( IsSeparator(chr) )
					return true;
			}
		}
		else if ( chr == stc.back() )
			stc.pop_back();
		//
		return false;
	}
};
static void SplitStringWithMultipleBrackets( const wstring &szString, vector<wstring> &szVector )
{
	for ( NStr::CStringIterator<wchar_t, const basic_string<wchar_t>&, CBracketMulticharSeparator<wchar_t, NStr::SQuoteTest<wchar_t> > > it(szString, ' '); !it.IsEnd(); it.Next() )
		szVector.push_back( it.Get() );
}
// TODO}



namespace NGlobal
{

struct SRecord
{
	void *pContext; 
	CmdHandler pCmdHandler;
	VarHandler pVarHandler;
	int nUniqueID;

	SRecord(): pContext( 0 ), pCmdHandler( 0 ), pVarHandler( 0 ), nUniqueID(-1) {}
	SRecord( void *_pCmdContext, CmdHandler _pCmd, int _nID ) : pContext(_pCmdContext), pCmdHandler(_pCmd), pVarHandler(0), nUniqueID(_nID) {}
	SRecord( void *_pVarContext, VarHandler _pCmd, int _nID ) : pContext(_pVarContext), pCmdHandler(0), pVarHandler(_pCmd), nUniqueID(_nID) {}
};
struct SRecordCheck
{
	int n;
	SRecordCheck( int _n ) : n(_n) {}
	bool operator()( const SRecord &r ) const { return r.nUniqueID == n; }
};
struct SCommandInfo
{
	vector<SRecord> data;
	EStorageClass storage;
	CValue sValue, sDefaultValue;
	bool bIsRegistered;
	SCommandInfo() : storage(STORAGE_NONE), bIsRegistered(false) {}
};
typedef hash_map<string, SCommandInfo> TRecordsMap;

class CRecordsMap : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CRecordsMap)
public:
	TRecordsMap recordsMap;
	CPtr<CRecordsMap> pHold;
};
static CRecordsMap *pRecordsMap;
static int bIsExiting;
struct SKillRecordsMap
{
	~SKillRecordsMap() { if ( pRecordsMap ) pRecordsMap->pHold = 0; pRecordsMap = 0; bIsExiting = true; }
} killRecordsMap;
static CRecordsMap* GetRecordsMap()
{
	if ( bIsExiting )
		return new CRecordsMap;
	if ( !pRecordsMap )
	{
		pRecordsMap = new CRecordsMap;
		pRecordsMap->pHold = pRecordsMap;
	}
	return pRecordsMap;
}

static void CmdDefaultHandler( const string &szID, const vector<wstring> &paramsSet, void *pContext );
static void SplitString( const wstring &szCmd, vector<wstring> *pRes );

static int nUniqueVarID = 1;
int RegisterCmd( const string &szID, CmdHandler pHandler, void *pContext )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	SCommandInfo &info = recordsMap[szID];
	info.data.push_back( SRecord( pContext, pHandler, ++nUniqueVarID ) );
	return nUniqueVarID;
}

int RegisterVar( const string &szID, VarHandler pHandler, void *pContext, const CValue &sValue, EStorageClass storage )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	SCommandInfo &info = recordsMap[szID];
	if ( info.data.empty() )
	{
		info.sValue = sValue;
		info.sDefaultValue = sValue;
	}
	info.bIsRegistered = true;
	info.storage = Max( info.storage, storage );
	info.data.push_back( SRecord( pContext, pHandler, ++nUniqueVarID ) );
	return nUniqueVarID;
}

void UnregisterCmd( const string &szID, int nID )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	TRecordsMap::iterator iTemp = recordsMap.find( szID );
	if ( iTemp == recordsMap.end() )
		return;

	SCommandInfo &info = iTemp->second;
	info.data.erase( remove_if( info.data.begin(), info.data.end(), SRecordCheck(nID) ), info.data.end() );
}

void UnregisterVar( const string &szID, int nID )
{
	UnregisterCmd( szID, nID );
}

void RemoveVar( const string &szID )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	TRecordsMap::iterator pos = recordsMap.find( szID );
	if ( pos != recordsMap.end() )
		recordsMap.erase( pos );
}

void GetIDList( vector<string> *pList )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	pList->resize( recordsMap.size() );

	int nCount = 0;
	for ( TRecordsMap::const_iterator iTemp = recordsMap.begin(); iTemp != recordsMap.end(); iTemp++ )
	{
		(*pList)[nCount] = iTemp->first;
		nCount++;
	}
}

void GetVarsByClass( vector< pair<string, CValue> > *pList, EStorageClass eStorageClass )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	pList->reserve( recordsMap.size() );

	for ( TRecordsMap::const_iterator it = recordsMap.begin(); it != recordsMap.end(); ++it )
	{
		if ( it->second.storage == eStorageClass )
			pList->push_back( pair<string, CValue>(it->first, it->second.sValue) );
	}
}

const CValue &GetVar( const string &szID, const CValue &sDefault )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	TRecordsMap::const_iterator iTemp = recordsMap.find( szID );
	if ( iTemp == recordsMap.end() )
		return sDefault;

	return iTemp->second.sValue;
}

static EStorageClass eNewVarStorageClass = STORAGE_DONT_CARE;
void SetVar( const string &szVar, const CValue &sValue, EStorageClass storage )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	TRecordsMap::iterator i = recordsMap.find( szVar );
	if ( i == recordsMap.end() )
	{
		storage = Max( storage, eNewVarStorageClass );
		recordsMap[ szVar ];
		i = recordsMap.find( szVar );
	}
	else
		NI_ASSERT( storage == STORAGE_DONT_CARE || storage == i->second.storage, "different storage classes has been specified for this var" );
	SCommandInfo &info = i->second;
	info.storage = Max( info.storage, storage );
	const vector<SRecord> &records = info.data;
	for ( int k = 0; k < records.size(); ++k )
	{
		const SRecord &sRecord = records[k];
		if ( sRecord.pVarHandler )
			sRecord.pVarHandler( szVar, sValue, sRecord.pContext );
	}
	info.sValue = sValue;
}

void ProcessCommand( const wstring &wsCommandStr )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	vector<wstring> wordsSet;
//	SplitString( wsCommandStr, &wordsSet );
	SplitStringWithMultipleBrackets( wsCommandStr, wordsSet );
	// remove empty strings
	for ( vector<wstring>::iterator it = wordsSet.begin(); it != wordsSet.end(); ) 
	{
		if ( it->empty() ) 
			it = wordsSet.erase( it );
		else if ( it->compare(0, 2, L"//") == 0 )
		{
			wordsSet.erase( it, wordsSet.end() );
			break;
		}
		else
			++it;
	}
	//

	if ( wordsSet.empty() )
		return;

	string szCommandName = NStr::ToMBCS( *wordsSet.begin() );
	wordsSet.erase( wordsSet.begin() );
	// process command alias
	{
		wstring wszAliasCmd = GetAlias( szCommandName );
		if ( !wszAliasCmd.empty() )
		{
			if ( !wordsSet.empty() )
			{
				wszAliasCmd += L" " + wordsSet[0];
				for ( int i = 1; i < wordsSet.size(); ++i )
					wszAliasCmd += L" " + wordsSet[i];
			}
			ProcessCommand( wszAliasCmd );
			return;
		}
	}

	TRecordsMap::iterator iTemp = recordsMap.find( szCommandName );
	if ( iTemp == recordsMap.end() )
	{
		csSystem << CC_RED << "Unknow command name'" << wsCommandStr << "'" << endl;
		csSystem << CC_RED << "Type 'help' to list available commands." << endl;
		return;
	}
	// remove unnecessary quotes
	for ( vector<wstring>::iterator it = wordsSet.begin(); it != wordsSet.end(); ++it )
	{
		if ( it->size() > 2 && (*it)[0] == L'\"' && (*it)[it->size() - 1] == L'\"' )
			*it = it->substr( 1, it->size() - 2 );
	}
	//
	const vector<SRecord> &records = iTemp->second.data;
	bool bWasHandled = false;
	for ( int k = 0; k < records.size(); ++k )
	{
		const SRecord &sRecord = records[k];
		if ( sRecord.pCmdHandler == 0 )
			continue;
		sRecord.pCmdHandler( szCommandName, wordsSet, sRecord.pContext );
		bWasHandled = true;
	}
	if ( !bWasHandled )
		CmdDefaultHandler( szCommandName, wordsSet, 0 );
}

void LoadConfig( const string &szFileName, EStorageClass _newVarStorage )
{
	eNewVarStorageClass = _newVarStorage;
  string szBuffer;

	CFileStream stream( szFileName, CFileStream::WIN_READ_ONLY );
  const int nSize = stream.GetSize();
	if ( nSize == 0 )
		return;
  szBuffer.resize( nSize );
  stream.Read( &(szBuffer[0]), nSize );

	csSystem << "Executing " << szFileName << endl;

	vector<string> cmdsSet;
	NStr::SplitString( szBuffer.c_str(), &cmdsSet, '\n' );
	NWin32Helper::CControl87Guard control87guard;
	_control87( _RC_CHOP | _PC_24, _MCW_RC | _MCW_PC );
	for ( vector<string>::const_iterator iTemp = cmdsSet.begin(); iTemp != cmdsSet.end(); ++iTemp )
	{
	  if ( iTemp->empty() )
	    continue;

	  string szCmd( *iTemp );
	  NStr::TrimBoth( szCmd, "\n\r" );
	  if ( szCmd.substr( 0, 1 ).compare( ";" ) == 0 )
	    continue;
	  if ( szCmd.substr( 0, 2 ).compare( "//" ) == 0 )
	    continue;
		//
		wstring wszCmd;
		NStr::ToUnicode( &wszCmd, szCmd );
	  ProcessCommand( wszCmd );
	}
	eNewVarStorageClass = STORAGE_DONT_CARE;
}

struct SSaveRecord
{
	string szName;
	string szValue;
};
struct SSaveRecordSort
{
	bool operator()( const SSaveRecord &s1, const SSaveRecord &s2 ) const 
	{ 
		return s1.szName < s2.szName; 
	}
};
static void SaveConfig( const string &szFileName, EStorageClass storage )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	vector<SSaveRecord> tempSet;
	for ( TRecordsMap::iterator iTemp = recordsMap.begin(); iTemp != recordsMap.end(); iTemp++ )
	{
		if ( iTemp->second.storage != storage )
			continue;

		if ( iTemp->second.sValue.GetString().empty() )
			continue;

		SSaveRecord &sRec = *tempSet.insert( tempSet.end() );
		sRec.szName = iTemp->first;
		sRec.szValue = NStr::ToMBCS( iTemp->second.sValue.GetString() );
	}
	sort( tempSet.begin(), tempSet.end(), SSaveRecordSort() );

	FILE *pFile = fopen( szFileName.c_str(), "w+" );
	if ( pFile == 0 )
	{
		csSystem << "Can't open " << szFileName << endl;
		return;
	}

	fprintf( pFile, "//============================================================================\n" );
	fprintf( pFile, "// generated by the game, please do not modify\n" );
	fprintf( pFile, "//============================================================================\n" );
	for ( vector<SSaveRecord>::iterator iTemp = tempSet.begin(); iTemp != tempSet.end(); iTemp++ )
	{
		// put global var with spaces in quotes before save
		if ( iTemp->szValue.find(' ') != string::npos )
			iTemp->szValue = '\"' + iTemp->szValue + '\"';
		fprintf( pFile, "setvar %s = %s\n", iTemp->szName.c_str(), iTemp->szValue.c_str() );
	}

	fclose( pFile );
}

void SaveAllVars( const string &szGlobalName, const string &szUserName )
{
	NFile::CreatePath( NFile::GetFilePath( szGlobalName ) );
	SaveConfig( szGlobalName, STORAGE_GLOBAL );
	NFile::CreatePath( NFile::GetFilePath( szUserName ) );
	SaveConfig( szUserName, STORAGE_USER );
}

void ResetVarsToDefault( EStorageClass storage )
{
	CPtr<CRecordsMap> pHold( GetRecordsMap() );
	TRecordsMap &recordsMap = pHold->recordsMap;

	vector<string> toDel;
	for ( TRecordsMap::iterator i = recordsMap.begin(); i != recordsMap.end(); ++i )
	{
		SCommandInfo &info = i->second;
		if ( info.storage == storage )
		{
			if ( !info.bIsRegistered )
				toDel.push_back( i->first );
			info.sValue = info.sDefaultValue;
		}
	}
	for ( int k = 0; k < toDel.size(); ++k )
		recordsMap.erase( recordsMap.find( toDel[k] ) );
}

static void SplitString( const wstring &szCmd, vector<wstring> *pRes )
{
	vector<wstring> params;
	list<WCHAR> stackBrackets;
	int i, nLastPos = 0;
	//
	for ( i = 0; i < szCmd.size(); ++i )
	{
		WCHAR c = szCmd[i];
		if ( NStr::SBracketsTest<wchar_t>::IsOpen(c) )
			stackBrackets.push_back( NStr::SBracketsTest<wchar_t>::GetClose( c ) );
		else if ( stackBrackets.empty() )
		{
			if ( ( c == L' ' ) || ( c == L'\t' ) || ( c == L'\r' ) )
			{
				wstring szRes = szCmd.substr( nLastPos, i - nLastPos );
				if ( !szRes.empty() )
					params.push_back( szRes );
				nLastPos = i + 1; 
			}
		}
		else if ( c == stackBrackets.back() )
			stackBrackets.pop_back();
	}
	// last substring
	if ( nLastPos + 1 <= int( i ) )
		params.push_back( szCmd.substr( nLastPos ) );

	*pRes = params;
}

static void CmdDefaultHandler( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.empty() )
	{
		const CValue &sValue = GetVar( szID, CValue() );
		csSystem << szID << " string = '" << sValue.GetString() << "' float = '" << sValue.GetFloat() << "'" << endl;
		return;
	}

	SetVar( szID, CValue( paramsSet.front() ) );
}

void CmdPrintHelp( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( NGlobal::GetVar( "VVP", 0 ) == 0 )
		return;
	vector<string> varsSet;
	NGlobal::GetIDList( &varsSet );
	sort( varsSet.begin(), varsSet.end() );

	csSystem << CC_BLUE << "Commands:" << endl;
	for ( int nTemp = 0; nTemp < varsSet.size(); nTemp++ )
		csSystem << varsSet[nTemp] << endl;
	csSystem << CC_BLUE << "Total:" << (int)varsSet.size() << endl;
}

static void CmdLoadConfig( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	for ( int nTemp = 0; nTemp < paramsSet.size(); ++nTemp )
		NGlobal::LoadConfig( "..\\" + NStr::ToMBCS( paramsSet[nTemp] ) );
}

static void CmdSetVar( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 3 )
	{
		csSystem << "usage: " << szID << " 'name' = 'value'" << endl;		
		return;
	}

	if ( paramsSet[1].compare( L"=" ) != 0 )
		return;

	NWin32Helper::CControl87Guard control87guard;
	_control87( _RC_CHOP | _PC_24, _MCW_RC | _MCW_PC );
	NGlobal::SetVar( NStr::ToMBCS( paramsSet[0] ), NGlobal::CValue( paramsSet[2] ) );
}

static void CmdAlias( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() < 3 )
	{
		csSystem << "usage: " << szID << " 'name' = 'aliased command'" << endl;		
		return;
	}
	if ( paramsSet[1].compare( L"=" ) != 0 )
		return;
	//
	const string szAliasName = NStr::ToMBCS( paramsSet[0] );
	wstring wszCommand = paramsSet[2];
	for ( int i = 3; i < paramsSet.size(); ++i )
		wszCommand = wszCommand + L" " + paramsSet[i];
	//
	if ( AddAlias( szAliasName, wszCommand ) == false )
		csSystem << "alias " << szAliasName << " already exist!" << endl;		
}

}

START_REGISTER(Commands)
REGISTER_CMD( "help", NGlobal::CmdPrintHelp )
REGISTER_CMD( "exec", NGlobal::CmdLoadConfig )
REGISTER_CMD( "setvar", NGlobal::CmdSetVar )
REGISTER_CMD( "alias", NGlobal::CmdAlias )
FINISH_REGISTER

