#include "stdafx.h"
//#include "Optimizer.h"
#include "parser.h"
#include "Data.h"
#include "output.h"
#include "ShaderAsm.h"

#if SHADERCOMPILER_WITH_D3DX
#include <d3dx9.h>
#endif

#include "port/cdecl.h"

#include <cstdint>
#include <iterator>

struct SCommand
{
	string szCmd;
	vector<string> params;
};

struct SProcedure
{
	string szName;
	bool bExtern;
	vector<string> params;
	vector<string> locals;
	vector<SCommand> cmds;

	void Clear()
	{
		params.clear();
		locals.clear();
		cmds.clear();
	}
	int GetLocalVar( const char *p ) const
	{
		for ( int k = 0; k < locals.size(); ++k )
		{
			if ( locals[k] == p )
				return k;
		}
		return -1;
	}
	int GetParam( const char *p ) const
	{
		for ( int k = 0; k < params.size(); ++k )
		{
			if ( params[k] == p )
				return k;
		}
		return -1;
	}
};

const char* pszCmds[] = {
"add", "dp3", "dp4", "dst", "expp", "lit", "logp", "mad", "max", "min", "mov", "mul", "rcp", 
"rsq", "sge", "slt", "sub", "exp", "frc", "log", "m3x2", "m3x3", "m3x4", "m4x3", "m4x4", 0 };

const char* pszRegs[] = {
	"oD0", "oD1", "oT0", "oT1", "oT2", "oT3", "oT4", "oT5", "oT6", "oT7", "oFog", "oPos", "oPts", 0 };

static bool IsVSRegister( const char *pTest, const char c )
{
	if ( toupper(c) != toupper( pTest[0] ) )
		return false;
	for ( ++pTest; pTest[0]; ++pTest )
	{
		if ( pTest[0] < '0' || pTest[0] > '9' )
			return false;
	}
	return true;
}


static bool IsAddressInstruction( const char *pTest )
{
	// Check if pTest looks "c[a0.x+Number]", parser create the lexem without spaces.
	return true;
	if ( strlen(pTest) < 7 )
		return false;

	if ( toupper( pTest[0] ) != 'C' &&
		   pTest[1] != '[' &&
			 toupper( pTest[2] ) != 'A' &&
			 toupper( pTest[3] ) != '0' &&
			 pTest[4] != '.' &&
			 toupper( pTest[5] ) != 'X' &&
			 toupper( pTest[6] ) != '+'
		 )
		return false;

	return true;
}


struct SProgram
{
	vector<SProcedure> procs;

	int GetProc( const char *p ) const
	{
		for ( int k = 0; k < procs.size(); ++k )
		{
			if ( procs[k].szName == p )
				return k;
		}
		return -1;
	}
	bool IsValidCommand( const char *p ) const
	{
		if ( IsOneOf( pszCmds, p ) )
			return true;
		return GetProc( p ) != -1;
	}
};

class CProgramParser
{
	enum EState
	{
		PROCEDURE,
		_ERROR
	};
	SProgram &prog;
	SProcedure cur;
	EState state;
	string szError;

