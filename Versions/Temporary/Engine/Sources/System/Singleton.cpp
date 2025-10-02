#include "StdAfx.h"
#include "ConsoleBufferInternal.h"
#include "../Misc/HashFuncs.h"

namespace NSingleton
{

typedef hash_map< int, CObj<CObjectBase> > CObjectsMap;
static CObjectsMap objects;

struct SSingletonAutoMagic
{
	SSingletonAutoMagic()
	{
		RegisterSingleton( new CConsoleBuffer(), IConsoleBuffer::tidTypeID );
	}
	~SSingletonAutoMagic()
	{
		SSingletonAutoMagic::Clear();
	}
	static void Clear()
	{
		objects.clear();
	}
};
static SSingletonAutoMagic singletonautomagic;

CObjectBase *Singleton( const int nTypeID )
{
	CObjectsMap::iterator pos = objects.find( nTypeID );
	return pos != objects.end() ? pos->second : 0;
}

void RegisterSingleton( CObjectBase *pObj, const int nTypeID )
{
	NI_ASSERT( objects.find(nTypeID) == objects.end(), StrFmt("Object (%d) (\"%s\") already registered", nTypeID, typeid(*pObj).name()) );
	objects[nTypeID] = pObj;
}

void UnRegisterSingleton( const int nTypeID )
{
	objects.erase( nTypeID );
}

void DoneSingletons()
{
	SSingletonAutoMagic::Clear();
}

void GetAllSingletonIDs( vector<int> *pRes )
{
	pRes->resize( 0 );
	for ( CObjectsMap::iterator i = objects.begin(); i != objects.end(); ++i )
		pRes->push_back( i->first );
}

// here we are using a cheat to save singleton's data without re-creating singletons itself
struct SSingletonSerializer
{
	class CSingleObject;
	static hash_map<CObjectBase*, bool, SDefaultPtrHash> saved;

	class CSingleObject
	{
		int nID;
		CObjectBase *pObject;
	public:
		CSingleObject(): nID(0), pObject( 0 ) {}
		CSingleObject( int _nID, CObjectBase *_pObject ): nID(_nID), pObject( _pObject ) {}
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &nID );
			// find target singleton if reading
			if ( saver.IsReading() )
				pObject = Singleton( nID );
			if ( pObject && SSingletonSerializer::saved.find( pObject ) == SSingletonSerializer::saved.end() )
			{
				saver.AddPolymorphicBase( 2, pObject );
				SSingletonSerializer::saved[pObject];
			}
			return 0;
		}
	};
	int operator&( IBinSaver &saver )
	{
		vector<CSingleObject> singles;
		if ( saver.IsReading() )
			saver.Add( 1, &singles );
		else
		{
			for ( CObjectsMap::iterator it = objects.begin(); it != objects.end(); ++it )
				singles.push_back( CSingleObject( it->first, it->second.GetPtr() ) );
			saver.Add( 1, &singles );
		}
		saved.clear();
		return 0;
	}
};

hash_map<CObjectBase*, bool, SDefaultPtrHash> SSingletonSerializer::saved;

void Serialize( const char chunkID, IBinSaver &saver )
{
	SSingletonSerializer s;
	saver.Add( chunkID, &s );
}

}

