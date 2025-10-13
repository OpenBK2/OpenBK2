#include "stdafx.h"

#include "FileUtils.h"
#include "RandomGenInternal.h"
#include "Commands.h"
#include "XmlSaver.h"

#include <cstdint>
#include <vector>
#include <string>

// ************************************************************************************************************************ //
// **
// ** random generator
// **
// **
// **
// ************************************************************************************************************************ //

static bool s_bLogRandomCalls = false;
namespace NRandom
{
	SRandData rnd;
	unsigned __int64 nRandomCallsTotal;
#ifndef _FINALRELEASE
	int nRandomCalls;
#endif

	// new random vals recalculation
	void Isaac( SRandData *pRnd );
	// initialize random generator with random seed
	void SetRandomSeed( IRandomSeed *pSeed );
	// create copy of the current random gen seed and return it
	IRandomSeed *CreateRandomSeedCopy();
	// copy compact debug info for ASYNC diagnostics
	void GetDebugState( SDebugState *pState );
	// cheap counter-only accessor for hot-path diagnostics
	unsigned __int64 GetRandomCallsCounter();
	// get random value
	UINT Random()
	{
		++nRandomCallsTotal;
		//DEBUG{
#ifndef _FINALRELEASE
		if ( s_bLogRandomCalls )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_DEBUG_WINDOW + 2, 
				StrFmt( "RandomCall %i", nRandomCalls ) );
			DebugTrace( "RandomCall %i", nRandomCalls );
			++nRandomCalls;
		}
#endif
		//DEBUG}
		if ( rnd.randcnt-- == 0 )
		{
			Isaac( &rnd );
			rnd.randcnt = RANDSIZ - 1;
		}
		return rnd.randrsl[rnd.randcnt];
	}

	static unsigned long CalcDebugChecksum( const unsigned _int32 *pValues, const int nCount )
	{
		unsigned long nChecksum = 2166136261u;
		for ( int i = 0; i < nCount; ++i )
		{
			nChecksum ^= pValues[i];
			nChecksum *= 16777619u;
		}
		return nChecksum;
	}

	void GetDebugState( SDebugState *pState )
	{
		if ( pState == 0 )
			return;

		pState->randcnt = rnd.randcnt;
		pState->randa = rnd.randa;
		pState->randb = rnd.randb;
		pState->randc = rnd.randc;
		pState->randrslChecksum = CalcDebugChecksum( rnd.randrsl, RANDSIZ );
		pState->randmemChecksum = CalcDebugChecksum( rnd.randmem, RANDSIZ );
		pState->randomCalls = nRandomCallsTotal;
	}

	unsigned __int64 GetRandomCallsCounter()
	{
		return nRandomCallsTotal;
	}

	std::vector<RngCall> Calls;
	int ci = 0;

	bool RecordCall(const char* file, int line, const char* func)
	{
		static const int MAX_CALLS_STORAGE = 512;

		if (Calls.size() < MAX_CALLS_STORAGE)
		{
			Calls.push_back({ci, file, line, func});
			ci++;
			return true;
		}

		size_t modIndex = ci % MAX_CALLS_STORAGE;
		Calls[modIndex] = {ci, file, line, func};
		ci++;
		return true;
	}

	void DumpRecords(FILE* f)
	{
		std::vector<RngCall> calls = Calls;
		std::sort(calls.begin(), calls.end(), [](const RngCall& c1, const RngCall& c2) {
			return c1.callNumber < c2.callNumber;
		});

		for (size_t i = 0; i < calls.size(); i++)
		{
			const char* file = calls[i].file != 0 ? calls[i].file : "<unknown>";
			const char* function = calls[i].function != 0 ? calls[i].function : "<unknown>";
			std::string location = std::string(file) + ":" + std::to_string(calls[i].line) + " at: " + function;
			fprintf(f, "\t#%d %s\n", calls[i].callNumber, location.c_str());
		}
	}

	static SRandomFunc rndFunc;
	const SRandomFunc& RndFunc()
	{
		return rndFunc;
	}
};

#define ind(mm,x)  (*(unsigned _int32 *)(( unsigned _int8 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))

#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (a ^ (mix)) + *(m2++); \
  *(m++) = y = ind( mm, x ) + a + b; \
  *(r++) = b = ind( mm, y >> RANDSIZL ) + x; \
}

#define mix(a,b,c,d,e,f,g,h) \
{ \
   a ^= b << 11; d += a; b += c; \
   b ^= c >> 2;  e += b; c += d; \
   c ^= d << 8;  f += c; d += e; \
   d ^= e >> 16; g += d; e += f; \
   e ^= f << 10; h += e; f += g; \
   f ^= g >> 4;  a += f; g += h; \
   g ^= h << 8;  b += g; h += a; \
   h ^= a >> 9;  c += h; a += b; \
}

