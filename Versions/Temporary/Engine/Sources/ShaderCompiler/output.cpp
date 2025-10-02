#include "StdAfx.h"
#include "output.h"
#include "Data.h"
#include "parser.h"


static void WriteRS( ofstream &f, const SStates &s, const char *pszName, const char *pszPrefix )
{
	f << "static SRenderState rs" << pszPrefix << pszName << "[] = { " << endl;
	for ( int i = 0; i < s.rs.size(); ++i )
		f << "{ " << s.rs[i].sz1.c_str() << ", " << s.rs[i].sz2.c_str() << "}, " << endl;
	f << "{(D3DRENDERSTATETYPE)0,0} };" << endl;
	f << "static STextureStageState tss" << pszPrefix << pszName << "[] = { " << endl;
	for ( int i = 0; i < s.tss.size(); ++i )
		f << "{ " << s.tss[i].sz1.c_str() << ", " << s.tss[i].sz2.c_str() << ", " << s.tss[i].sz3.c_str() << "}, " << endl;
	f << "{-1,(D3DTEXTURESTAGESTATETYPE)0,0} };" << endl;
}

static void WriteShader( ofstream &f, const DWORD *pRes )
{
	f << "{ 0x";
	f << hex;
	int i;
	for ( i = 0; (pRes[i] & 0xffff ) != 0xffff; ++i )
		f << pRes[i] << ", 0x";
	f << pRes[i] << " };" << dec << endl;
}

static void WriteShader( ofstream &f, const string &szName, const vector<DWORD> &shader, string *pszDeclaration )
{
	if ( !pszDeclaration->empty() )
		*pszDeclaration += ",";
	if ( shader.empty() )
	{
		*pszDeclaration += "0";
		return;
	}
	f << "static DWORD " << szName << "[] =";
	WriteShader( f, &shader[0] );
	*pszDeclaration += szName;
}

static bool AreFilesDifferent( const string & szFileName0, const string & szFileName1 )
{
	// Ripped STL force me to create this shit
	FILE *pFile0 = fopen( szFileName0.c_str(), "rb" );
	FILE *pFile1 = fopen( szFileName1.c_str(), "rb" );

	bool bDifferent = false;
	if ( !pFile0 || !pFile1 )
		bDifferent = true;
	else
	{
		fseek( pFile0, 0, SEEK_END );
		fseek( pFile1, 0, SEEK_END );

		int nLength0 = ftell( pFile0 );
		int nLength1 = ftell( pFile1 );

		if ( nLength0 == nLength1 )
		{
			fseek( pFile0, 0, SEEK_SET );
			fseek( pFile1, 0, SEEK_SET );

			int nLength = nLength0;

			char *pBuffer0 = new char[nLength];
			char *pBuffer1 = new char[nLength];

			fread( pBuffer0, 1, nLength, pFile0 );
			fread( pBuffer1, 1, nLength, pFile1 );

			for ( int iIndex = 0; iIndex < nLength; iIndex++ )
			{
				if ( pBuffer0[iIndex] != pBuffer1[iIndex] )
				{
					bDifferent = true;
					break;
				}
			}

			delete [] pBuffer0;
			delete [] pBuffer1;
		}
		else
			bDifferent = true;
	}

	if ( pFile0 )
		fclose( pFile0 );

	if ( pFile1 )
		fclose( pFile1 );

	return bDifferent;
}


