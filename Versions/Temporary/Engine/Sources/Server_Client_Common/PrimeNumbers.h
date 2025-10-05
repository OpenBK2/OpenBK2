#pragma once

#include "Server_Client_Common_export.h"


class SERVER_CLIENT_COMMON_EXPORT CPrimeNumbers : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CPrimeNumbers );
	std::vector<int> numbers;
	std::unordered_set<int> isPrime;
public:
	CPrimeNumbers();

	const int GetNNumbers() const { return numbers.size(); }
	const int GetPrime( const int nIndex ) const { return numbers[nIndex]; }
	bool IsPrime( const int nNumber ) { return isPrime.find( nNumber ) != isPrime.end(); }
};


