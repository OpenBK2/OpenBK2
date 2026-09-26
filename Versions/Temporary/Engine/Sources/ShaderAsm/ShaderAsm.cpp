#include "ShaderAsm.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iterator>

// The token layout is Direct3D 9's, as d3d9types.h describes it:
//
// - version: 0xFFFF0000 for a pixel shader, 0xFFFE0000 for a vertex shader,
//   ORed with major << 8 | minor.
// - instruction: opcode in bits 0-15, opcode-specific control in 16-23, from
//   shader model 2 on the number of parameter tokens that follow in 24-27, and
//   the co-issue flag of ps.1.x in bit 30. Bit 31 is clear.
// - parameter: bit 31 set, register number in 0-10, register type split over
//   28-30 (low three bits) and 11-12 (high two).
//   - destination: write mask in 16-19, result modifier in 20-23 (_sat,
//     _pp), result shift in 24-27 (_x2 is 1, _d2 is 15).
//   - source: swizzle in 16-23, two bits per component, and source modifier in
//     24-27.
// - end: 0x0000FFFF.

namespace NShaderAsm
{
namespace
{

enum EProfile
{
	VS11 = 1,
	PS11 = 2,
	PS14 = 4,
	PS20 = 8,
	PS1X = PS11 | PS14,
	PS = PS1X | PS20,
	ALL = VS11 | PS,
};

// D3DSHADER_PARAM_REGISTER_TYPE. REG_ADDR (vertex shaders) and REG_TEXTURE
// (pixel shaders) share a number.
enum ERegType
{
	REG_TEMP = 0,
	REG_INPUT = 1,
	REG_CONST = 2,
	REG_ADDR = 3,
	REG_TEXTURE = 3,
	REG_RASTOUT = 4,
	REG_ATTROUT = 5,
	REG_TEXCRDOUT = 6,
	REG_COLOROUT = 8,
	REG_DEPTHOUT = 9,
	REG_SAMPLER = 10,
};

// D3DSHADER_PARAM_SRCMOD_TYPE
enum ESrcMod
{
	SRCMOD_NONE = 0,
	SRCMOD_NEG = 1,
	SRCMOD_BIAS = 2,
	SRCMOD_BIASNEG = 3,
	SRCMOD_SIGN = 4,
	SRCMOD_SIGNNEG = 5,
	SRCMOD_COMP = 6,
	SRCMOD_X2 = 7,
	SRCMOD_X2NEG = 8,
	SRCMOD_DZ = 9,
	SRCMOD_DW = 10,
};

const uint32_t OPCODE_ADD = 2;
const uint32_t OPCODE_SUB = 3;
const uint32_t OPCODE_DCL = 31;
const uint32_t OPCODE_DEF = 81;
const uint32_t OPCODE_PHASE = 0xFFFD;
const uint32_t TOKEN_END = 0x0000FFFF;
const uint32_t PARAM_BIT = 0x80000000;
const uint32_t COISSUE_BIT = 0x40000000;
const uint32_t RESULT_SAT = 1 << 20;
const uint32_t RESULT_PP = 2 << 20;
const uint32_t RESULT_CENTROID = 4 << 20;
const uint32_t TEXLD_PROJECT = 1 << 16;
const uint32_t TEXLD_BIAS = 2 << 16;
const uint32_t SWIZZLE_XYZW = 0xE4;
const uint32_t SWIZZLE_WWWW = 0xFF;

// One mnemonic in one set of profiles. A mnemonic whose operands differ between
// shader models, texld in ps.1.4 and ps.2.0 for one, has an entry per form.
struct SInstruction
{
	const char *pszName;
	uint32_t nOpcode;
	int nProfiles;
	int nDst;
	int nSrc;
	uint32_t dwControl;
	// What D3DX fills in when a vs.1.1 instruction leaves it out. The matrix
	// instructions that produce fewer than four components write only those,
	// and the scalar instructions read .w of a source given without a swizzle.
	uint32_t dwVSDefaultMask;
	bool bVSScalarSource;
};

const SInstruction instructions[] =
{
	{ "nop", 0, ALL, 0, 0, 0, 0, false },
	{ "mov", 1, ALL, 1, 1, 0, 0, false },
	{ "add", 2, ALL, 1, 2, 0, 0, false },
	{ "sub", 3, ALL, 1, 2, 0, 0, false },
	{ "mad", 4, ALL, 1, 3, 0, 0, false },
	{ "mul", 5, ALL, 1, 2, 0, 0, false },
	{ "rcp", 6, VS11 | PS20, 1, 1, 0, 0, true },
	{ "rsq", 7, VS11 | PS20, 1, 1, 0, 0, true },
	{ "dp3", 8, ALL, 1, 2, 0, 0, false },
	{ "dp4", 9, VS11 | PS14 | PS20, 1, 2, 0, 0, false },
	{ "min", 10, VS11 | PS20, 1, 2, 0, 0, false },
	{ "max", 11, VS11 | PS20, 1, 2, 0, 0, false },
	{ "slt", 12, VS11, 1, 2, 0, 0, false },
	{ "sge", 13, VS11, 1, 2, 0, 0, false },
	{ "exp", 14, VS11 | PS20, 1, 1, 0, 0, true },
	{ "log", 15, VS11 | PS20, 1, 1, 0, 0, true },
	{ "lit", 16, VS11, 1, 1, 0, 0, false },
	{ "dst", 17, VS11, 1, 2, 0, 0, false },
	{ "lrp", 18, PS, 1, 3, 0, 0, false },
	{ "frc", 19, VS11 | PS20, 1, 1, 0, 0, false },
	{ "m4x4", 20, VS11 | PS20, 1, 2, 0, 0, false },
	{ "m4x3", 21, VS11 | PS20, 1, 2, 0, 0x7, false },
	{ "m3x4", 22, VS11 | PS20, 1, 2, 0, 0, false },
	{ "m3x3", 23, VS11 | PS20, 1, 2, 0, 0x7, false },
	{ "m3x2", 24, VS11 | PS20, 1, 2, 0, 0x3, false },
	{ "pow", 32, PS20, 1, 2, 0, 0, false },
	{ "crs", 33, PS20, 1, 2, 0, 0, false },
	{ "abs", 35, PS20, 1, 1, 0, 0, false },
	{ "nrm", 36, PS20, 1, 1, 0, 0, false },
	{ "expp", 78, VS11, 1, 1, 0, 0, true },
	{ "logp", 79, VS11, 1, 1, 0, 0, true },
	{ "cnd", 80, PS1X, 1, 3, 0, 0, false },
	{ "cmp", 88, PS14 | PS20, 1, 3, 0, 0, false },
	{ "dp2add", 90, PS20, 1, 3, 0, 0, false },
	// ps.1.1 texture addressing: the destination is also the texture stage
	{ "texcoord", 64, PS11, 1, 0, 0, 0, false },
	{ "tex", 66, PS11, 1, 0, 0, 0, false },
	{ "texm3x2pad", 71, PS11, 1, 1, 0, 0, false },
	{ "texm3x2tex", 72, PS11, 1, 1, 0, 0, false },
	{ "texm3x3pad", 73, PS11, 1, 1, 0, 0, false },
	{ "texm3x3tex", 74, PS11, 1, 1, 0, 0, false },
	{ "texm3x3spec", 76, PS11, 1, 2, 0, 0, false },
	{ "texm3x3vspec", 77, PS11, 1, 1, 0, 0, false },
	// ps.1.4 spells texcoord and tex texcrd and texld, and gives them a source
	{ "texcrd", 64, PS14, 1, 1, 0, 0, false },
	{ "texld", 66, PS14, 1, 1, 0, 0, false },
	// ps.2.0 sampling takes the coordinates and a sampler
	{ "texld", 66, PS20, 1, 2, 0, 0, false },
	{ "texldp", 66, PS20, 1, 2, TEXLD_PROJECT, 0, false },
	{ "texldb", 66, PS20, 1, 2, TEXLD_BIAS, 0, false },
};

struct SProfile
{
	int nProfile;
	uint32_t dwVersion;
};

// The first line of a shader: vs.1.1, ps.1.4, ps_2_0 and so on
bool ParseVersion( std::string_view line, SProfile *pProfile )
{
	std::string sz;
	for ( char c : line )
	{
		if ( !isspace( (unsigned char)c ) )
		{
			sz += (char)tolower( (unsigned char)c );
		}
	}
	if ( sz.size() != 6 || ( sz[2] != '.' && sz[2] != '_' ) || sz[4] != sz[2] )
	{
		return false;
	}
	const std::string szShort = sz.substr( 0, 2 ) + sz.substr( 3, 1 ) + sz.substr( 5, 1 );
	if ( szShort == "vs11" )
	{
		*pProfile = { VS11, 0xFFFE0101 };
	}
	else if ( szShort == "ps11" )
	{
		*pProfile = { PS11, 0xFFFF0101 };
	}
	else if ( szShort == "ps14" )
	{
		*pProfile = { PS14, 0xFFFF0104 };
	}
	else if ( szShort == "ps20" )
	{
		*pProfile = { PS20, 0xFFFF0200 };
	}
	else
	{
		return false;
	}
	return true;
}

uint32_t RegisterBits( int nType, int nNumber )
{
	return PARAM_BIT | ( uint32_t( nType & 7 ) << 28 ) | ( uint32_t( nType & 0x18 ) << 8 ) | uint32_t( nNumber );
}

// x, y, z, w and their colour spellings r, g, b, a
int ComponentIndex( char c )
{
	switch ( c )
	{
	case 'x': case 'r': return 0;
	case 'y': case 'g': return 1;
	case 'z': case 'b': return 2;
	case 'w': case 'a': return 3;
	default: return -1;
	}
}

class CAssembler
{
	SProfile profile;
	std::vector<uint32_t> &tokens;
	std::string szError;
	int nLine;

