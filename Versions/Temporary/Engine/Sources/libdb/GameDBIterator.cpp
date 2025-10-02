#include "StdAfx.h"
#include "ResourceManager.h"
#include "Db.h"

namespace NDb
{

class CDBIterator : public IDBIterator
{
	OBJECT_BASIC_METHODS( CDBIterator )
	vector<CDBID> dbids;
	vector<CDBID>::const_iterator posCurrent;
	//
	CDBIterator() {}
public:
	CDBIterator( int nTypeID )
	{
		GetObjectsList( &dbids, nTypeID );
		posCurrent = dbids.empty() ? dbids.end() : dbids.begin();
	}

	bool MoveNext()
	{
		if ( IsEnd() )
			return false;
		++posCurrent;
		return !IsEnd();
	}

	bool IsEnd() const
	{
		return posCurrent == dbids.end();
	}

	CResource *Get() const
	{
		return IsEnd() ? 0 : NDb::GetObject( *posCurrent );
	}

	void MoveFirst()
	{
		posCurrent = dbids.empty() ? dbids.end() : dbids.begin();
	}
};

IDBIterator *CreateDBIterator( int nTypeID )
{
	return new CDBIterator( nTypeID );
}

}
