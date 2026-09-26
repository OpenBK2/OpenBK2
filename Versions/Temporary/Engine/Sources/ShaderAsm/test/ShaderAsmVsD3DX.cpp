// NShaderAsm::Assemble against D3DXAssembleShader itself, on generated shaders.
//
// GfxShadersCorpus_test proves the assembler on the game's own shaders, which
// only exercise what those shaders happen to use. This varies one thing at a
// time around them, per shader model: every mnemonic the assembler knows, each
// with a spread of write masks, then each source slot with a spread of
// swizzles and modifiers, then the result modifiers, co-issue and the
// declarations. Each case is a whole program, a prologue that declares and
// initialises what the case reads, the case, and an epilogue that writes the
// output D3DX insists on, and both assemblers get the same text.
//
// D3DX validates as it assembles and refuses much that is well formed, reads of
// uninitialised components or a sampler used twice in ps.1.4, and so on. Those
// cases say nothing about the encoding and are skipped. A case D3DX accepts
// and NShaderAsm rejects is a failure: it is something the game could write
// and the assembler could not build.
//
// Windows only, as it needs D3DX. It loads d3dx9_43.dll at run time rather than
// linking it, and skips where the DirectX runtime that carries it is not
// installed, which includes the CI runners: the SDK there is only its headers
// and libraries.

#include <windows.h>
#include <d3dx9.h>

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "ShaderAsm.h"

#include <gtest/gtest.h>

