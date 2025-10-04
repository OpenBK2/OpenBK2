#include "stdafx.h"
#include "libdb/DBObserver.h"
#include "System/VFSOperations.h"

namespace NDb
{

class CVFSDbObserver : public IDbObserver
{
	OBJECT_BASIC_METHODS( CVFSDbObserver );
	//
	typedef list<CDBID> CObjectsList;
	CObjectsList objects2remove;
public:
	void ObjectChanged( const CDBID &dbid ) {}
	void ObjectAdded( const CDBID &dbid ) 
	{
		// remove from 'objects2remove' newly added object
		for ( CObjectsList::iterator it = objects2remove.begin(); it != objects2remove.end(); ++it )
		{
			if ( *it == dbid )
			{
				objects2remove.erase( it );
				break;
			}
		}
	}
	void ObjectRemoved( const CDBID &dbid ) { objects2remove.push_back( dbid ); }
	void ObjectMoved( const CDBID &dbidSrc, const CDBID &dbidDst )
	{
		ObjectRemoved( dbidSrc );
		ObjectAdded( dbidDst );
	}
	void DiscardAllChanges() { objects2remove.clear(); }
	void SaveAllChanges()
	{
		// delete all 'objects2remove' from disk
		if ( NVFS::IFileCreator *pCreator = NVFS::GetMainFileCreator() )
		{
			for ( CObjectsList::const_iterator it = objects2remove.begin(); it != objects2remove.end(); ++it )
				pCreator->RemoveFile( NDb::GetFileName(*it) );
		}
		objects2remove.clear();
	}
};

IDbObserver *CreateVFSDbObserver()
{
	return new CVFSDbObserver();
}

}
