#include "stdafx.h"

#include "UpdatableList.h"

#define MAX_CHANGES 8192
#define MASK (MAX_CHANGES - 1)

CUpdatableList::CUpdatableList()
: dwVersion( 0 ), changes( MAX_CHANGES )
{
}

void CUpdatableList::Add( const int nID )
{
	++dwVersion;

	changes[dwVersion & MASK].nID = nID;
	changes[dwVersion & MASK].change = EC_ADDED;

	now.insert( nID );
}

void CUpdatableList::Remove( const int nID )
{
	++dwVersion;

	changes[dwVersion & MASK].nID = nID;
	changes[dwVersion & MASK].change = EC_REMOVED;

	now.erase( nID );
}

void CUpdatableList::Change( const int nID )
{
	++dwVersion;

	changes[dwVersion & MASK].nID = nID;
	changes[dwVersion & MASK].change = EC_CHANGED;
}

void CUpdatableList::GetFullUpdate( const int nNoIncludeID, SUpdateInfo *pUpdate )
{
	pUpdate->Clear();
	pUpdate->dwVersion = dwVersion;
	pUpdate->bFullUpdate = true;

	for ( std::unordered_set<int>::iterator iter = now.begin(); iter != now.end(); ++iter )
	{
		if ( *iter != nNoIncludeID )
			pUpdate->added.push_back( *iter );
	}
}

void CUpdatableList::GetUpdate( const int nNoIncludeID, const DWORD dwOldVersion, SUpdateInfo *pUpdate )
{
	pUpdate->Clear();
	pUpdate->dwVersion = dwVersion;
	pUpdate->bFullUpdate = false;

	std::unordered_set<int> added, changed;

	const DWORD dwStart = ( dwOldVersion + 1 ) & MASK;
	const DWORD dwFinish = ( dwVersion + 1 ) & MASK;
	DWORD dwNow = dwStart;
	while ( dwNow != dwFinish )
	{
		if ( changes[dwNow].change != EC_NOP && changes[dwNow].nID != nNoIncludeID )
		{
			const int nID = changes[dwNow].nID;
			switch ( changes[dwNow].change )
			{
			case EC_ADDED:
				added.insert( nID );
				changed.erase( nID );

				break;
			case EC_REMOVED:
				{
					std::unordered_set<int>::iterator iter = added.find( nID );
					if ( iter == added.end() )
						pUpdate->removed.push_back( nID );
					else
						added.erase( iter );

					changed.erase( nID );
				}
				break;
			case EC_CHANGED:
				if ( added.find( nID ) == added.end() )
					changed.insert( nID );

				break;
			}
		}

		dwNow = (dwNow + 1) & MASK;
	}

	for ( std::unordered_set<int>::iterator iter = added.begin(); iter != added.end(); ++iter )
		pUpdate->added.push_back( *iter );
	for ( std::unordered_set<int>::iterator iter = changed.begin(); iter != changed.end(); ++iter )
		pUpdate->changed.push_back( *iter );
}

void CUpdatableList::GetDiff( const int nNoIncludeID, const DWORD dwOldVersion, SUpdateInfo *pUpdate )
{
	const DWORD dwDiff = dwVersion - dwOldVersion;

	if ( dwDiff > changes.size() || dwOldVersion == 0 )
		GetFullUpdate( nNoIncludeID, pUpdate );
	else
	{
		GetUpdate( nNoIncludeID, dwOldVersion, pUpdate );

		if ( pUpdate->added.size() + pUpdate->changed.size() >= now.size() )
			GetFullUpdate( nNoIncludeID, pUpdate );
	}
}


