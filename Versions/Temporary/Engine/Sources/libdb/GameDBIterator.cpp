#include "stdafx.h"

#include "Db.h"
#include "ResourceManager.h"

namespace NDb
{

class CDBIterator : public IDBIterator
{
	OBJECT_BASIC_METHODS( CDBIterator )
	std::vector<CDBID> dbids;
	std::vector<CDBID>::const_iterator posCurrent;

	//
	CDBIterator() {}

public:
	CDBIterator( int nTypeID )
	{
		GetObjectsList( &dbids, nTypeID );
		posCurrent = dbids.empty() ? dbids.end() : dbids.begin();
	}

	bool MoveNext() override
	{
		if ( IsEnd() )
		{
			return false;
		}
		++posCurrent;
		return !IsEnd();
	}

	bool IsEnd() const override
	{
		return posCurrent == dbids.end();
	}

	CResource *Get() const override
	{
		return IsEnd() ? 0 : NDb::GetObject( *posCurrent );
	}

	void MoveFirst() override
	{
		posCurrent = dbids.empty() ? dbids.end() : dbids.begin();
	}
};

IDBIterator *CreateDBIterator( int nTypeID )
{
	return new CDBIterator( nTypeID );
}

} // namespace NDb
