#include "StdAfx.h"
#include "hlsl.h"
#include "parser.h"
#include "Data.h"
#include <d3dx9.h>
#pragma comment( lib, "d3dx9.lib" )

void Parse( SHLSLSrcInfo *pRes, const char *psz )
{
	vector<string> words;
	SplitString( psz, &words );
	if ( words.empty() )
		return;
	if ( words[0] == "vs" )
		pRes->szVSName = words[1];
	else if ( words[0] == "ps" )
		pRes->szPSName = words[1];
	else if ( words[0] == "vsoutput" )
		pRes->vsOutput.push_back( SHLSLParam( words[2], words[1], words[3] ) );
	else if ( words[0] == "feature_switch" )
	{
		vector<string> options;
		for ( int k = 1; k < words.size(); ++k )
			options.push_back( words[k] );
		pRes->features.push_back( options );
	}
	else if ( words[0] == "feature" )
	{
		vector<string> options;
		options.push_back( "" );
		options.push_back( words[1] );
		pRes->features.push_back( options );
	}
	else if ( words[0] == "vsparam" )
		pRes->vsParams.push_back( SHLSLParam( words[2], words[1], words[3] ) );
	else if ( words[0] == "psparam" )
		pRes->psParams.push_back( SHLSLParam( words[2], words[1], words[3] ) );
}

static void AssignIndices( vector<SHLSLParam> *pRes )
{
	hash_map<string,int> lastIndices;
	for ( int k = 0; k < pRes->size(); ++k )
	{
		SHLSLParam &p = (*pRes)[k];
		hash_map<string,int>::iterator i = lastIndices.find( p.szBinding );
		if ( i == lastIndices.end() )
		{
			lastIndices[ p.szBinding ] = 0;
			i = lastIndices.find( p.szBinding );
		}
		p.szBinding = p.szBinding + GetNumber( i->second );
		++i->second;
	}
}

static void WriteRootFile( const string &_szFName, 
	const vector<SHLSLParam> &vsOutput, const vector<SHLSLParam> &params, const vector<string> &defines, const string &szInclude )
{
	ofstream root( _szFName.c_str() );
	for ( int k = 0; k < defines.size(); ++k )
	{
		if ( defines[k] == "" )
			continue;
		root << "#define " << defines[k] << endl;
	}
	for ( int k = 0; k < params.size(); ++k )
	{
		const SHLSLParam &p = params[k];
		root << "const " << p.szType << " " << p.szName << " : register(" << p.szBinding << ");" << endl;
	}
	root << "struct vs_output {\n";
	root << "  float4 Pos : POSITION;\n";
	for ( int k = 0; k < vsOutput.size(); ++k )
	{
		const SHLSLParam &p = vsOutput[k];
		root << "  " << p.szType << " " << p.szName << " : " << p.szBinding << ";" << endl;
	}
	root << "};\n\n";
	root << "#include \"" << szInclude << "\"" << endl;
}

static LPD3DXBUFFER CompileShader( const char *pszFName, const char *pszFuncName, const char *pszProfile, string *pszError )
{
	LPD3DXBUFFER pCode = 0, pError = 0;
	DWORD dwShaderFlags = 0; //D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;//
	HRESULT hr;
	hr = D3DXCompileShaderFromFile( pszFName, NULL, NULL, pszFuncName, pszProfile, dwShaderFlags, &pCode, &pError, 0 );
	if ( hr != D3D_OK && pError )
	{
		if ( pszError )
			*pszError = (const char*)pError->GetBufferPointer();
		if ( pCode )
			pCode->Release();
		pError->Release();
		return 0;
	}
	else
		ASSERT( hr == D3D_OK );
	if ( pError )
		pError->Release();
	return pCode;
}

static bool CanBeCompiled( const vector<SHLSLParam> &vsOutput, const vector<SHLSLParam> &params, const vector<string> &defines, 
	const char *pszFName, const char *pszFunc, const char *pszProfile, string *pszError )
{
	WriteRootFile( "temp.fx", vsOutput, params, defines, pszFName );
	LPD3DXBUFFER pCode = CompileShader( "temp.fx", pszFunc, pszProfile, pszError );
	DeleteFile( "temp.fx" );
	if ( pCode )
		pCode->Release();
	return pCode != 0;
}

static void CompileShader( const vector<SHLSLParam> &vsOutput, const vector<SHLSLParam> &params, const vector<string> &defines, 
	const char *pszFName, const char *pszFunc, const char *pszProfile, vector<DWORD> *pRes )
{
	pRes->resize(0);
	WriteRootFile( "temp.fx", vsOutput, params, defines, pszFName );
	LPD3DXBUFFER pCode = CompileShader( "temp.fx", pszFunc, pszProfile, 0 );
	DeleteFile( "temp.fx" );
	if ( pCode )
	{
		int nSize = pCode->GetBufferSize();
		pRes->resize( ( nSize + 3 ) / 4, 0 );
		memcpy( &(*pRes)[0], pCode->GetBufferPointer(), nSize );
		pCode->Release();
	}
}

static void ReadFile( const char *pszName, vector<string> *pRes )
{
	pRes->resize(0);
	std::ifstream f( pszName );
	while ( !f.eof() && !f.bad() )
	{
		char buf[10000];
		f.getline( buf, 10000 );
		pRes->push_back( buf );
	}
}

struct SShaderFile
{
	vector<string> lines;
	vector<bool> commented;
};
static void ReadFile( const char *pszName, SShaderFile *pRes )
{
	ReadFile( pszName, &pRes->lines );
	pRes->commented.resize( pRes->lines.size(), false );
}
static void WriteFile( const char *pszName, const SShaderFile &src )
{
	ofstream f( pszName );
	for ( int k = 0; k < src.lines.size(); ++k )
	{
		if ( src.commented[k] )
			f << "//";
		f << src.lines[k] << endl;
	}
}