	bool Fail( const std::string &sz )
	{
		if ( szError.empty() )
		{
			szError = "line " + std::to_string( nLine ) + ": " + sz;
		}
		return false;
	}

	bool IsPixel() const { return profile.nProfile != VS11; }

	// A register name and number, without modifiers or selector
	bool ParseRegister( const std::string &sz, int *pnType, int *pnNumber )
	{
		struct SNamed { const char *pszName; int nType; int nNumber; int nProfiles; };
		static const SNamed named[] =
		{
			{ "opos", REG_RASTOUT, 0, VS11 },
			{ "ofog", REG_RASTOUT, 1, VS11 },
			{ "opts", REG_RASTOUT, 2, VS11 },
			{ "odepth", REG_DEPTHOUT, 0, PS20 },
		};
		for ( const SNamed &n : named )
		{
			if ( sz == n.pszName )
			{
				if ( !( n.nProfiles & profile.nProfile ) )
				{
					return Fail( "register " + sz + " does not exist in this shader model" );
				}
				*pnType = n.nType;
				*pnNumber = n.nNumber;
				return true;
			}
		}

		struct SNumbered { const char *pszPrefix; int nType; int nProfiles; };
		static const SNumbered numbered[] =
		{
			{ "od", REG_ATTROUT, VS11 },
			{ "ot", REG_TEXCRDOUT, VS11 },
			{ "oc", REG_COLOROUT, PS20 },
			{ "r", REG_TEMP, ALL },
			{ "v", REG_INPUT, ALL },
			{ "c", REG_CONST, ALL },
			{ "a", REG_ADDR, VS11 },
			{ "t", REG_TEXTURE, PS },
			{ "s", REG_SAMPLER, PS20 },
		};
		for ( const SNumbered &n : numbered )
		{
			const size_t nPrefix = strlen( n.pszPrefix );
			if ( sz.compare( 0, nPrefix, n.pszPrefix ) != 0 || sz.size() == nPrefix )
			{
				continue;
			}
			const std::string szDigits = sz.substr( nPrefix );
			if ( szDigits.find_first_not_of( "0123456789" ) != std::string::npos )
			{
				continue;
			}
			if ( !( n.nProfiles & profile.nProfile ) )
			{
				return Fail( "register " + sz + " does not exist in this shader model" );
			}
			const long nNumber = strtol( szDigits.c_str(), 0, 10 );
			if ( nNumber > 2047 )
			{
				return Fail( "register number out of range in " + sz );
			}
			*pnType = n.nType;
			*pnNumber = (int)nNumber;
			return true;
		}
		if ( sz.find( '[' ) != std::string::npos )
		{
			return Fail( "relative addressing is not supported: " + sz );
		}
		return Fail( "unknown register " + sz );
	}

