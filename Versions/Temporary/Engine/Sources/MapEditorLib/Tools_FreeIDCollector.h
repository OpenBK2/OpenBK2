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
	typedef std::list<SLockedIDNode> CLockedIDNodeList;

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
		unsigned nFirstID;
		unsigned nLastID;
	};
	typedef std::list<SLockedIDNode> CLockedIDNodeList;

	CLockedIDNodeList lockedIDNodeList;

	inline unsigned GetFirstID() { return 1; }
	inline unsigned GetNextID( const unsigned nID ) { return ( nID + 1 ); }
	inline unsigned GetPreviousID( const unsigned nID ) { return ( nID - 1 ); }
	bool FindLockedIDNode( CLockedIDNodeList::iterator *pItLockedIDNode, const unsigned nID );
public:

	inline void Clear() { lockedIDNodeList.clear(); }
	inline bool IsIDLocked( const unsigned nID ) { return FindLockedIDNode( 0, nID ); }
	unsigned LockID();
	bool LockID( unsigned nID );
	void FreeID( const unsigned nID );
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


