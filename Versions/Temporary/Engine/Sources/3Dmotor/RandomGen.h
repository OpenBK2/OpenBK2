#pragma once

struct SRandomSeed
{
	int nSeed;
	SRandomSeed();
	SRandomSeed( int seed );
};

struct SRand
{
	SRandomSeed seed;
	
	SRand();
	SRand( const SRandomSeed &rseed ) : seed( rseed ) {}
	int Get( int nMax );
	float GetFloat( float fpMin, float fpMax );
};

class CRoulette
{
	std::vector<float> m_aSectors;
public:
	CRoulette() : m_aSectors( 1, 0.0f ) {}
	void AddSector( float fValue );
	int GetRandomSector( SRand *pRand ) const;
	float GetSectorValue( int nIndex ) const { return m_aSectors[ nIndex + 1 ] - m_aSectors[ nIndex ]; }
	int operator&( CStructureSaver &f );
};

/*
 	Random generator class header
	Uses the ISAAC random generator, (c) Bob Jenkins, seeded from std::random_device

	Written by [REDACTED]
	[REDACTED], 1998
*/

const int RANDSIZL = 8;
const int RANDSIZ = 1 << RANDSIZL;

class CRandomGenerator
{
private:
	unsigned int randcnt;
	unsigned int randrsl[RANDSIZ];
	unsigned int randmem[RANDSIZ];
	unsigned int randa;
	unsigned int randb;
	unsigned int randc;
public:
	CRandomGenerator() { Init(); }
	unsigned int Get();
	unsigned int Get( unsigned int nMax ) { ASSERT( nMax != 0 ); return Get() % nMax; }
	unsigned int Get( unsigned int nMin, unsigned int nMax )	{ return Get( nMax-nMin ) + nMin; }
	float GetFloat( float fpMin, float fpMax );
	bool Check( int nCheck, int nRange = 100 ) { return Get(nRange) < nCheck; }
	bool NegCheck( int nCheck, int nRange = 100 ) { return Get(nRange) >= nCheck; }
private:
	void Init();
	void Isaac();
	void FillRandRsl();
};

inline unsigned int CRandomGenerator::Get()
{
	if ( randcnt-- == 0 )
	{
		Isaac();
		randcnt=RANDSIZ-1;
	}
	return randrsl[ randcnt ];
}

inline float CRandomGenerator::GetFloat( float fpMin, float fpMax )
{
	ASSERT( fpMin <= fpMax );
	return fpMin + float( Get() * ( ( fpMax - fpMin ) * (1/double(0xFFFFFFFF)) ) );
}

inline float SRand::GetFloat( float fpMin, float fpMax )
{
	ASSERT( fpMin <= fpMax );
	return fpMin + float( Get( 0xFFFF ) * ( ( fpMax - fpMin ) * (1/double(0xFFFF)) ) );
}

// Not `random`: POSIX declares `long random( void )` in <stdlib.h>, which arrives
// through <cstdlib> in the prelude, and an object cannot share the name.
//
// Presentation only, and deliberately not reproducible - Init seeds from a
// non-deterministic source. Nothing the simulation reads may come from here, or
// clients diverge; AILogic uses System/RandomGen.h and NRandom for that.
extern CRandomGenerator randomGen;