	// Splits "r0_bx2.a" or "r0.a_bx2" into register, modifier suffix and selector
	static void SplitOperand( const std::string &sz, std::string *pszReg, std::string *pszSuffix, std::string *pszSelector )
	{
		std::string szRest = sz;
		const size_t nDot = szRest.find( '.' );
		if ( nDot != std::string::npos )
		{
			*pszSelector = szRest.substr( nDot + 1 );
			szRest.erase( nDot );
			const size_t nUnderscore = pszSelector->find( '_' );
			if ( nUnderscore != std::string::npos )
			{
				*pszSuffix = pszSelector->substr( nUnderscore + 1 );
				pszSelector->erase( nUnderscore );
			}
		}
		const size_t nUnderscore = szRest.find( '_' );
		if ( nUnderscore != std::string::npos )
		{
			*pszSuffix = szRest.substr( nUnderscore + 1 );
			szRest.erase( nUnderscore );
		}
		*pszReg = szRest;
	}

	// dwDefaultMask is the write mask when the operand has none
	bool ParseDestination( const std::string &sz, uint32_t dwResult, uint32_t dwDefaultMask, uint32_t *pdwToken )
	{
		std::string szReg, szSuffix, szMask;
		SplitOperand( sz, &szReg, &szSuffix, &szMask );
		if ( !szSuffix.empty() )
		{
			return Fail( "a destination takes no modifier: " + sz );
		}
		int nType, nNumber;
		if ( !ParseRegister( szReg, &nType, &nNumber ) )
		{
			return false;
		}
		uint32_t dwMask = dwDefaultMask;
		if ( sz.find( '.' ) != std::string::npos )
		{
			dwMask = 0;
			int nLast = -1;
			for ( char c : szMask )
			{
				const int n = ComponentIndex( c );
				// components in order and once each, as D3D requires of a write mask
				if ( n <= nLast )
				{
					return Fail( "invalid write mask in " + sz );
				}
				dwMask |= 1 << n;
				nLast = n;
			}
			if ( dwMask == 0 )
			{
				return Fail( "empty write mask in " + sz );
			}
		}
		*pdwToken = RegisterBits( nType, nNumber ) | ( dwMask << 16 ) | dwResult;
		return true;
	}

