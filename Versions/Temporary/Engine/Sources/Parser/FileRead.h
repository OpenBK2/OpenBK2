#pragma once

namespace NLang
{
	const char* GetParsingFileName();
	bool OpenFile( const string &szFileName );
	int ReadData( char *pBuf, int nMaxSize );
}


