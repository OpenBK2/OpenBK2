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
	vector<float> m_aSectors;
public:
	CRoulette() : m_aSectors( 1, 0.0f ) {}
	void AddSector( float fValue );
	int GetRandomSector( SRand *pRand ) const;
	float GetSectorValue( int nIndex ) const { return m_aSectors[ nIndex + 1 ] - m_aSectors[ nIndex ]; }
	int operator&( CStructureSaver &f );
};

/*
 	Random generator class header
	Uses ISAAC random generator, (c) Bob Jenkins with
	with using random file for initial random seeds

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
	void Init();  // very slow operation
	void Isaac();
	void FillRandRsl();
	BOOL RecFindFile( string &szFoundName, const char *pszBaseDir, int nToFind, int* pnTotFinded );
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

extern CRandomGenerator random;


