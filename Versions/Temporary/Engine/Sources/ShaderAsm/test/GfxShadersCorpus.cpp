// NShaderAsm::Assemble against D3DXAssembleShader, over every shader the game
// has. GfxShaders.corpus holds each one's assembly text and the tokens D3DX
// made of it; see the comment at its top for how it was produced.

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ShaderAsm.h"

#include <gtest/gtest.h>

namespace {

struct SEntry
{
	std::string szName;
	std::string szSource;
	std::vector<uint32_t> expected;
};

std::vector<SEntry> ReadCorpus( const std::string &szPath )
{
	std::ifstream file( szPath, std::ios::binary );
	std::vector<SEntry> entries;
	enum { OUTSIDE, SOURCE, EXPECT } state = OUTSIDE;
	std::string szLine;
	while ( std::getline( file, szLine ) )
	{
		if ( szLine.compare( 0, 11, "@@@ shader " ) == 0 )
		{
			entries.push_back( SEntry() );
			entries.back().szName = szLine.substr( 11 );
			state = SOURCE;
		}
		else if ( szLine == "@@@ expect" )
		{
			state = EXPECT;
		}
		else if ( szLine == "@@@ end" )
		{
			state = OUTSIDE;
		}
		else if ( state == SOURCE )
		{
			entries.back().szSource += szLine + "\n";
		}
		else if ( state == EXPECT )
		{
			std::istringstream tokens( szLine );
			uint32_t dw;
			while ( tokens >> std::hex >> dw )
			{
				entries.back().expected.push_back( dw );
			}
		}
	}
	return entries;
}

std::string Hex( uint32_t dw )
{
	char sz[16];
	snprintf( sz, sizeof( sz ), "%08x", dw );
	return sz;
}

TEST( ShaderAsm, MatchesD3DXOnEveryGameShader )
{
	const std::vector<SEntry> entries = ReadCorpus( GFXSHADERS_CORPUS );
	ASSERT_EQ( entries.size(), 116u ) << "reading " << GFXSHADERS_CORPUS;

	int nMatched = 0;
	for ( const SEntry &e : entries )
	{
		std::vector<uint32_t> tokens;
		std::string szError;
		if ( !NShaderAsm::Assemble( e.szSource, &tokens, &szError ) )
		{
			ADD_FAILURE() << e.szName << ": " << szError;
			continue;
		}
		if ( tokens == e.expected )
		{
			++nMatched;
			continue;
		}
		// the first token that differs, and where it falls
		size_t n = 0;
		while ( n < tokens.size() && n < e.expected.size() && tokens[n] == e.expected[n] )
		{
			++n;
		}
		ADD_FAILURE() << e.szName << ": differs at token " << n << " of " << e.expected.size() << ", got "
			<< ( n < tokens.size() ? Hex( tokens[n] ) : "end" ) << ", D3DX has "
			<< ( n < e.expected.size() ? Hex( e.expected[n] ) : "end" );
	}
	EXPECT_EQ( nMatched, (int)entries.size() );
}

TEST( ShaderAsm, RejectsWhatItDoesNotCover )
{
	std::vector<uint32_t> tokens;
	std::string szError;
	EXPECT_FALSE( NShaderAsm::Assemble( "vs.2.0\nmov oPos, v0\n", &tokens, &szError ) );
	EXPECT_FALSE( NShaderAsm::Assemble( "vs.1.1\nmov r0, c[a0.x + 4]\n", &tokens, &szError ) );
	EXPECT_NE( szError.find( "relative addressing" ), std::string::npos ) << szError;
	EXPECT_FALSE( NShaderAsm::Assemble( "ps.2.0\nfrob r0, r1\n", &tokens, &szError ) );
	EXPECT_NE( szError.find( "line 2" ), std::string::npos ) << szError;
	EXPECT_FALSE( NShaderAsm::Assemble( "ps.1.1\ntexld r0, t0\n", &tokens, &szError ) );
	EXPECT_TRUE( tokens.empty() );
}

}
