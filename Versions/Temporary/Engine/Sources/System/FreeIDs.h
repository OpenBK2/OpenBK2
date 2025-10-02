#ifndef __FREEIDS_H__
#define __FREEIDS_H__

#pragma ONCE


#include "System_export.h"

//*******************************************************************
//*											Free Identifiers														*
//*******************************************************************

class SYSTEM_EXPORT CFreeIds
{
	enum { NUM_OF_ELEMENTS = 200 };

	typedef hash_map<int, bool> CGivenIDMap;
	CGivenIDMap givenIDs;
	
	vector<int> nexts;
	int firstEl;
public:
	CFreeIds( const int nElements = NUM_OF_ELEMENTS ) { Init( nElements ); }

	void Init( const int nElements );
	void Clear();
	
	const int Get();
	void Return( const int id );

	inline int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &givenIDs );
		saver.Add( 2, &nexts );
		saver.Add( 3, &firstEl );
		return 0;
	}
	
};

#endif // __FREEIDS_H__
