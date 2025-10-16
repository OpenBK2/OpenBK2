#include "stdafx.h"
#include "RandomGen.h"

#include "port/time.h"

#include <cstdint>

SRandomSeed::SRandomSeed() : nSeed( GetCurrentTimeMilliseconds() )
{
}

SRandomSeed::SRandomSeed( int seed ) : nSeed( seed )
{
}

SRand::SRand() : seed( GetCurrentTimeMilliseconds() )
{
	Get( 4 );
}

int SRand::Get( int nMax )
{
	return (((seed.nSeed = seed.nSeed * 214013L + 2531011L) >> 16) & 0x7fff) * nMax / 0x8000;
}

// CRoulette

int CRoulette::GetRandomSector( SRand *pRand ) const
{
	ASSERT( pRand );
	if ( m_aSectors.back() == 0 )
		return 0;
	float fValue = pRand->GetFloat( 0.0f, m_aSectors.back() );
	int nBegin = 0;
	int nEnd = m_aSectors.size() - 1;
	while ( nBegin + 1 < nEnd )
	{
		int nCenter = nBegin + ( nEnd - nBegin ) / 2;
		if ( fValue < m_aSectors[nCenter] )
			nEnd = nCenter;
		else
			nBegin = nCenter;
	}
	return nBegin;
}

void CRoulette::AddSector( float fValue )
{
	m_aSectors.push_back( m_aSectors.back() + fValue );
}

int CRoulette::operator&( CStructureSaver &f )
{
	f.Add( 1, &m_aSectors );
	return 0;
}

CRandomGenerator random;

const LPCSTR PSZ_MASK_TO_FIND_FILES = "C:\\*.*";

#define ind(mm,x)  (*(unsigned int *)(( unsigned _int8 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))

#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (a^(mix)) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
   a^=b<<11; d+=a; b+=c; \
   b^=c>>2;  e+=b; c+=d; \
   c^=d<<8;  f+=c; d+=e; \
   d^=e>>16; g+=d; e+=f; \
   e^=f<<10; h+=e; f+=g; \
   f^=g>>4;  a+=f; g+=h; \
   g^=h<<8;  b+=g; h+=a; \
   h^=a>>9;  c+=h; a+=b; \
}

void CRandomGenerator::Isaac()
{
	unsigned int a, b, x, y, *m, *mm, *m2, *r, *mend;
	mm = randmem; 
	r = randrsl;
	a = randa; 
	b = randb + ( ++randc );
	for ( m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; )
	{
		rngstep( a<<13, a, b, mm, m, m2, r, x );
		rngstep( a>>6 , a, b, mm, m, m2, r, x );
		rngstep( a<<2 , a, b, mm, m, m2, r, x );
		rngstep( a>>16, a, b, mm, m, m2, r, x );
	}
	for ( m2 = mm; m2<mend; )
	{
		rngstep( a<<13, a, b, mm, m, m2, r, x );
		rngstep( a>>6 , a, b, mm, m, m2, r, x );
		rngstep( a<<2 , a, b, mm, m, m2, r, x );
		rngstep( a>>16, a, b, mm, m, m2, r, x );
	}
	randb = b; 
	randa = a;
}

void CRandomGenerator::Init()
{
	int i;
	unsigned int a, b, c, d, e, f, g, h;
	unsigned int *m, *r;

	FillRandRsl();
	randa = randb = randc = 0;
	m = randmem;
	r = randrsl;
	a = b = c = d = e = f = g = h = 0x9e3779b9;  /* the golden ratio */

	for ( i=0; i<4; ++i )          /* scramble it */
		mix( a, b, c, d, e, f, g, h );

	/* initialize using the contents of r[] as the seed */
	for ( i=0; i<RANDSIZ; i+=8 )
	{
		a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
		e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
		mix( a, b, c, d, e, f, g, h );
		m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
		m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
	}
	/* do a second pass to make all of the seed affect all of m */
	for (i=0; i<RANDSIZ; i+=8)
	{
		a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
		e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
		mix(a,b,c,d,e,f,g,h);
		m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
		m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
	}
	Isaac();				/* fill in the first set of results */
	randcnt=RANDSIZ;		/* prepare to use the first set of results */
}

// --------------------------- FillRandRsl() ---------------------------------------------------------------
const int N_FROM_START = 1024;

BOOL CRandomGenerator::RecFindFile( std::string &szFoundName, const char *pszBaseMask, int nToFind, int* pnTotFinded )
{
	WIN32_FIND_DATA ff;
	HANDLE hf = FindFirstFile( pszBaseMask, &ff );
	std::string szPath( pszBaseMask );
	szPath = szPath.substr( 0, szPath.length() - 3 );
	if ( hf != INVALID_HANDLE_VALUE )
	{
		for ( BOOL bCont = TRUE; bCont; bCont = FindNextFile( hf, &ff ) )
		{
			if ( ff.cFileName[0] == '.' || (ff.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)) != 0 )
				continue;
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if ( RecFindFile( szFoundName, (szPath + ff.cFileName + "\\*.*").c_str(), nToFind, pnTotFinded ) == TRUE )
					return TRUE;
				continue;
			}
			if ( *pnTotFinded >= nToFind )
			{
				if ( ff.nFileSizeLow >= N_FROM_START + sizeof( randrsl ) )
				{
					szFoundName = szPath + ff.cFileName;
					return TRUE;
				}
				( *pnTotFinded )--;
			}
			( *pnTotFinded )++;
		}
		FindClose( hf );
	}
	return FALSE;
}

// The purpose of this func is to fill randrsl[RANDSIZ] arrays
// with initial random values
// It's uses first RANDSIZ values from random file for this
void CRandomGenerator::FillRandRsl()
{
	srand( GetCurrentTimeMilliseconds() );
	int n = GetCurrentTimeMilliseconds();
	for ( int k = 0; k < RANDSIZ; ++k )
	{
		randrsl[k] = n;
		n = n * 124325 + 12341289;
	}
	std::string szFoundName;
	for ( int nSafe = 0; nSafe < 10; ++nSafe )
	{
		int nTotFinded = 0;
		if ( !RecFindFile( szFoundName, PSZ_MASK_TO_FIND_FILES, rand() % 512 + 1, &nTotFinded ) )
		{
			if ( nTotFinded < 2 )
				break;
			int nToFind = rand() % ( nTotFinded - 1 ) + 1;
			nTotFinded = 0;
			if ( !RecFindFile( szFoundName, PSZ_MASK_TO_FIND_FILES, nToFind, &nTotFinded ) )
				continue;
		}
		HANDLE hf = CreateFile( szFoundName.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0 );
		if ( hf != INVALID_HANDLE_VALUE )
		{
			bool bIsRead = true;
			SetFilePointer( hf, N_FROM_START - rand() % ( N_FROM_START - 512 ), 0, FILE_BEGIN );
			unsigned long dwRes;
			if ( !ReadFile( hf, randrsl, sizeof(randrsl ), &dwRes, 0 ) )
				bIsRead = false;
			CloseHandle( hf );
			if ( !bIsRead )
				continue;
		}
		BOOL bHaveNotZero = FALSE;
		for ( int i = 0; i < RANDSIZ; i++ )
			if ( randrsl[i] )
			{
				bHaveNotZero = TRUE;
				break;
			}
		if ( bHaveNotZero == FALSE )
			continue;
		break;
	}
	for ( int i = 0; i < RANDSIZ; i++ )
		randrsl[i] ^= rand();
}