void NRandom::Isaac( SRandData *pRnd )
{
	unsigned _int32 a, b, x, y, *m, *mm, *m2, *r, *mend;
	mm = pRnd->randmem; 
	r = pRnd->randrsl;
	a = pRnd->randa; 
	b = pRnd->randb + ( ++pRnd->randc );
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
	pRnd->randb = b; 
	pRnd->randa = a;
}

struct SRandomGenAutoMagic
{
	SRandomGenAutoMagic()
	{
		CPtr<CRandomGenSeed> pSeed = new CRandomGenSeed();
		pSeed->Init();
		NRandom::SetRandomSeed( pSeed );
	}
};
SRandomGenAutoMagic automagic;

void NRandom::SetRandomSeed( IRandomSeed *pSeed )
{
	nRandomCallsTotal = 0;
#ifndef _FINALRELEASE
	nRandomCalls = 0;
#endif
	if ( CRandomGenSeed *pRGS = checked_cast<CRandomGenSeed*>(pSeed) )
		rnd = pRGS->GetRandData();
	else
	{
		NI_ASSERT( false, "Wrong class as a random seed" );
	}
}
IRandomSeed *NRandom::CreateRandomSeedCopy()
{
	CRandomGenSeed *pSeed = new CRandomGenSeed();
	pSeed->SetRandData( rnd );
	return pSeed;
}

// ************************************************************************************************************************ //
// **
// ** random generator seed
// **
// **
// **
// ************************************************************************************************************************ //

void __declspec(dllexport) CRandomGenSeed::SFLB0_InitVariables()
{
	rnd.randa = rnd.randb = rnd.randc = 0;
	unsigned _int32 *m = rnd.randmem;
	unsigned _int32 *r = rnd.randrsl;
	unsigned _int32 a, b, c, d, e, f, g, h;
	a = b = c = d = e = f = g = h = 0x9e3779b9;  // the golden ratio
	// scramble it
	for ( int i = 0; i < 4; ++i )
		mix( a, b, c, d, e, f, g, h );
	// initialize using the contents of r[] as the seed
	for ( int i = 0; i < RANDSIZ; i += 8 )
	{
		a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
		e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
		mix( a, b, c, d, e, f, g, h );
		m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
		m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
	}
	// do a second pass to make all of the seed affect all of m_
	for ( int i = 0; i < RANDSIZ; i += 8 )
	{
		a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
		e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
		mix( a, b, c, d, e, f, g, h );
		m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
		m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
	}
	// fill in the first set of results
	NRandom::Isaac( &rnd );
	// prepare to use the first set of results
	rnd.randcnt = RANDSIZ;		
}

void CRandomGenSeed::Init()
{
	//DEBUG{ fixed random
	//Zero( rnd.randrsl );
	//DEBUG}

	FillRandRsl();

	SFLB0_InitVariables();
}

void CRandomGenSeed::InitByZeroSeed()
{
	Zero( rnd.randrsl );
	SFLB0_InitVariables();
}

const int N_FROM_START = 1024;
bool CRandomGenSeed::RecFindFile( LPSTR pszFindedName, LPCSTR pszBaseMask, int nToFind, int* pnTotFinded )
{
	for ( NFile::CFileIterator fileFind( pszBaseMask ); !fileFind.IsEnd(); ++fileFind )
	{
		if ( fileFind.IsDots() || fileFind.IsSystem() || fileFind.IsHidden() )
			continue;
		if ( fileFind.IsDirectory() )
		{
			if ( RecFindFile( pszFindedName, (fileFind.GetFullName() + "\\*.*").c_str(), nToFind, pnTotFinded ) == TRUE )
				return true;
			continue;
		}
		if ( *pnTotFinded >= nToFind )
		{
			if ( fileFind.GetLength() >= N_FROM_START + sizeof(rnd.randrsl) )
			{
				strcpy( pszFindedName, fileFind.GetFullName().c_str() );
				return true;
			}
			( *pnTotFinded )--;
		}
		( *pnTotFinded )++;
	}
	return false;
}

