#include "stdafx.h"
#include "Tools_FreeIDCollector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../libdb/Manipulator.h"

bool CFreeIDCollector::FindLockedIDNode( CLockedIDNodeList::iterator *pItLockedIDNode, const UINT nID )
{
	if ( nID == INVALID_NODE_ID )
	{
		return false;
	}
	for ( CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.begin(); itLockedIDNode != lockedIDNodeList.end(); ++itLockedIDNode )
	{
		if ( ( itLockedIDNode->nFirstID <= nID ) && ( nID <= itLockedIDNode->nLastID ) )
		{
			if ( pItLockedIDNode != 0 )
			{
				*( pItLockedIDNode ) = itLockedIDNode;
			}
			return true;
		}
	}
	return false;
}


UINT CFreeIDCollector::LockID()
{
	if ( lockedIDNodeList.empty() )
	{
		UINT nLockID = GetFirstID();
		CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( lockedIDNodeList.end(), SLockedIDNode() );
		itLockedIDNode->nFirstID = nLockID;
		itLockedIDNode->nLastID = nLockID;
		return nLockID;
	}
	else
	{
		CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.begin();
		if ( itLockedIDNode->nFirstID > GetFirstID() )
		{
			if ( LockID( GetFirstID() ) )
			{
				return GetFirstID();
			}
		}
		UINT nLockID = GetNextID( itLockedIDNode->nLastID );
		itLockedIDNode->nLastID = nLockID;
		//
		CLockedIDNodeList::iterator itNextLockedIDNode = itLockedIDNode;
		++itNextLockedIDNode;
		if ( itNextLockedIDNode != lockedIDNodeList.end() )
		{
			if ( itNextLockedIDNode->nFirstID == GetNextID( itLockedIDNode->nLastID ) )
			{
				itNextLockedIDNode->nFirstID = itLockedIDNode->nFirstID;
				lockedIDNodeList.erase( itLockedIDNode );
			}
		}
		return nLockID;
	}
}	


bool CFreeIDCollector::LockID( UINT nID )
{
	if ( ( nID == INVALID_NODE_ID ) || ( nID < GetFirstID() ) )
	{
		return false;
	}
	if ( lockedIDNodeList.empty() )
	{
		CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( lockedIDNodeList.end(), SLockedIDNode() );
		itLockedIDNode->nFirstID = nID;
		itLockedIDNode->nLastID = nID;
		return true;
	}
	//
	CLockedIDNodeList::iterator itNextLockedIDNode = lockedIDNodeList.end();
	for ( CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.begin(); itLockedIDNode != lockedIDNodeList.end(); ++itLockedIDNode )
	{
		if ( nID < itLockedIDNode->nFirstID )
		{
			itNextLockedIDNode = itLockedIDNode;
			break;
		}
		if ( ( itLockedIDNode->nFirstID <= nID ) && ( nID <= itLockedIDNode->nLastID ) )
		{
			return false;
		}
	}
	//
	if ( itNextLockedIDNode == lockedIDNodeList.begin() )
	{
		if ( GetPreviousID( lockedIDNodeList.front().nFirstID ) == nID )
		{
			lockedIDNodeList.front().nFirstID = nID;
		}
		else
		{
			CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( lockedIDNodeList.begin(), SLockedIDNode() );
			itLockedIDNode->nFirstID = nID;
			itLockedIDNode->nLastID = nID;
		}
		return true;
	}
	//
	if ( itNextLockedIDNode == lockedIDNodeList.end() )
	{
		if ( GetNextID( lockedIDNodeList.back().nLastID ) == nID )
		{
			lockedIDNodeList.back().nLastID = nID;
		}
		else
		{
			CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( lockedIDNodeList.end(), SLockedIDNode() );
			itLockedIDNode->nFirstID = nID;
			itLockedIDNode->nLastID = nID;
		}
		return true;
	}
	//
	CLockedIDNodeList::iterator itPreviousLockedIDNode = itNextLockedIDNode;
	--itPreviousLockedIDNode;
	if ( GetNextID( itPreviousLockedIDNode->nLastID ) == GetPreviousID( itNextLockedIDNode->nFirstID ) )
	{
		itPreviousLockedIDNode->nLastID = itNextLockedIDNode->nLastID;
		lockedIDNodeList.erase( itNextLockedIDNode );
	}
	else if ( GetNextID( itPreviousLockedIDNode->nLastID ) == nID )
	{
		itPreviousLockedIDNode->nLastID = nID;
	}
	else if ( GetPreviousID( itNextLockedIDNode->nFirstID ) == nID )
	{
		itNextLockedIDNode->nFirstID = nID;
	}
	else
	{
		CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( itNextLockedIDNode, SLockedIDNode() );
		itLockedIDNode->nFirstID = nID;
		itLockedIDNode->nLastID = nID;
	}
	return true;
}


void CFreeIDCollector::FreeID( const UINT nID )
{
	if ( nID == INVALID_NODE_ID )
	{
		return;
	}
	CLockedIDNodeList::iterator posLockedIDNode = lockedIDNodeList.end();
	if ( FindLockedIDNode( &posLockedIDNode, nID ) && ( posLockedIDNode != lockedIDNodeList.end() ) )
	{
		if ( posLockedIDNode->nFirstID == posLockedIDNode->nLastID )
		{
			lockedIDNodeList.erase( posLockedIDNode );
		}
		else if ( nID == posLockedIDNode->nFirstID )
		{
			posLockedIDNode->nFirstID = GetNextID( posLockedIDNode->nFirstID );
		}
		else if ( nID == posLockedIDNode->nLastID )
		{
			posLockedIDNode->nLastID = GetPreviousID( posLockedIDNode->nLastID );
		}
		else
		{
			CLockedIDNodeList::iterator itLockedIDNode = lockedIDNodeList.insert( posLockedIDNode, SLockedIDNode() );
			itLockedIDNode->nFirstID = posLockedIDNode->nFirstID;
			itLockedIDNode->nLastID = GetPreviousID( nID );
			posLockedIDNode->nFirstID = GetNextID( nID );
		}
	}
}

// basement storage  