static void TypeSrcInfo( const string &szFile, const vector<string> &defines )
{
	cout << "compiling " << szFile << " with defines : ";
	for ( int k = 0; k < defines.size(); ++k )
		cout << defines[k] << " ";
	cout << endl;
}

static bool CompileShaders( SHLSLShader *pRes, const SHLSLSrcInfo &h, const vector<string> &defines, int _nID )
{
	vector<SHLSLParam> vsOutput = h.vsOutput;
	AssignIndices( &vsOutput );
	
	string szError;
	// try PS
	if ( !CanBeCompiled( vsOutput, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_2_0", &szError ) )
	{
		TypeSrcInfo( h.szPSName, defines );
		cout << "compilation error\n" << szError << endl;
		return false;
	}
	// try vs
	if ( !CanBeCompiled( vsOutput, h.vsParams, defines, h.szVSName.c_str(), "VS", "vs_2_0", &szError ) )
	{
		TypeSrcInfo( h.szVSName, defines );
		cout << "compilation error\n" << szError << endl;
		return false;
	}

	// reduce vs output keeping ps compiling
	vsOutput = h.vsOutput;
	for ( int k = 0; k < vsOutput.size(); ++k )
	{
		vector<SHLSLParam> test = vsOutput;
		test.erase( test.begin() + k );
		AssignIndices( &test );
		if ( CanBeCompiled( test, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_2_0", 0 ) )
		{
			vsOutput.erase( vsOutput.begin() + k );
			--k;
		}
	}
	AssignIndices( &vsOutput );

	// compile vs by commenting out offending lines
	SShaderFile file;
	ReadFile( h.szVSName.c_str(), &file );

	const string szTempFileName = "tempVS.fx";
	bool bVSHasCompiled = false;
	for(;;)
	{
		WriteFile( szTempFileName.c_str(), file );
		string szError;
		bool bOk = CanBeCompiled( vsOutput, h.vsParams, defines, szTempFileName.c_str(), "VS", "vs_2_0", &szError );
		DeleteFile( szTempFileName.c_str() );
		if ( bOk )
		{
			bVSHasCompiled = true;
			break;
		}
		// analyze error
		int nPos = szError.find( szTempFileName );
		if ( nPos == string::npos )
			break;
		int nLine;
		sscanf( szError.c_str() + nPos + szTempFileName.size(), "(%d)", &nLine );
		file.commented[ nLine - 1 ] = true;
	}
	if ( !bVSHasCompiled )
	{
		TypeSrcInfo( h.szVSName, defines );
		cout << "failed to comment out lines in vertex shader file to get it compiled with reduced vs output" << endl;
		return false;
	}

	// compile for all profiles
	string szName = "HLSL" + h.szName + GetNumber( _nID );
	SVShader vs;
	vs.szName = szName;
	WriteFile( szTempFileName.c_str(), file );
	CompileShader( vsOutput, h.vsParams, defines, szTempFileName.c_str(), "VS", "vs_1_1", &vs.vsShader11 );
	CompileShader( vsOutput, h.vsParams, defines, szTempFileName.c_str(), "VS", "vs_2_0", &vs.vsShader20 );
	DeleteFile( szTempFileName.c_str() );

	SPShader ps;
	ps.szName = szName;
	CompileShader( vsOutput, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_1_1", &ps.psShader11 );
	CompileShader( vsOutput, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_1_4", &ps.psShader14 );
	CompileShader( vsOutput, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_2_0", &ps.psShader20 );
	CompileShader( vsOutput, h.psParams, defines, h.szPSName.c_str(), "PS", "ps_2_a", &ps.psShader20a );

	pRes->nPixelShaderNumber = pixelShaders.size();
	pRes->nVertexShaderNumber = vertexShaders.size();
	pixelShaders.push_back( ps );
	vertexShaders.push_back( vs );

	return true;
}

void Compile( const SHLSLSrcInfo &h )
{
	SHLSLShaderGroup sg;
	sg.szName = h.szName;

	vector<int> featureSelector, featureMult;
	featureSelector.resize( h.features.size(), 0 );
	featureMult.resize( h.features.size() );
	int nFeatureMult = 1;
	for ( int k = 0; k < featureMult.size(); ++k )
	{
		featureMult[k] = nFeatureMult;
		nFeatureMult *= h.features[k].size();
	}

	sg.shaders.resize( nFeatureMult );

	//output defines
	for ( int k = 0; k < h.features.size(); ++k )
	{
		for ( int i = 0; i < h.features[k].size(); ++i )
		{
			if ( h.features[k][i] == "" )
				continue;
			sg.defines.push_back( h.features[k][i] );
			sg.defineIDs.push_back( featureMult[k] * i );
		}
	}

	// compile shaders with all feature combinations
	for(;;)
	{
		vector<string> defines;
		for ( int k = 0; k < featureSelector.size(); ++k )
			defines.push_back( h.features[k][ featureSelector[k] ] );

		int nID = 0;
		for ( int k = 0; k < featureSelector.size(); ++k )
			nID += featureSelector[k] * featureMult[k];

		SHLSLShader res;
		CompileShaders( &res, h, defines, nID );

		sg.shaders[ nID ] = res;

		bool bOk = false;
		for ( int k = 0; k < featureSelector.size(); ++k )
		{
			++featureSelector[k];
			if ( featureSelector[k] == h.features[k].size() )
				featureSelector[k] = 0;
			else
			{
				bOk = true;
				break;
			}
		}
		if ( !bOk )
			break;
	}
	hlslShaders.push_back( sg );
}
