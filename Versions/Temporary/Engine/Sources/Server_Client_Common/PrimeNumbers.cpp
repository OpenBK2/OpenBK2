#include "stdafx.h"

#include "PrimeNumbers.h"

const int N_NUMBERS = 1000;

CPrimeNumbers::CPrimeNumbers()
{
	numbers.resize( N_NUMBERS );

	vector<BYTE> canBePrime;
	canBePrime.resize( N_NUMBERS * 50, 1 );

	int nFound = 0;
	int nNum = 2;
	while ( nFound < N_NUMBERS )
	{
		NI_ASSERT( nNum < canBePrime.size(), "Size of canbe prime is to small" );

		numbers[nFound++] = nNum;

		isPrime.insert( nNum );
		for ( int i = 2; i < canBePrime.size() / nNum; ++i )
		{
			NI_ASSERT( nNum * i < canBePrime.size(), "Wrong n" )
			canBePrime[nNum * i] = 0;
		}

		++nNum;
		while ( nNum < canBePrime.size() && canBePrime[nNum] == 0 )
			++nNum;
	}
}


