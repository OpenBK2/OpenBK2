#pragma once

struct SLexem
{
	string szData;

	SLexem() {}
	SLexem( const string &_szData ): szData(_szData) {}
	SLexem( const char *pStart, const char *pFinish ): szData(pStart, pFinish) {}
};

char* GetNextLine( char *p, int *pnLines );
void ParseLine( vector<SLexem> *pRes, const char *p, const char *pFinish );
string StripAfterDot( const char *p, string *pSuffix = 0, string *pPrefix = 0 );
string GetNumber( int n );
bool IsOneOf( const char **pList, const char *p );
string Filter( char *pszSrc );
void SplitString( const char *pszData, vector<string> *pRes );