void WriteResult( const char *pszOutput )
{
	// write .h file
	{
		const string szTempFileName( string(pszOutput) + ".tmp" );
		const string szDestFileName( string(pszOutput) + ".h" );

		ofstream fh( szTempFileName.c_str() );
		fh << "#ifndef __" << pszOutput << "_H__" << endl;
		fh << "#define __" << pszOutput << "_H__" << endl;
		//fh << "#include \"GfxShadersDescr.h\"" << endl;
		//fh << "namespace NGfx" << endl;
		//fh << "{" << endl;
		fh << "struct SVShader;" << endl;
		fh << "struct SPShader;" << endl;
		fh << "struct SHLSLShader;" << endl;
		for ( int k = 0; k < vertexShaders.size(); ++k )
		{
			fh << "extern SVShader vs" << vertexShaders[k].szName << ";" << endl;
		}
		fh << endl << "extern SVShader *vsAllShaders[" << vertexShaders.size() << "];" << endl;
		fh << endl;
		for ( int k = 0; k < pixelShaders.size(); ++k )
		{
			fh << "extern SPShader ps" << pixelShaders[k].szName << ";" << endl;
		}
		fh << endl << "extern SPShader *psAllShaders[" << pixelShaders.size() << "];" << endl;
		fh << endl;

		for ( int k = 0; k < hlslShaders.size(); ++k )
		{
			const SHLSLShaderGroup &g = hlslShaders[k];
			fh << "extern SHLSLShader hlsl" << g.szName << "[" << g.shaders.size() << "];" << endl;
			fh << "namespace NShader" << endl << "{" << endl;
			for ( int k = 0; k < g.defines.size(); ++k )
				fh << "  const int " << g.defines[k] << " = " << g.defineIDs[k] << ";" << endl;
			fh << "}" << endl << endl;
		}
		//fh << "}" << endl;
		fh << "#endif" << endl;
		fh.close();

		// Check file for changes
		if ( AreFilesDifferent( szTempFileName, szDestFileName ) )
		{
			CopyFile( szTempFileName.c_str(), szDestFileName.c_str(), false );							
		}
		DeleteFile( szTempFileName.c_str() );
	}

	// write .cpp file
	{
		const string szTempFileName( string(pszOutput) + ".tmp" );
		const string szDestFileName( string(pszOutput) + ".cpp" );

		ofstream f( szTempFileName.c_str() );

		f << "#include \"StdAfx.h\"" << endl;
		f << "#include \"GfxShadersDescr.h\"" << endl;
		f << "#include \"" << pszOutput << ".h\"" << endl;
		//f << "namespace NGfx" << endl;
		//f << "{" << endl;
		for ( int k = 0; k < vertexShaders.size(); ++k )
		{
			SVShader &v = vertexShaders[k];

			string szDeclaration;
			WriteShader( f, "dwvs11" + v.szName, v.vsShader11, &szDeclaration );
			WriteShader( f, "dwvs20" + v.szName, v.vsShader20, &szDeclaration );
			f << "SVShader vs" << v.szName << "( " << k + 1 << ", " << szDeclaration << ");" << endl;
		}
		for ( int k = 0; k < pixelShaders.size(); ++k )
		{
			SPShader &v = pixelShaders[k];

			string szDeclaration;
			//if ( pCode && ((DWORD*)pCode->GetBufferPointer())[0] == 0xffff0104 )
			//	cout << "WARNING, pixel shader " << v.szName << ", using [PS] shader as [PS14] one" << endl;
			WriteShader( f, "dwps11" + v.szName, v.psShader11, &szDeclaration );
			WriteShader( f, "dwps14" + v.szName, v.psShader14, &szDeclaration );
			WriteShader( f, "dwps20" + v.szName, v.psShader20, &szDeclaration );
			WriteShader( f, "dwps20a" + v.szName, v.psShader20a, &szDeclaration );

			WriteRS( f, v.shader, v.szName.c_str(), "sha" );
			WriteRS( f, v.states, v.szName.c_str(), "state" );
			f << "SPShader ps" << v.szName << "( " << k + 1 << " , " << szDeclaration << ", ";
			f << "rssha" << v.szName << ", " << "tsssha" << v.szName << ", ";
			f << "rsstate" << v.szName << ", " << "tssstate" << v.szName << " );" << endl;
		}

		f << endl << "SVShader *vsAllShaders[" << vertexShaders.size() << "] = { ";
		for ( int k = 0; k < vertexShaders.size(); ++k )
		{
			f << "&vs" << vertexShaders[k].szName;
			if ( k != vertexShaders.size() - 1 )
				f << ", ";
		}
		f << " };" << endl;

		f << endl << "SPShader *psAllShaders[" << pixelShaders.size() << "] = { ";
		for ( int k = 0; k < pixelShaders.size(); ++k )
		{
			f << "&ps" << pixelShaders[k].szName;
			if ( k != pixelShaders.size() - 1 )
				f << ", ";
		}
		f << " };" << endl;

		f << endl;
		for ( int k = 0; k < hlslShaders.size(); ++k )
		{
			const SHLSLShaderGroup &g = hlslShaders[k];
			string szTotal = "{";
			for ( int i = 0; i < g.shaders.size(); ++i )
			{
				const SHLSLShader &h = g.shaders[i];
				if ( szTotal[ szTotal.size() - 1 ] != '{' )
					szTotal += ",";
				szTotal += "{" + GetNumber( h.nVertexShaderNumber + 1 ) + "," + GetNumber( h.nPixelShaderNumber + 1 ) + "}";
			}
			szTotal += "}";
			f << endl;
			f << "SHLSLShader hlsl" << g.szName << "[" << g.shaders.size() << "] = " << szTotal << ";" << endl << endl;
		}
		//f << "}" << endl;
		f.close();

		// Check file for changes
		if ( AreFilesDifferent( szTempFileName, szDestFileName ) )
		{
			CopyFile( szTempFileName.c_str(), szDestFileName.c_str(), false );							
		}
		DeleteFile( szTempFileName.c_str() );
	}
}

