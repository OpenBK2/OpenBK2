#pragma once

namespace NLang
{
	const char* GetParsingFileName();
	bool OpenFile( const std::string &szFileName );
	int ReadData( char *pBuf, int nMaxSize );
}


