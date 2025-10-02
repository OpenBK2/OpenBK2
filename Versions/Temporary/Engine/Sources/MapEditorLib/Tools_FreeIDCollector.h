#if !defined(__COMMON_TOOLS__FREE_ID_COLLECTOR__)
#define __COMMON_TOOLS__FREE_ID_COLLECTOR__
#pragma once

/**
template<class TID>
class CFreeIDCollector
{
	struct SLockedIDNode
	{
		TID nFirst;
		TID nLast;
	}
	typedef list<SLockedIDNode> CLockedIDNodeList;

	CLockedIDNodeList lockedIDNodeList;
public:

	inline void Clear() { lockedIDNodeList.clear(); }
	bool IsIDLocked( const TID &rID );
	void LockID( TID *pID );
	void FreeID( const TID &rID );
	//
	virtual void GetNextID( TID *pNextID, const TID &rID ) = 0;
	virtual void GetFirstID( TID *pFirstID ) = 0;
};
/**/

class CFreeIDCollector
{
	struct SLockedIDNode
	{
		UINT nFirstID;
		UINT nLastID;
	};
	typedef list<SLockedIDNode> CLockedIDNodeList;

	CLockedIDNodeList lockedIDNodeList;

	inline UINT GetFirstID() { return 1; }
	inline UINT GetNextID( const UINT nID ) { return ( nID + 1 ); }
	inline UINT GetPreviousID( const UINT nID ) { return ( nID - 1 ); }
	bool FindLockedIDNode( CLockedIDNodeList::iterator *pItLockedIDNode, const UINT nID );
public:

	inline void Clear() { lockedIDNodeList.clear(); }
	inline bool IsIDLocked( const UINT nID ) { return FindLockedIDNode( 0, nID ); }
	UINT LockID();
	bool LockID( UINT nID );
	void FreeID( const UINT nID );
	//
	void Trace() const
	{
		DebugTrace( "free ID collector, begin" );
		for ( CLockedIDNodeList::const_iterator itLockedIDNode = lockedIDNodeList.begin(); itLockedIDNode != lockedIDNodeList.end(); ++itLockedIDNode )
		{
			DebugTrace( "[%d...%d]", itLockedIDNode->nFirstID, itLockedIDNode->nLastID );
		}
		DebugTrace( "free ID collector, end" );
	}
};

#endif // #if !defined(__COMMON_TOOLS__FREE_ID_COLLECTOR__)
