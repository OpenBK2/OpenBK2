#include "stdafx.h"

#include "FreeIDs.h"

#include "stdafx.h"

//*******************************************************************
//*															CFreeIds														*
//*******************************************************************

void CFreeIds::Init( const int nElements )
{
	nexts.clear(); nexts.resize( Max( nElements, 2 ), 0 );
	givenIDs.clear();

	firstEl = 1;
}

void CFreeIds::Clear()
{
	Init( NUM_OF_ELEMENTS );
}

void CFreeIds::Return( const int id )
{
	if ( givenIDs.find( id ) != givenIDs.end() )
	{
//		NI_ASSERT_T( id < nexts.size(), NStr::Format( "Wrong id was given (%d)", id ) );
		
		nexts[id] = firstEl;
		firstEl = id;
		
		givenIDs.erase( id );
	}
}

const int CFreeIds::Get()
{
	const int nResult = firstEl;
	givenIDs.insert( CGivenIDMap::value_type(nResult, true) );

//	NI_ASSERT_T( firstEl < nexts.size(), NStr::Format( "Wrong first element (%d)", firstEl ) );
	firstEl = nexts[firstEl] == 0 ? firstEl + 1 : nexts[firstEl];
	if ( firstEl >= nexts.size() )
		nexts.resize( firstEl * 2, 0 );

	return nResult;
}