	// dwDefaultSwizzle is the swizzle when the operand has none
	bool ParseSource( const std::string &szOperand, uint32_t dwDefaultSwizzle, uint32_t *pdwToken )
	{
		std::string sz = szOperand;
		bool bNeg = false, bComp = false;
		if ( !sz.empty() && sz[0] == '-' )
		{
			bNeg = true;
			sz.erase( 0, 1 );
		}
		if ( sz.compare( 0, 2, "1-" ) == 0 )
		{
			bComp = true;
			sz.erase( 0, 2 );
		}
		std::string szReg, szSuffix, szSwizzle;
		SplitOperand( sz, &szReg, &szSuffix, &szSwizzle );

		int nMod = bNeg ? SRCMOD_NEG : SRCMOD_NONE;
		int nNeedProfiles = ALL;
		if ( bComp )
		{
			if ( bNeg || !szSuffix.empty() )
			{
				return Fail( "1- does not combine with another modifier: " + szOperand );
			}
			nMod = SRCMOD_COMP;
			nNeedProfiles = PS1X;
		}
		else if ( szSuffix == "bias" )
		{
			nMod = bNeg ? SRCMOD_BIASNEG : SRCMOD_BIAS;
			nNeedProfiles = PS1X;
		}
		else if ( szSuffix == "bx2" )
		{
			nMod = bNeg ? SRCMOD_SIGNNEG : SRCMOD_SIGN;
			nNeedProfiles = PS1X;
		}
		else if ( szSuffix == "x2" )
		{
			nMod = bNeg ? SRCMOD_X2NEG : SRCMOD_X2;
			nNeedProfiles = PS14;
		}
		else if ( szSuffix == "dz" || szSuffix == "db" )
		{
			nMod = SRCMOD_DZ;
			nNeedProfiles = PS14;
		}
		else if ( szSuffix == "dw" || szSuffix == "da" )
		{
			nMod = SRCMOD_DW;
			nNeedProfiles = PS14;
		}
		else if ( !szSuffix.empty() )
		{
			return Fail( "unknown source modifier _" + szSuffix );
		}
		if ( ( nMod == SRCMOD_DZ || nMod == SRCMOD_DW ) && bNeg )
		{
			return Fail( "_dz and _dw cannot be negated: " + szOperand );
		}
		if ( !( nNeedProfiles & profile.nProfile ) )
		{
			return Fail( "source modifier not available in this shader model: " + szOperand );
		}

		int nType, nNumber;
		if ( !ParseRegister( szReg, &nType, &nNumber ) )
		{
			return false;
		}
		uint32_t dwSwizzle = dwDefaultSwizzle;
		if ( sz.find( '.' ) != std::string::npos )
		{
			if ( szSwizzle.empty() || szSwizzle.size() > 4 )
			{
				return Fail( "invalid swizzle in " + szOperand );
			}
			// a short swizzle repeats its last component: .xy is .xyyy
			dwSwizzle = 0;
			for ( int i = 0; i < 4; ++i )
			{
				const int n = ComponentIndex( szSwizzle[std::min<size_t>( i, szSwizzle.size() - 1 )] );
				if ( n < 0 )
				{
					return Fail( "invalid swizzle in " + szOperand );
				}
				dwSwizzle |= uint32_t( n ) << ( 2 * i );
			}
		}
		*pdwToken = RegisterBits( nType, nNumber ) | ( dwSwizzle << 16 ) | ( uint32_t( nMod ) << 24 );
		return true;
	}