	bool IsValidVarName( const char *p )
	{
		string sz = StripAfterDot( p );
		const char *psz = sz.c_str();
		return IsOneOf( pszRegs, psz )  || IsVSRegister( psz, 'V' ) || IsVSRegister( psz, 'C' ) || IsVSRegister( psz, 'A' ) ||
			cur.GetLocalVar(psz) != -1 || cur.GetParam(psz) != -1 || IsAddressInstruction( psz );
	}
	void ShitHappened( const string &szInfo )
	{
		szError = szInfo;
		state = _ERROR;
	}
public:
	CProgramParser( SProgram &_p ): prog(_p), state(PROCEDURE) {}
	bool IsError() const { return state == _ERROR; }
	const string& GetError() const { return szError; }
	void Finish()
	{
		if ( !cur.cmds.empty() )
			prog.procs.push_back( cur );
		cur.Clear();
	}
	void AddLine( const char *pszLine, const char *pszLineEnd )
	{
		vector<SLexem> lexems;
		ParseLine( &lexems, pszLine, pszLineEnd );
		if ( lexems.empty() )
			return;
		switch ( state )
		{
			case PROCEDURE:
				if ( lexems[0].szData == "proc" || lexems[0].szData == "func" )
				{
					// if had program output it
					Finish();

					if ( lexems.size() < 2 )
					{
						ShitHappened( "proc name expected" );
						return;
					}
					cur.szName = lexems[1].szData;
					if ( prog.GetProc( cur.szName.c_str() ) != -1 )
					{
						ShitHappened( string("proc ") + cur.szName + " is already defined" );
						return;
					}
					cur.bExtern = lexems[0].szData == "proc";
					for ( int k = 2; k < lexems.size(); ++k )
						cur.params.push_back( lexems[k].szData );
					if ( cur.bExtern && !cur.params.empty() )
					{
						ShitHappened( string("proc ") + cur.szName + " should have no params, only funcs are allowed to have them" );
						return;
					}
				}
				else if ( lexems[0].szData == "locals" )
				{
					for ( int k = 1; k < lexems.size(); ++k )
						cur.locals.push_back( lexems[k].szData );
				}
				else
				{
					SCommand cmd;
					cmd.szCmd = lexems[0].szData;
					if ( !prog.IsValidCommand( cmd.szCmd.c_str() ) )
					{
						ShitHappened( string("unrecognised command ") + cmd.szCmd );
						return;
					}
					for ( int k = 1; k < lexems.size(); ++k )
					{
						if ( !IsValidVarName( lexems[k].szData.c_str() ) )
						{
							ShitHappened( string("invalid command parameter ") + lexems[k].szData );
							return;
						}
						cmd.params.push_back( lexems[k].szData );
					}
					int nCmd = prog.GetProc( cmd.szCmd.c_str() );
					if ( nCmd != -1 )
					{
						if ( prog.procs[nCmd].params.size() != cmd.params.size() )
						{
							ShitHappened( string("invalid number of params of cmd ") + cmd.szCmd );
							return;
						}
					}
					cur.cmds.push_back( cmd );
				}
				break;
		}
	}
};

struct CVSCompiler
{
	const SProgram &prog;
	struct SStkFrame
	{
		const SProcedure *pProc;
		int nIP, nBaseRegister;
		vector<string> args;

		SStkFrame( const SProcedure *_pProc, int _nBaseRegister ): pProc(_pProc), nBaseRegister(_nBaseRegister), nIP(0) {}
	};
public:
	CVSCompiler( const SProgram &_p ): prog(_p) {}
	void GenerateShader( string *pRes, const char *pszProcName )
	{
		string &res = *pRes;
		res = "vs.1.1\n";
		res += "dcl_position v0;\n";
		res += "dcl_normal v1;\n";
		res += "dcl_texcoord0 v3;\n";
		res += "dcl_tangent0 v4;\n";
		res += "dcl_tangent1 v5;\n";
		res += "dcl_texcoord1 v6;\n";
		int n = prog.GetProc( pszProcName );
		if ( n == -1 )
			return;
		list<SStkFrame> stk;
		stk.push_back( SStkFrame( &prog.procs[n], 0 ) );
		while ( !stk.empty() )
		{
			SStkFrame &f = stk.back();
			if ( f.nIP >= f.pProc->cmds.size() )
				stk.pop_back();
			else
			{
				const SCommand &cmd = f.pProc->cmds[f.nIP++];
				int nProc = prog.GetProc( cmd.szCmd.c_str() );
				if ( nProc != -1 )
				{
					SStkFrame nf( &prog.procs[nProc], f.nBaseRegister + f.pProc->locals.size() );
					nf.args = cmd.params;
					ASSERT( nf.args.size() == prog.procs[nProc].params.size() );
					for ( int k = 0; k < nf.args.size(); ++k )
					{
						string sz = StripAfterDot( nf.args[k].c_str() );
						const char *pszParam = sz.c_str();
						int nLocal = f.pProc->GetLocalVar( pszParam ), nParam = f.pProc->GetParam( pszParam );
						if ( nLocal != -1 )
							nf.args[k] = "r" + GetNumber( nLocal + f.nBaseRegister );
						if ( nParam != -1 )
							nf.args[k] = f.args[nParam];
					}
					stk.push_back( nf );
				}
				else
				{
					res += cmd.szCmd + " ";
					for ( int k = 0; k < cmd.params.size(); ++k )
					{
						string szParam, szSuffix, szPrefix;
						szParam = StripAfterDot( cmd.params[k].c_str(), &szSuffix, &szPrefix );
						const char *pszParam = szParam.c_str();
						int nLocal = f.pProc->GetLocalVar( pszParam ), nParam = f.pProc->GetParam( pszParam );
						if ( nLocal != -1 )
							res += szPrefix + "r" + GetNumber( nLocal + f.nBaseRegister ) + szSuffix;
						else if ( nParam != -1 )
							res += szPrefix + f.args[nParam] + szSuffix;
						else
							res += szPrefix + pszParam + szSuffix;
						if ( k != cmd.params.size() - 1 )
							res += ", ";
					}
					res += "\n";
				}
			}
		}
	}
};

