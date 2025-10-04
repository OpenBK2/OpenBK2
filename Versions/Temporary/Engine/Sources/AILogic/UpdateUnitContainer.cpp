#include "stdafx.h"
#include "UpdateUnitContainer.h"

void CUpdateUnitContainer::Init( const float fMaxRadius )
{
	f2MaxRadius = 2*fMaxRadius;
	fMaxRadius2 = fMaxRadius/2;
	updateLists.resize( 6 );
}

void CUpdateUnitContainer::Clear()
{
	updateLists.clear();
}

void CUpdateUnitContainer::Push( const int nID, const EUpdateWarFogUnitInfoFlag updateFlag, const float fDist )
{
	int nNewLevel = -1;
	switch( updateFlag ) 
	{
	case UPD_DELETE_UNIT:
	case UPD_CREATE_NEW_UNIT:
	case UPD_CHANGE_PARTY:
		nNewLevel = 0;
		break;
	case UPD_UPDATE_VISIBILITY:
		nNewLevel = 1;
		break;
	case UPD_UPDATE_PROPERTIES:
		nNewLevel = 2;
	case UPD_UPDATE_POSITION:
		if ( fDist > f2MaxRadius )
			nNewLevel = 3;
		else if ( fDist > fMaxRadius2 )
			nNewLevel = 4;
		else if ( fDist > 0 )  
			nNewLevel = 5;
	}

	for ( int nLevel = 0; nLevel < updateLists.size(); ++nLevel )
	{
		CUpdateUnitList::iterator pos = updateLists[nLevel].find( nID );
		if ( pos != updateLists[nLevel].end() )
		{
			if ( nLevel < nNewLevel )
			{
				if ( updateFlag == UPD_UPDATE_POSITION )
					pos->second.fDist = fDist;
				return;
			}
			else if ( nLevel == nNewLevel )
			{
				if ( updateFlag == UPD_DELETE_UNIT && pos->second.updateFlag == UPD_CREATE_NEW_UNIT )
					updateLists[nLevel].erase( pos );
				else
				{
					if ( updateFlag == UPD_UPDATE_POSITION )
						pos->second.fDist = fDist;
					pos->second.updateFlag  = updateFlag;
				}
				return;
			}
			else
			{
				updateLists[nLevel].erase( pos );
				break;
			}
		}
	}
	if ( nNewLevel >= 0 )
		updateLists[nNewLevel][nID] = SUpdateUnitInfo( updateFlag, fDist );
}

int CUpdateUnitContainer::Pop()
{
	for ( int i = 0; i < updateLists.size(); ++i )
	{
		if ( !updateLists[i].empty() )
		{
			CUpdateUnitList::iterator pos = updateLists[i].begin();
			const int nResult = pos->first;
			updateLists[i].erase( pos );
			return nResult;
		}
	}
	return -1;
}

bool CUpdateUnitContainer::IsEmpty()
{
	for ( int i = 0; i < updateLists.size(); ++i )
	{
		if ( !updateLists[i].empty() )
			return false;
	}
	return true;
}