	// The result modifiers after a mnemonic: _sat, _pp, _centroid, _x2 ...
	bool ParseResultModifiers( const std::vector<std::string> &mods, uint32_t *pdwResult )
	{
		uint32_t dwResult = 0;
		for ( const std::string &m : mods )
		{
			uint32_t dwShift = 0;
			int nProfiles = 0;
			if ( m == "sat" )
			{
				dwResult |= RESULT_SAT;
				nProfiles = PS;
			}
			else if ( m == "pp" )
			{
				dwResult |= RESULT_PP;
				nProfiles = PS20;
			}
			else if ( m == "centroid" )
			{
				dwResult |= RESULT_CENTROID;
				nProfiles = PS20;
			}
			else if ( m == "x2" ) { dwShift = 1; nProfiles = PS1X; }
			else if ( m == "x4" ) { dwShift = 2; nProfiles = PS1X; }
			else if ( m == "x8" ) { dwShift = 3; nProfiles = PS14; }
			else if ( m == "d2" ) { dwShift = 15; nProfiles = PS1X; }
			else if ( m == "d4" ) { dwShift = 14; nProfiles = PS14; }
			else if ( m == "d8" ) { dwShift = 13; nProfiles = PS14; }
			else
			{
				return Fail( "unknown instruction modifier _" + m );
			}
			if ( !( nProfiles & profile.nProfile ) )
			{
				return Fail( "modifier _" + m + " not available in this shader model" );
			}
			if ( dwShift != 0 )
			{
				if ( dwResult & 0x0F000000 )
				{
					return Fail( "more than one result shift" );
				}
				dwResult |= dwShift << 24;
			}
		}
		*pdwResult = dwResult;
		return true;
	}