// The purpose of this func is to fill randrsl[RANDSIZ] arrays
// with initial random values
// It's uses first RANDSIZ values from random file for this
void CRandomGenSeed::FillRandRsl()
{
	// find first drive
	char buf[1024];
	char pszMaskToFindFiles[256];
	int nSize = GetLogicalDriveStrings( sizeof(buf), buf );
	int i;
	for ( i = 0; i < nSize; )
	{
		char* pDrive = buf + i;
		i += strlen( pDrive ) + 1;
		if ( GetDriveType(pDrive) == DRIVE_FIXED || GetDriveType(pDrive) == DRIVE_REMOTE )
		{
			strcpy( pszMaskToFindFiles, pDrive );
			strcat( pszMaskToFindFiles, "*.*" );
			break;
		}
	}
	if ( i == nSize )
		return; // cannot find any hd, run without initialization
	//
	srand( GetTickCount() );
	char pszFindedName[256];
	BOOL bSuccess = FALSE;
	
	while ( !bSuccess )
	{
		int nTotFinded = 0;
		if ( !RecFindFile( pszFindedName, pszMaskToFindFiles, rand() % 512 + 1, &nTotFinded ) )
		{
			int nToFind = rand() % ( nTotFinded - 1 ) + 1;
			nTotFinded = 0;
			if ( !RecFindFile( pszFindedName, pszMaskToFindFiles, nToFind, &nTotFinded ) )
				continue;
		}
		bSuccess = TRUE;
		HANDLE hFile = CreateFile( pszFindedName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			srand( GetTickCount() );
			SetFilePointer( hFile, N_FROM_START - rand() % ( N_FROM_START - 512 ), 0, FILE_BEGIN );
			unsigned long dwReadBytes = 0;
			if ( ReadFile( hFile, rnd.randrsl, sizeof(rnd.randrsl), &dwReadBytes, 0 ) != TRUE || (dwReadBytes != sizeof(rnd.randrsl)) )
				bSuccess = FALSE;
			CloseHandle( hFile );
		}
		else
			bSuccess = FALSE;
		BOOL bHaveNotZero = FALSE;
		for ( int i = 0; i < RANDSIZ; i++ )
		{
			if ( rnd.randrsl[i] )
			{
				bHaveNotZero = TRUE;
				break;
			}
		}
		if ( bHaveNotZero == FALSE )
		{
			bSuccess = FALSE;
			Sleep( 10 );
		}
		for ( i = 0; i < RANDSIZ; i++ )
			rnd.randrsl[i] ^= rand();
	}
}

void CRandomGenSeed::Store( CDataStream *pStream )
{
	pStream->Write( &rnd.randcnt, sizeof(rnd.randcnt) );
	pStream->Write( &(rnd.randrsl[0]), sizeof(rnd.randrsl) );
	pStream->Write( &(rnd.randmem[0]), sizeof(rnd.randmem) );
	pStream->Write( &rnd.randa, sizeof(rnd.randa) );
	pStream->Write( &rnd.randb, sizeof(rnd.randb) );
	pStream->Write( &rnd.randc, sizeof(rnd.randc) );
}

void CRandomGenSeed::Restore( CDataStream *pStream )
{
	pStream->Read( &rnd.randcnt, sizeof(rnd.randcnt) );
	pStream->Read( &(rnd.randrsl[0]), sizeof(rnd.randrsl) );
	pStream->Read( &(rnd.randmem[0]), sizeof(rnd.randmem) );
	pStream->Read( &rnd.randa, sizeof(rnd.randa) );
	pStream->Read( &rnd.randb, sizeof(rnd.randb) );
	pStream->Read( &rnd.randc, sizeof(rnd.randc) );
}

int CRandomGenSeed::operator&( IBinSaver &saver )
{
	saver.Add( 1, &rnd.randcnt );
	saver.AddRawData( 2, &(rnd.randrsl[0]), sizeof(rnd.randrsl) );
	saver.AddRawData( 3, &(rnd.randmem[0]), sizeof(rnd.randmem) );
	saver.Add( 4, &rnd.randa );
	saver.Add( 5, &rnd.randb );
	saver.Add( 6, &rnd.randc );
	return 0;
}

int CRandomGenSeed::operator&( IXmlSaver &saver )
{
	saver.Add( "RandCounter", &rnd.randcnt );
	saver.Add( "RandA", &rnd.randa );
	saver.Add( "RandB", &rnd.randb );
	saver.Add( "RandC", &rnd.randc );
	saver.AddRawData( "RandRSL", &(rnd.randrsl[0]), sizeof(rnd.randrsl) );
	saver.AddRawData( "RandMem", &(rnd.randmem[0]), sizeof(rnd.randmem) );
	return 0;
}

START_REGISTER(RandomGen_0x10170B00)
REGISTER_VAR_EX( "log_random_calls", NGlobal::VarBoolHandler, &s_bLogRandomCalls, false, STORAGE_NONE );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( 0x1009DC80, CRandomGenSeed )

