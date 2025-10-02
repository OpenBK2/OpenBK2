#pragma once

struct SHLSLParam
{
	string szName, szType, szBinding;

	SHLSLParam() {}
	SHLSLParam( const string &_szName, const string &_szType, const string &_szBinding ) 
		: szName(_szName), szType(_szType), szBinding(_szBinding) {}
};

struct SHLSLSrcInfo
{
	string szName;
	string szVSName, szPSName;
	vector<SHLSLParam> vsOutput, vsParams, psParams;
	vector< vector<string> > features;
};

void Parse( SHLSLSrcInfo *pRes, const char *psz );
void Compile( const SHLSLSrcInfo &h );