	// Emits an instruction token and its parameters, with the length field
	// shader model 2 has and 1.x leaves zero
	void Emit( uint32_t dwInstruction, const std::vector<uint32_t> &params )
	{
		if ( profile.nProfile == PS20 )
		{
			dwInstruction |= uint32_t( params.size() ) << 24;
		}
		tokens.push_back( dwInstruction );
		tokens.insert( tokens.end(), params.begin(), params.end() );
	}

	// dcl, dcl_2d, dcl_cube, dcl_volume, dcl_position, dcl_texcoord1 ...
	bool AssembleDcl( const std::string &szMnemonic, const std::vector<std::string> &operands )
	{
		if ( operands.size() != 1 )
		{
			return Fail( "dcl takes one register" );
		}
		const std::string szUsage = szMnemonic.size() > 3 ? szMnemonic.substr( 4 ) : std::string();
		uint32_t dwUsageToken = PARAM_BIT;
		if ( profile.nProfile == VS11 )
		{
			// the vertex element a v register is fed from, and its index
			static const char *usages[] =
			{
				"position", "blendweight", "blendindices", "normal", "psize", "texcoord", "tangent",
				"binormal", "tessfactor", "positiont", "color", "fog", "depth", "sample",
			};
			const size_t nDigits = szUsage.find_first_of( "0123456789" );
			const std::string szName = szUsage.substr( 0, nDigits );
			const int nIndex = nDigits == std::string::npos ? 0 : atoi( szUsage.c_str() + nDigits );
			int nUsage = -1;
			for ( int i = 0; i < (int)std::size( usages ); ++i )
			{
				if ( szName == usages[i] )
				{
					nUsage = i;
				}
			}
			if ( nUsage < 0 || nIndex > 15 )
			{
				return Fail( "unknown vertex input usage dcl_" + szUsage );
			}
			dwUsageToken |= uint32_t( nUsage ) | ( uint32_t( nIndex ) << 16 );
		}
		else if ( profile.nProfile == PS20 )
		{
			// texture registers and colours are declared bare, samplers by texture type
			if ( szUsage == "2d" )
			{
				dwUsageToken |= 2u << 27;
			}
			else if ( szUsage == "cube" )
			{
				dwUsageToken |= 3u << 27;
			}
			else if ( szUsage == "volume" )
			{
				dwUsageToken |= 4u << 27;
			}
			else if ( !szUsage.empty() )
			{
				return Fail( "unknown declaration dcl_" + szUsage );
			}
		}
		else
		{
			return Fail( "ps.1.x has no dcl" );
		}
		uint32_t dwReg;
		if ( !ParseDestination( operands[0], 0, 0xF, &dwReg ) )
		{
			return false;
		}
		Emit( OPCODE_DCL, { dwUsageToken, dwReg } );
		return true;
	}

	bool AssembleDef( const std::vector<std::string> &operands )
	{
		if ( operands.size() != 5 )
		{
			return Fail( "def takes a constant register and four values" );
		}
		uint32_t dwReg;
		if ( !ParseDestination( operands[0], 0, 0xF, &dwReg ) )
		{
			return false;
		}
		std::vector<uint32_t> params = { dwReg };
		for ( int i = 1; i < 5; ++i )
		{
			char *pEnd = 0;
			const float f = strtof( operands[i].c_str(), &pEnd );
			if ( pEnd == operands[i].c_str() || *pEnd != 0 )
			{
				return Fail( "def value is not a number: " + operands[i] );
			}
			uint32_t dw;
			memcpy( &dw, &f, sizeof( dw ) );
			params.push_back( dw );
		}
		Emit( OPCODE_DEF, params );
		return true;
	}

