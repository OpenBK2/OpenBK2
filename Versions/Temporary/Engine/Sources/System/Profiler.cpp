#include "stdafx.h"

#include "Profiler.h"

namespace NProfiler
{

typedef hash_map<string, DWORD> Times;
static Times *pTimes = new Times;

static struct STimesDeleter
{
	~STimesDeleter()
	{
		delete pTimes;
		pTimes = 0;
	}
} timeDeleter;

void DumpStats()
{
	if ( pTimes == 0 )
		DbgTrc( "can't dump stats" );
	else
	{
		DbgTrc( "============= Profile info =============" );
		
		vector<string> sortedTimes;
		sortedTimes.reserve( pTimes->size() );
		for ( Times::iterator iter = pTimes->begin(); iter != pTimes->end(); ++iter )
			sortedTimes.push_back( iter->first );

		sort( sortedTimes.begin(), sortedTimes.end() );
		for ( int i = 0; i < sortedTimes.size(); ++i )
		{
			const string szProfileInfo = sortedTimes[i];
			DbgTrc( "%s : time %d", szProfileInfo.c_str(), (*pTimes)[szProfileInfo] );
		}

		DbgTrc( "========================================" );
	}
}

CProfiler::CProfiler( const char* pszFile, const int _nLine )
: szFile( pszFile ), nLine( _nLine ), dwStartTime( GetTickCount() )
{
}

CProfiler::~CProfiler()
{
	const DWORD dwTime = GetTickCount() - dwStartTime;
	const string szHash = StrFmt( "%s(%d)", szFile.c_str(), nLine );

	if ( pTimes )
	{
		Times::iterator iter = pTimes->find( szHash );
		if ( iter == pTimes->end() )
			pTimes->insert( Times::value_type( szHash, dwTime ) );
		else
			iter->second += dwTime;
	}
	else
		DbgTrc( "%s(%d) : can't measure", szFile.c_str(), nLine );
}
}

static void Profile( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	NProfiler::DumpStats();
}

START_REGISTER(ProfileCommands)
REGISTER_CMD( "profile", Profile )
FINISH_REGISTER