enum EState
{
	NONE,
	VSHADER,
	PSHADER,
	PSHADER14,
	PSHADER_RS,
	PSHADER_TSS,
	PS_PROC
};

// Set when a shader fails to assemble, so that nothing is written: a failed
// shader would otherwise go out as a null array and the game would draw with
// whichever fallback GfxRender picks.
static bool bAssemblyFailed = false;

#if SHADERCOMPILER_WITH_D3DX
typedef HRESULT ( WINAPI *TD3DXAssembleShader )( LPCSTR, UINT, CONST D3DXMACRO *, LPD3DXINCLUDE, DWORD, LPD3DXBUFFER *, LPD3DXBUFFER * );

// D3DXAssembleShader from d3dx9_43.dll, loaded rather than linked so that the
// tool still runs where the DirectX runtime is not installed, without the
// check. Null there, and said so once.
static TD3DXAssembleShader GetD3DXAssembleShader()
{
	static const HMODULE hD3DX = LoadLibraryA( "d3dx9_43.dll" );
	static bool bWarned = false;
	if ( !hD3DX && !bWarned )
	{
		cout << "d3dx9_43.dll is not installed: shaders are not checked against D3DX" << endl;
		bWarned = true;
	}
	return hD3DX ? (TD3DXAssembleShader)GetProcAddress( hD3DX, "D3DXAssembleShader" ) : 0;
}

// D3DXAssembleShader's tokens for a shader, without D3DXSHADER_DEBUG, so with
// no comment blocks, which is what ShaderAsm matches. False and the message in
// *pszError if D3DX refuses the shader.
static bool AssembleWithD3DX( TD3DXAssembleShader pAssemble, const string &s, vector<uint32_t> *pRes, string *pszError )
{
	LPD3DXBUFFER pCode = 0, pError = 0;
	const HRESULT hr = pAssemble( s.c_str(), (UINT)s.length(), 0, 0, 0, &pCode, &pError );
	if ( pError )
	{
		*pszError = (const char*)pError->GetBufferPointer();
		pError->Release();
	}
	if ( FAILED( hr ) || !pCode )
	{
		return false;
	}
	const uint32_t *p = (const uint32_t*)pCode->GetBufferPointer();
	pRes->assign( p, p + pCode->GetBufferSize() / 4 );
	pCode->Release();
	return true;
}
#endif

// Assembles one shader with ShaderAsm.
//
// On Windows, where d3dx9_43.dll is installed, the shader is assembled a second
// time with D3DXAssembleShader and any difference fails it. ShaderAsm checks syntax and
// no more, while D3DX validates the program too: a read of a register nothing
// wrote, or a ps.1.1 shader over its instruction count, is an error there and
// silently assembled here. So an edited GfxShaders.txt wants a Windows build to
// regenerate it, or at least to check it once.
static void CompileShader( const string &s, const string &name, const char *pszType, vector<uint32_t> *pRes )
{
	pRes->clear();
	if ( s == "" )
	{
		return;
	}
	string szError;
	if ( !NShaderAsm::Assemble( s, pRes, &szError ) )
	{
		cout << pszType << " shader " << name << " has an error " << szError << endl;
		bAssemblyFailed = true;
		return;
	}
#if SHADERCOMPILER_WITH_D3DX
	const TD3DXAssembleShader pAssemble = GetD3DXAssembleShader();
	if ( !pAssemble )
	{
		return;
	}
	vector<uint32_t> d3dx;
	string szD3DXError;
	if ( !AssembleWithD3DX( pAssemble, s, &d3dx, &szD3DXError ) )
	{
		cout << pszType << " shader " << name << " is refused by D3DX: " << szD3DXError << endl;
		bAssemblyFailed = true;
	}
	else if ( d3dx != *pRes )
	{
		cout << pszType << " shader " << name << " assembles differently with D3DX; ShaderAsm needs fixing" << endl;
		bAssemblyFailed = true;
	}
#endif
}