namespace {

struct SProfileCases
{
	const char *pszName;
	const char *pszPrologue;
	const char *pszEpilogue;
	std::vector<const char *> binary;	// takes a destination and two sources
	std::vector<const char *> unary;
	std::vector<const char *> ternary;
	std::vector<const char *> dsts;
	std::vector<const char *> srcs;
	std::vector<const char *> resultMods;
	// instruction sequences, or whole programs when they start with a version
	std::vector<const char *> snippets;
	// the operands a mnemonic's variations start from, where the usual r2 (or
	// the first dst), r0, r1, c0 would make D3DX refuse all of them: the
	// mnemonic, then its destination and sources
	std::vector<std::vector<const char *>> defaults;
};

const SProfileCases profiles[] =
{
	{
		"vs.1.1",
		"vs.1.1\ndcl_position v0\ndcl_normal v1\ndcl_texcoord0 v2\ndcl_tangent1 v5\nmov r0, v0\nmov r1, v1\n",
		"mov oPos, r0\n",
		{ "add", "sub", "mul", "dp3", "dp4", "min", "max", "slt", "sge", "dst", "m4x4", "m4x3", "m3x4", "m3x3", "m3x2" },
		{ "mov", "rcp", "rsq", "exp", "log", "expp", "logp", "lit", "frc" },
		{ "mad" },
		{ "r2", "r2.x", "r2.y", "r2.xy", "r2.xz", "r2.yzw", "r2.w", "oD0", "oD1.xyz", "oT0.xy", "oT7", "oFog", "oPts", "oPos" },
		{ "r0", "r1.x", "r1.w", "-r0", "r0.yzxw", "-r1.xyz", "r0.zw", "c5", "c95.y", "-c0.wzyx", "v0", "v1.x", "-v2.yx", "v5" },
		{},
		{ "nop", "mov r2, c0\nmul oD0, r2.x, v1" },
		{
			{ "m4x4", "r2", "r0", "c0" },
			{ "m4x3", "r2", "r0", "c0" },
			{ "m3x4", "r2", "r0", "c0" },
			{ "m3x3", "r2", "r0", "c0" },
			{ "m3x2", "r2", "r0", "c0" },
		},
	},
	{
		"ps.1.1",
		"ps.1.1\ntex t0\ntex t1\ntexcoord t2\nmov r1, v0\nmov r0, c1\n",
		"mov r0, r1\n",
		{ "add", "sub", "mul", "dp3" },
		{ "mov" },
		{ "mad", "lrp", "cnd" },
		{ "r1", "r1.rgb", "r1.a", "t0", "t1.rgb" },
		{ "r1", "r1.a", "-r1", "1-r1", "r1_bx2", "-r1_bx2", "r1_bias", "-r1_bias", "t0", "t1.a", "-t2.b", "v0", "v1_bx2", "c3", "c7.a", "-c0", "1-c2.a", "t0.a" },
		{ "_x2", "_x4", "_d2", "_sat", "_x2_sat", "_x4_sat", "_d2_sat" },
		{
			"mul r1.rgb, t0, v0\n+add r1.a, t1, c0",
			"mul_x4_sat r1.rgb, t0, v0\n+add_x4_sat r1.a, t1, -t2.b",
			"nop",
			"mov r0.a, t0.b\ncnd r1, r0.a, t0, t1",
			"ps.1.1\ntex t0\ntexm3x2pad t1, t0\ntexm3x2tex t2, t0\nmov r0, t2",
			"ps.1.1\ntex t0\ntexm3x2pad t1, t0_bx2\ntexm3x2tex t2, t0_bx2\nmov r0, t2",
			"ps.1.1\ntex t0\ntexm3x3pad t1, t0\ntexm3x3pad t2, t0\ntexm3x3tex t3, t0\nmov r0, t3",
			"ps.1.1\ntex t0\ntexm3x3pad t1, t0_bx2\ntexm3x3pad t2, t0_bx2\ntexm3x3vspec t3, t0_bx2\nmov r0, t3",
			"ps.1.1\ntex t0\ntexm3x3pad t1, t0\ntexm3x3pad t2, t0\ntexm3x3spec t3, t0, c0\nmov r0, t3",
			"ps.1.1\ntexcoord t0\ntexcoord t1\nmul r0.rgb, t0, t1\n+mov r0.a, t1.b",
		},
		{
			{ "cnd", "r1", "r0.a", "r1", "c0" },
		},
	},
	{
		"ps.1.4",
		"ps.1.4\ntexld r0, t0\ntexld r1, t1\ntexcrd r2.rgb, t2\nmov r2.a, c0\n",
		"mov r0, r1\n",
		{ "add", "sub", "mul", "dp3", "dp4" },
		{ "mov" },
		{ "mad", "lrp", "cnd", "cmp" },
		{ "r1", "r1.rgb", "r1.a", "r1.r", "r1.rg", "r1.gba", "r3", "r5.b" },
		{ "r0", "r1.a", "r2.b", "r0.r", "r0.g", "-r0", "1-r1", "r0_bx2", "-r1_bx2", "r1_bias", "-r1_bias", "r0_x2", "-r0_x2", "v0", "v1.a", "c0", "-c7.b" },
		{ "_x2", "_x4", "_x8", "_d2", "_d4", "_d8", "_sat", "_x4_sat", "_d8_sat" },
		{
			"texcrd r3.rgb, t3.xyw",
			"texcrd r3.rgb, t3.xyz",
			"texld r3, t3",
			"texld r3, t3_dz",
			"texld r3, t3.xyw",
			"texld r3, t3_dw.xyw",
			"texld r3, t3.xyz\nphase\ntexld r4, r3\nmov r1, r4\nmov r0, r1",
			"texld r3, t3\nphase\ntexld r4, r3_dz\nmov r1, r4\nmov r0, r1",
			"mul r1.rgb, r0, v0\n+add r1.a, r0, c0",
			"dp3_x4_sat r1, r0, v0",
			"nop",
		},
	},
	{
		"ps.2.0",
		"ps.2.0\ndcl t0\ndcl t1.xy\ndcl v0\ndcl_2d s0\ndcl_cube s1\nmov r0, t0\nmov r1, v0\n",
		"mov oC0, r0\n",
		{ "add", "sub", "mul", "dp3", "dp4", "min", "max", "pow", "m4x4", "m4x3", "m3x4", "m3x3", "m3x2", "crs" },
		{ "mov", "rcp", "rsq", "exp", "log", "frc", "abs", "nrm" },
		{ "mad", "lrp", "cmp", "dp2add" },
		{ "r2", "r2.x", "r2.y", "r2.xy", "r2.xyz", "r2.xz", "r2.yw", "r2.w", "r31", "oC0" },
		{ "r0", "r1.x", "r1.w", "-r0", "r0.yzxw", "-r1.xyz", "r0.zw", "r0.wzyx", "c5", "c31.y", "-c0.wzyx", "t0", "t1.xy", "v0.a", "-v0" },
		{ "_sat", "_pp", "_sat_pp" },
		{
			"texld r2, t0, s0",
			"texld r2, t1, s0",
			"texldp r2, t0, s0",
			"texldb r2, t0, s0",
			"texld r2, r0, s1",
			"texld_pp r2, t0, s0",
			"dcl_volume s2\ntexld r2, t0, s2",
			"dcl t2.xyz\ntexld r2, t2, s1",
			"dcl t3.x\nmov r2, t3.x",
			"dcl v1.xyz\nmov r2, v1.xyzz",
			"mov oDepth, r0.x",
			"def c4, 1, 0.5, -2, 0.25\nmul r2, r0, c4",
			"def c4, 3.40282347e+38, 1.17549435e-38, -0, 0\nmul r2, r0, c4",
			"nop",
		},
		{
			{ "m4x4", "r2", "r0", "c0" },
			{ "m3x4", "r2", "r0", "c0" },
			{ "m4x3", "r2.xyz", "r0", "c0" },
			{ "m3x3", "r2.xyz", "r0", "c0" },
			{ "m3x2", "r2.xy", "r0", "c0" },
			{ "pow", "r2", "r0.x", "r1.y" },
			{ "rcp", "r2", "r0.x" },
			{ "rsq", "r2", "r0.w" },
			{ "exp", "r2", "r0.y" },
			{ "log", "r2", "r0.z" },
		},
	},
};

typedef HRESULT ( WINAPI *TD3DXAssembleShader )( LPCSTR, UINT, CONST D3DXMACRO *, LPD3DXINCLUDE, DWORD, LPD3DXBUFFER *, LPD3DXBUFFER * );

// D3DXAssembleShader from d3dx9_43.dll, the June 2010 SDK's, or null where it
// is not installed
TD3DXAssembleShader GetD3DXAssembleShader()
{
	static const HMODULE hD3DX = LoadLibraryA( "d3dx9_43.dll" );
	return hD3DX ? (TD3DXAssembleShader)GetProcAddress( hD3DX, "D3DXAssembleShader" ) : 0;
}

bool AssembleD3DX( const std::string &sz, std::vector<uint32_t> *pTokens )
{
	LPD3DXBUFFER pCode = 0, pError = 0;
	const HRESULT hr = GetD3DXAssembleShader()( sz.c_str(), (UINT)sz.size(), 0, 0, 0, &pCode, &pError );
	if ( pError )
	{
		pError->Release();
	}
	if ( FAILED( hr ) || !pCode )
	{
		return false;
	}
	const uint32_t *p = (const uint32_t *)pCode->GetBufferPointer();
	pTokens->assign( p, p + pCode->GetBufferSize() / 4 );
	pCode->Release();
	return true;
}

// A case on one line, for the failure messages
std::string OneLine( const std::string &sz )
{
	std::string szRes;
	for ( char c : sz )
	{
		szRes += c == '\n' ? std::string( " | " ) : std::string( 1, c );
	}
	return szRes;
}

std::string Hex( const std::vector<uint32_t> &tokens )
{
	std::string sz;
	for ( uint32_t dw : tokens )
	{
		char szToken[12];
		snprintf( szToken, sizeof( szToken ), "%08x ", dw );
		sz += szToken;
	}
	return sz;
}

// The operand lists for one mnemonic with nOperands sources: vary the
// destination with default sources, then each source slot on its own
std::vector<std::string> Vary( const SProfileCases &p, const std::string &szMnemonic, int nSources )
{
	std::vector<std::string> lines;
	const std::string szDefaultSrc[3] = { "r0", "r1", "c0" };
	std::string szDefaultDst = p.dsts[0];
	std::vector<std::string> srcs( szDefaultSrc, szDefaultSrc + nSources );
	for ( const std::vector<const char *> &def : p.defaults )
	{
		if ( szMnemonic == def[0] )
		{
			szDefaultDst = def[1];
			srcs.assign( def.begin() + 2, def.end() );
		}
	}
	auto line = [&]( const std::string &szMod, const std::string &szDst, const std::vector<std::string> &srcs )
	{
		std::string sz = szMnemonic + szMod + " " + szDst;
		for ( const std::string &s : srcs )
		{
			sz += ", " + s;
		}
		return sz;
	};
	for ( const char *pszDst : p.dsts )
	{
		lines.push_back( line( "", pszDst, srcs ) );
	}
	for ( int i = 0; i < nSources; ++i )
	{
		for ( const char *pszSrc : p.srcs )
		{
			std::vector<std::string> varied = srcs;
			varied[i] = pszSrc;
			lines.push_back( line( "", szDefaultDst, varied ) );
		}
	}
	for ( const char *pszMod : p.resultMods )
	{
		lines.push_back( line( pszMod, szDefaultDst, srcs ) );
	}
	return lines;
}

TEST( ShaderAsm, MatchesD3DXOnGeneratedShaders )
{
	if ( !GetD3DXAssembleShader() )
	{
		GTEST_SKIP() << "d3dx9_43.dll is not installed, so there is no D3DX to compare with";
	}
	int nCompared = 0, nSkipped = 0, nFailed = 0;
	for ( const SProfileCases &p : profiles )
	{
		std::vector<std::string> cases;
		for ( const char *psz : p.unary )
		{
			const std::vector<std::string> v = Vary( p, psz, 1 );
			cases.insert( cases.end(), v.begin(), v.end() );
		}
		for ( const char *psz : p.binary )
		{
			const std::vector<std::string> v = Vary( p, psz, 2 );
			cases.insert( cases.end(), v.begin(), v.end() );
		}
		for ( const char *psz : p.ternary )
		{
			const std::vector<std::string> v = Vary( p, psz, 3 );
			cases.insert( cases.end(), v.begin(), v.end() );
		}
		cases.insert( cases.end(), p.snippets.begin(), p.snippets.end() );

		int nProfileCompared = 0;
		// compared cases per mnemonic, keyed by the first word of the case
		std::map<std::string, int> comparedPerMnemonic;
		for ( const std::vector<const char *> *pList : { &p.unary, &p.binary, &p.ternary } )
		{
			for ( const char *psz : *pList )
			{
				comparedPerMnemonic[psz] = 0;
			}
		}
		for ( const std::string &szCase : cases )
		{
			const bool bWhole = szCase.compare( 0, 3, std::string( p.pszName, 3 ) ) == 0;
			const std::string szShader = bWhole ? szCase + "\n" : std::string( p.pszPrologue ) + szCase + "\n" + p.pszEpilogue;
			std::vector<uint32_t> expected;
			if ( !AssembleD3DX( szShader, &expected ) )
			{
				++nSkipped;
				continue;
			}
			++nCompared;
			++nProfileCompared;
			++comparedPerMnemonic[szCase.substr( 0, szCase.find_first_of( " _\n" ) )];
			std::vector<uint32_t> tokens;
			std::string szError;
			if ( !NShaderAsm::Assemble( szShader, &tokens, &szError ) )
			{
				++nFailed;
				ADD_FAILURE() << p.pszName << " \"" << OneLine( szCase ) << "\": D3DX assembles it, ShaderAsm says " << szError;
			}
			else if ( tokens != expected )
			{
				++nFailed;
				ADD_FAILURE() << p.pszName << " \"" << OneLine( szCase ) << "\"\n  ShaderAsm " << Hex( tokens ) << "\n  D3DX      " << Hex( expected );
			}
		}
		// a profile where D3DX accepted almost nothing would mean the prologue is wrong
		EXPECT_GT( nProfileCompared, 20 ) << p.pszName;
		// a mnemonic D3DX refused in every variation was never compared at all,
		// which means the variations need another prologue or operand
		for ( const auto &[szMnemonic, nCompared] : comparedPerMnemonic )
		{
			EXPECT_GT( nCompared, 0 ) << p.pszName << " " << szMnemonic << " was never compared";
			printf( "%s %s: %d\n", p.pszName, szMnemonic.c_str(), nCompared );
		}
	}
	printf( "compared %d cases with D3DX, %d skipped as invalid to D3DX, %d failed\n", nCompared, nSkipped, nFailed );
}

}
