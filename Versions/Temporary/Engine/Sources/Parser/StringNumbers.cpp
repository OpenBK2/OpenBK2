#include "stdafx.h"

#include "StringNumbers.h"
#include "../Misc/StrProc.h"

namespace NStr
{

bool IsInt( const string &szVal )
{
	if ( szVal.empty() )
		return false;
	
	int n = 0;
	while ( n < szVal.size() && IsSign( szVal[n] ) )
		++n;
	if ( n >= szVal.size() )
		return false;

	while ( n < szVal.size() && IsDecDigit( szVal[n] ) )
		++n;
	if ( n < szVal.size() )
		return false;

	return true;
}

bool IsFloat( const string &szVal )
{
	if ( szVal.empty() )
		return false;

	int n = 0;
	while ( n < szVal.size() && IsSign( szVal[n] ) )
		++n;
	if ( n >= szVal.size() )
		return false;

	while ( n < szVal.size() && IsDecDigit( szVal[n] ) )
		++n;

	if ( n >= szVal.size() )
		return true;

	if ( szVal[n] == '.' )
	{
		++n;
		while ( n < szVal.size() && IsDecDigit( szVal[n] ) )
			++n;

		if ( n >= szVal.size() )
			return true;
	}

	if ( szVal[n] != 'e' )
		return false;
	++n;

	if ( n >= szVal.size() )
		return false;

	if ( IsSign( szVal[n] ) )
	{
		++n;
		if ( n >= szVal.size() )
			return false;
	}

	while ( n < szVal.size() && IsDecDigit( szVal[n] ) )
		++n;

	if ( n >= szVal.size() )
		return true;

	if ( szVal[n] != 'f' )
		return false;

	++n;
	if ( n < szVal.size() )
		return false;

	return true;
}

}

