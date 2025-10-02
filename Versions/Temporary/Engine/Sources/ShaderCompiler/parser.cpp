#include "StdAfx.h"
#include "parser.h"

char* GetNextLine( char *p, int *pnLines )
{
	while ( p[0] != 0 && p[0] != 10 && p[0] != 13 )
		++p;
	while ( p[0] == 10 || p[0] == 13 )
	{
		*pnLines += p[0] == 10;
		++p;
	}
	return p;
}

void ParseLine( vector<SLexem> *pRes, const char *p, const char *pFinish )
{
	pRes->resize( 0 );
	if ( p == pFinish )
		return;
	// check for comments
	if ( p[0] == ';' )
		return; 
	const char *pStart = p;
	for ( ; p < pFinish; ++p )
	{
		switch ( p[0] )
		{
		case '/':
			if ( p[1] == '/' )
			{
				if ( pStart != p )
					pRes->push_back( SLexem( pStart, p ) );
				return;
			}
			break;
		case 9:
		case 10:
		case 13:
		case ' ':
		case ',':
		case '(':
		case ')':
			if ( pStart != p )
				pRes->push_back( SLexem( pStart, p ) );
			pStart = p + 1;
			break;
		case 'c':
		case 'C':
			// Special case: c[a0.x + Number] is the one lexem
			if ( p < pFinish-1 && p[1] == '[' )
			{
				string szTestLexem;
				szTestLexem.reserve( pFinish - p + 1 );
				while ( p < pFinish && *p != ']' && *p != 0x0d)
				{
					if ( *p != ' ' )
						szTestLexem += *p;

					++p;
				}
				if ( p < pFinish && *p == ']' )
					szTestLexem += *p;

				pRes->push_back( SLexem(szTestLexem) );
				pStart = p + 1;
			}
			break;
		}
	}
}

string StripAfterDot( const char *p, string *pSuffix, string *pPrefix )
{
	if ( pSuffix )
		*pSuffix = "";
	if ( pPrefix )
		*pPrefix = "";
	string res( p );
	if ( res.size() > 0 && res[0] == '-' )
	{
		res = res.substr( 1 );
		if ( pPrefix )
			*pPrefix = "-";
	}
	int n = res.find( '.' );
	if ( n != string::npos )
	{
		if ( pSuffix )
			*pSuffix = res.substr( n );
		res = res.substr( 0, n );
	}
	return res;
}

string GetNumber( int n )
{
	char szBuf[8] = {0,0,0,0,0,0,0,0};
	itoa( n, szBuf, 10 );
	return szBuf;
}

bool IsOneOf( const char **pList, const char *p )
{
	for ( ; pList[0]; ++pList )
	{
		if ( strcmp( pList[0], p ) == 0 )
			return true;
	}
	return false;
}

//string Filter( char *pszSrc )
//{
//	string szRes;
//	for ( ; pszSrc[0]; ++pszSrc )
//	{
//		if ( isalnum( pszSrc[0] ) )
//			szRes += pszSrc[0];
//	}
//	return szRes;
//}
string Filter( char *pszSrc )
{
	string szRes;
	//for ( ; pszSrc[0]; ++pszSrc )
	while ( ( *pszSrc != 0 ) && ( *pszSrc != 10 ) && ( *pszSrc != 13 ) )
	{
		if ( isalnum( *pszSrc ) )
			szRes += *pszSrc;
		++pszSrc;
	}
	return szRes;
}


void SplitString( const char *pszData, vector<string> *pRes )
{
	pRes->clear();
	const char *pszStart = pszData;;
	for(;;)
	{
		while ( pszStart[0] != 0 && isspace( pszStart[0] ) )
			++pszStart;
		string sz;
		while ( !isspace( pszStart[0] ) && pszStart[0] != 0 )
			sz += *pszStart++;
		if ( !sz.empty() )
			pRes->push_back( sz );
		if ( pszStart[0] == 0 )
			break;
	}
}