EState parseState;
string szError, szName;
SStates rs, tss;
int nLine;
char *pszShader, *pszShader14;

typedef std::unordered_map<string, string> CPSProcHash;
CPSProcHash psProcHash;

static char parsedWord[256];
static string PreCompileShader( char *pBuf )
{
	int k;
	string res;
	while ( *pBuf != 0 )
	{
		k = 0;
		while ( ( *pBuf != 0 ) && ( *pBuf != 10 ) && ( *pBuf != 13 ) )
			parsedWord[k++] = *pBuf++;
		parsedWord[k] = '\0';

		CPSProcHash::const_iterator it = psProcHash.find( parsedWord );
		if ( it != psProcHash.end() )
			res += it->second;
		else
			res += parsedWord;

		while ( ( *pBuf == 10 ) || ( *pBuf == 13 ) )
			res += *pBuf++;
	}
	return res;
}

static void FinishState( char *pszFinish )
{
	switch ( parseState )
	{
		case NONE:
			break;
		case VSHADER:
/*			{
				SVShader v;
				v.szName = szName;
				v.szShader = pszShader;
				vertexShaders.push_back( v );
			}*/
			break;
		case PSHADER:
		case PSHADER14:
		case PSHADER_RS:
		case PSHADER_TSS:
			{
				SPShader p;
				p.szName = szName;
				CompileShader( PreCompileShader( pszShader ), szName, "pixel", &p.psShader11 );
				if ( pszShader14 )
					CompileShader( PreCompileShader( pszShader14 ), szName, "pixel14", &p.psShader14 );
				p.states = rs;
				p.shader = tss;
				pixelShaders.push_back( p );
			}
			break;
		case PS_PROC:
			psProcHash[szName] = pszShader;
			break;
		default:
			ASSERT( 0 );
			break;
	}
}

static void AddRS( SStates *pRes, const char *pszLine )
{
	vector<string> res;
	SplitString( pszLine, &res );
	if ( res.size() == 2 )
	{
		SRS r;
		r.sz1 = res[0];
		r.sz2 = res[1];
		pRes->rs.push_back( r );
	}
	else if ( res.size() == 3 )
	{
		STSS r;
		r.sz1 = res[0];
		r.sz2 = res[1];
		r.sz3 = res[2];
		pRes->tss.push_back( r );
	}
	else
	{
		cout << "ignoring state in line " << nLine << endl;
	}
}