	bool AssembleLine( const std::string &szLine )
	{
		std::string sz = szLine;
		bool bCoissue = false;
		if ( sz[0] == '+' )
		{
			if ( !( profile.nProfile & PS1X ) )
			{
				return Fail( "co-issue exists only in ps.1.x" );
			}
			bCoissue = true;
			sz.erase( 0, 1 );
		}

		// the mnemonic runs to the first space; operands are comma separated,
		// and spaces inside them mean nothing
		size_t nSpace = 0;
		while ( nSpace < sz.size() && isspace( (unsigned char)sz[nSpace] ) )
		{
			++nSpace;
		}
		sz.erase( 0, nSpace );
		const size_t nEnd = sz.find_first_of( " \t" );
		const std::string szMnemonic = sz.substr( 0, nEnd );
		std::vector<std::string> operands;
		if ( nEnd != std::string::npos )
		{
			std::string szOperand;
			for ( size_t i = nEnd; i <= sz.size(); ++i )
			{
				if ( i == sz.size() || sz[i] == ',' )
				{
					if ( szOperand.empty() )
					{
						if ( i != sz.size() || !operands.empty() )
						{
							return Fail( "empty operand" );
						}
					}
					else
					{
						operands.push_back( szOperand );
					}
					szOperand.clear();
				}
				else if ( !isspace( (unsigned char)sz[i] ) )
				{
					szOperand += sz[i];
				}
			}
		}

		if ( szMnemonic == "dcl" || szMnemonic.compare( 0, 4, "dcl_" ) == 0 )
		{
			return AssembleDcl( szMnemonic, operands ) && ( !bCoissue || Fail( "dcl cannot be co-issued" ) );
		}
		if ( szMnemonic == "def" )
		{
			return AssembleDef( operands ) && ( !bCoissue || Fail( "def cannot be co-issued" ) );
		}
		if ( szMnemonic == "phase" )
		{
			if ( profile.nProfile != PS14 || !operands.empty() || bCoissue )
			{
				return Fail( "phase exists only in ps.1.4, on its own" );
			}
			tokens.push_back( OPCODE_PHASE );
			return true;
		}

		// "mul_x4_sat" is mul with _x4 and _sat
		std::vector<std::string> parts;
		for ( size_t nStart = 0;; )
		{
			const size_t nUnderscore = szMnemonic.find( '_', nStart );
			parts.push_back( szMnemonic.substr( nStart, nUnderscore - nStart ) );
			if ( nUnderscore == std::string::npos )
			{
				break;
			}
			nStart = nUnderscore + 1;
		}
		const SInstruction *pInstruction = 0;
		bool bNameKnown = false;
		for ( const SInstruction &ins : instructions )
		{
			if ( parts[0] == ins.pszName )
			{
				bNameKnown = true;
				if ( ins.nProfiles & profile.nProfile )
				{
					pInstruction = &ins;
					break;
				}
			}
		}
		if ( !pInstruction )
		{
			return Fail( bNameKnown ? "instruction " + parts[0] + " is not available in this shader model" : "unknown instruction " + parts[0] );
		}
		const std::vector<std::string> mods( parts.begin() + 1, parts.end() );
		uint32_t dwResult;
		if ( !ParseResultModifiers( mods, &dwResult ) )
		{
			return false;
		}
		if ( (int)operands.size() != pInstruction->nDst + pInstruction->nSrc )
		{
			return Fail( parts[0] + " takes " + std::to_string( pInstruction->nDst + pInstruction->nSrc ) + " operands" );
		}
		// all four components, and xyzw, unless vs.1.1 implies otherwise
		const bool bVS = profile.nProfile == VS11;
		const uint32_t dwDefaultMask = bVS && pInstruction->dwVSDefaultMask ? pInstruction->dwVSDefaultMask : 0xF;
		const uint32_t dwDefaultSwizzle = bVS && pInstruction->bVSScalarSource ? SWIZZLE_WWWW : SWIZZLE_XYZW;
		std::vector<uint32_t> params;
		for ( int i = 0; i < (int)operands.size(); ++i )
		{
			uint32_t dw;
			const bool bOk = i < pInstruction->nDst ? ParseDestination( operands[i], dwResult, dwDefaultMask, &dw ) : ParseSource( operands[i], dwDefaultSwizzle, &dw );
			if ( !bOk )
			{
				return false;
			}
			params.push_back( dw );
		}
		if ( pInstruction->nDst == 0 && dwResult != 0 )
		{
			return Fail( parts[0] + " has no destination to modify" );
		}
		uint32_t nOpcode = pInstruction->nOpcode;
		// D3DX writes sub as an add of the negated second source in vs.1.1 and
		// ps.2.0, whose only source modifier is the negation, and keeps it as sub
		// in ps.1.x, where the source may carry _bx2, _bias or 1- instead.
		if ( nOpcode == OPCODE_SUB && ( profile.nProfile & ( VS11 | PS20 ) ) )
		{
			nOpcode = OPCODE_ADD;
			params[2] ^= uint32_t( SRCMOD_NEG ) << 24;
		}
		Emit( nOpcode | pInstruction->dwControl | ( bCoissue ? COISSUE_BIT : 0 ), params );
		return true;
	}

public:
	CAssembler( std::vector<uint32_t> &_tokens ): tokens( _tokens ), nLine( 0 ) {}

