#include "stdafx.h"

#include "UnitsIterators.h"
#include "Misc/Bresenham.h"

#include <cstdint>

//*******************************************************************
//*													CGlobalIter															*
//*******************************************************************

void CGlobalIter::Init( const uint8_t cStartDipl, const uint8_t cFilter )
{
	nParties = 0;
	for ( int i = 0; i < 3; ++i )
	{
		if ( theDipl.GetDiplStatusForParties( cStartDipl, i ) & cFilter )
			parties[nParties++] = i;
	}

	nCurParty = 0; 
	if ( nParties == 0 )
		iter = 0;
	else
	{
		iter = units.units.begin( parties[nCurParty] );
		while ( nCurParty < nParties && ( iter == 0 || units.units.GetEl(iter) == 0 ) )
		{
			if ( iter != 0 )
				iter = units.units.GetNext( iter );
			else
			{
				++nCurParty;			

				if ( nCurParty < nParties )
					iter = units.units.begin( parties[nCurParty] );
			}
		}
	}
	
	visitedUnits.clear();
}

void CGlobalIter::Iterate()
{
	iter = units.units.GetNext( iter );

	// something went wrong
	if ( visitedUnits.find( iter ) != visitedUnits.end() )
		iter = 0;

	visitedUnits.insert( iter );

	while ( ( iter == 0 || units.units.GetEl(iter) == 0 ) && nCurParty < nParties )
	{
		if ( iter != 0 )
			iter = units.units.GetNext( iter );
		else
		{
			++nCurParty;

			if ( nCurParty < nParties )
				iter = units.units.begin( parties[nCurParty] );
		}
	}
}

CAIUnit* CGlobalIter::operator*() const 
{ 
	return units.units.GetEl(iter);
}

//*******************************************************************
//*													CPlanesIter															*
//*******************************************************************

CPlanesIter::CPlanesIter()
{
	iter = units.planes.begin();
	while ( iter != units.planes.end() && *iter == 0)
		++iter;
}

void CPlanesIter::Iterate()
{
 	++iter;
	while ( iter != units.planes.end() && *iter == 0 )
		++iter;
}

CAviation* CPlanesIter::operator*() const
{
	return *iter;
}

const bool CPlanesIter::IsFinished() const
{
	return iter == units.planes.end();
}