static void ParseFile( char *pszFile )
{
	char *pszParse = pszFile, *pNextLine;
	int nNextLine;
	nLine = 1;
	parseState = NONE;
	SProgram prog; 
	CProgramParser pparser( prog );
	for ( ; pszParse[0]; pszParse = pNextLine, nLine = nNextLine )
	{
		nNextLine = nLine;
		pNextLine = GetNextLine( pszParse, &nNextLine );
		if ( strlen( pszParse ) > 5 )
		{
			if ( strncmp( pszParse, "[VS]", 4 ) == 0 )
			{
				pszParse[0] = 0;
				FinishState( pszParse );
				pNextLine[-1] = 0;
				//szName = Filter( pszParse + 4 );
				//pszShader = pNextLine;
				parseState = VSHADER;
				continue;
			}
			if ( strncmp( pszParse, "[PS_PROC]", 9 ) == 0 )
			{
				pszParse[0] = 0;
				FinishState( pszParse );
				szName = Filter( pszParse + 9 );
				rs.Clear();
				tss.Clear();
				pNextLine[-1] = 0;
				pszShader = pNextLine;
				pszShader14 = 0;
				parseState = PS_PROC;
			}
			if ( strncmp( pszParse, "[PS]", 4 ) == 0 )
			{
				pszParse[0] = 0;
				FinishState( pszParse );
				szName = Filter( pszParse + 4 );
				rs.Clear();
				tss.Clear();
				pNextLine[-1] = 0;
				pszShader = pNextLine;
				pszShader14 = 0;
				parseState = PSHADER;
				continue;
			}
			if ( strncmp( pszParse, "[PS14]", 6 ) == 0 )
			{
				pszParse[0] = 0;
				if ( parseState != PSHADER )
				{
					szError = "[PS14] should follow [PS]";
					return;
				}
				pNextLine[-1] = 0;
				pszShader14 = pNextLine;
				parseState = PSHADER14;
				continue;
			}
			if ( strncmp( pszParse, "[RS]", 4 ) == 0 )
			{
				pszParse[0] = 0;
				if ( parseState != PSHADER && parseState != PSHADER14 )
				{
					szError = "[RS] should follow [PS] or [PS14]";
					return;
				}
				parseState = PSHADER_RS;
				continue;
			}
			if ( strncmp( pszParse, "[TSS]", 5 ) == 0 )
			{
				pszParse[0] = 0;
				if ( parseState != PSHADER && parseState != PSHADER_RS )
				{
					szError = "[TSS] should follow either [RS] or [PS]";
					return;
				}
				parseState = PSHADER_TSS;
				continue;
			}
			// [HLSL] sections compiled HLSL through D3DXCompileShaderFromFile.
			// The one there was had no consumer and is gone, and so is the
			// support. Refuse one rather than read its lines as part of the
			// section before it.
			if ( strncmp( pszParse, "[HLSL]", 6 ) == 0 )
			{
				szError = "[HLSL] sections are no longer supported";
				return;
			}
		}
		if ( pszParse[0] == ';' || (pszParse[0] == '/' && pszParse[1] == '/' ) )
			continue;
		switch ( parseState )
		{
			case NONE:
				break;
			case VSHADER:
				pparser.AddLine( pszParse, pNextLine );
				if ( pparser.IsError() )
				{
					szError = pparser.GetError();
					return;
				}
				break;
			case PSHADER:
				break;
			case PSHADER_RS:
				if ( pNextLine[0] )
					pNextLine[-1] = 0;
				AddRS( &rs, pszParse );
				break;
			case PSHADER_TSS:
				if ( pNextLine[0] )
					pNextLine[-1] = 0;
				AddRS( &tss, pszParse );
				break;
			case PS_PROC:
				break;
		}
	}
	FinishState( pszParse );
	pparser.Finish();
	// generate vertex programs
	for ( int k = 0; k < prog.procs.size(); ++k )
	{
		if ( prog.procs[k].bExtern )
		{
			SVShader v;
			v.szName = prog.procs[k].szName;
			string szShader = pszShader;
			CVSCompiler vsc( prog );
			vsc.GenerateShader( &szShader, prog.procs[k].szName.c_str() );
			// has no effect on nVidia & ATi; looks like drivers already do this
			//NVSOptimize::OptimizeVertexShader( &v.szShader, v.szShader );
			
			CompileShader( szShader.c_str(), v.szName, "vertex", &v.vsShader11 );

			vertexShaders.push_back( v );
#if BOOST_OS_WINDOWS
			OutputDebugString( v.szName.c_str() );
			OutputDebugString( "\n" );
			OutputDebugString( szShader.c_str() );
#endif
		}
	}
}

int PORT_CDECL main( int argc, char* argv[] )
{
	if ( argc != 3 )
	{
		cout << "Usage: ShaderCompiler srcFile dstFile\n";
		return 2;
	}
	cout << "Compiling " << argv[1] << " into " << argv[2] << endl;
	// the parser works in place on a zero terminated copy of the file
	std::ifstream file( argv[1], std::ios::binary );
	if ( !file )
	{
		cout << "failed to open " << argv[1] << endl;
		return 1;
	}
	vector<char> source( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
	source.push_back( 0 );
	ParseFile( source.data() );
	if ( szError != "" )
	{
		cout << argv[1] << " has an error (0): error X5328: "  << "at line " << nLine << ": " << szError << endl << endl;
		return 1;
	}
	if ( bAssemblyFailed )
	{
		cout << argv[2] << " not written" << endl;
		return 1;
	}
	WriteResult( argv[2] );
	return 0;
}