	bool Run( std::string_view source, std::string *pszError )
	{
		bool bHaveVersion = false;
		size_t nPos = 0;
		while ( nPos <= source.size() && szError.empty() )
		{
			size_t nEol = source.find( '\n', nPos );
			if ( nEol == std::string_view::npos )
			{
				nEol = source.size();
			}
			std::string szLine( source.substr( nPos, nEol - nPos ) );
			nPos = nEol + 1;
			++nLine;

			// comments, then the case: the syntax is case insensitive
			const size_t nSemicolon = szLine.find( ';' );
			const size_t nSlashes = szLine.find( "//" );
			szLine.erase( std::min( nSemicolon, nSlashes ) == std::string::npos ? szLine.size() : std::min( nSemicolon, nSlashes ) );
			for ( char &c : szLine )
			{
				c = (char)tolower( (unsigned char)c );
			}
			const size_t nFirst = szLine.find_first_not_of( " \t\r" );
			if ( nFirst == std::string::npos )
			{
				continue;
			}
			szLine.erase( 0, nFirst );
			szLine.erase( szLine.find_last_not_of( " \t\r" ) + 1 );

			if ( !bHaveVersion )
			{
				if ( !ParseVersion( szLine, &profile ) )
				{
					Fail( "expected vs.1.1, ps.1.1, ps.1.4 or ps.2.0, found " + szLine );
					break;
				}
				tokens.push_back( profile.dwVersion );
				bHaveVersion = true;
				continue;
			}
			AssembleLine( szLine );
		}
		if ( szError.empty() && !bHaveVersion )
		{
			Fail( "no version line" );
		}
		if ( !szError.empty() )
		{
			tokens.clear();
			if ( pszError )
			{
				*pszError = szError;
			}
			return false;
		}
		tokens.push_back( TOKEN_END );
		return true;
	}
};

}

bool Assemble( std::string_view source, std::vector<uint32_t> *pTokens, std::string *pszError )
{
	pTokens->clear();
	CAssembler assembler( *pTokens );
	return assembler.Run( source, pszError );
}

}
