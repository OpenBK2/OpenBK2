#include "StdAfx.h"
#include "ChunklessSaver.h"

// CObjectBase

const char* CObjectBase::GetTypeName() const
{
	const char *pszName = typeid(*this).name(), *p;
	p = strstr( pszName, "::" );
	if ( !p )
		return pszName + 6;
	return p + 2;
}

const char* CObjectBase::GetFullTypeName() const
{
	return typeid(*this).name();
}

static bool bDestroyInProgress;
static list<CObjectBase*> *pDestroy, *pInvalidate;

inline list<CObjectBase*>& GetDestroy()
{
	if ( !pDestroy )
		pDestroy = new list<CObjectBase*>;
	return *pDestroy;
}

inline list<CObjectBase*>& GetInvalidate()
{
	if ( !pInvalidate )
		pInvalidate = new list<CObjectBase*>;
	return *pInvalidate;
}

static void FreeLists()
{
	if ( pDestroy && pDestroy->empty() )
	{
		delete pDestroy; 
		pDestroy = 0;
	}
	if ( pInvalidate && pInvalidate->empty() )
	{
		delete pInvalidate; 
		pInvalidate = 0;
	}
}

static struct STracker
{
	bool bIsRunning;
	STracker(): bIsRunning(true) {}
	~STracker() { bIsRunning = false; FreeLists(); }
} tracker;

void CObjectBase::ReleaseObj( int nRef, int nMask )
{
	nObjData -= nRef;
	if ( (nObjData & 0x7fffffff) == 0 && nRefData == 0 )
	{
		if ( bDestroyInProgress )
			GetDestroy().push_back( this );
		else
		{
			bDestroyInProgress = true;
			delete this;
			DestroyDelayed(); 
		}
	}
	else if ( (nObjData & nMask) == 0 )
	{
		nObjData |= 0x80000000;
		if ( bDestroyInProgress )
		{
			AddRef();
			GetInvalidate().push_back( this );
		}
		else
		{
			AddRef();
			bDestroyInProgress = true;
			DestroyContents(); 
			ReleaseRef();
			DestroyDelayed();
		}
	}
}

void CObjectBase::ReleaseRef()
{
	--nRefData;
	if ( nRefData == 0 && (nObjData & 0x7fffffff) == 0 )
	{
		if ( bDestroyInProgress )
			GetDestroy().push_back( this );
		else
		{
			bDestroyInProgress = true;
			delete this;
			DestroyDelayed();
		}
	}
}

void CObjectBase::DestroyDelayed()
{
	ASSERT( bDestroyInProgress );
	list<CObjectBase*> &toDestroy = GetDestroy();
	list<CObjectBase*> &toInvalidate = GetInvalidate();
	while ( !toDestroy.empty() || !toInvalidate.empty() )
	{
		while ( !toDestroy.empty() )
		{
			CObjectBase *pObj = toDestroy.front();
			toDestroy.pop_front();
			delete pObj;
		}
		while ( !toInvalidate.empty() )
		{
			CObjectBase *pObj = toInvalidate.front();
			toInvalidate.pop_front();
			pObj->DestroyContents(); 
			pObj->ReleaseRef();
		}
	}
	bDestroyInProgress = false;
	if ( !tracker.bIsRunning )
		FreeLists();		
}

#ifdef TRACK_OBJECTS_STATISTICS

struct SSimplePointerHash
{
	template <class T> 
		int operator()( T *p ) const { return (int)p; }
};

hash_set<CObjectBase *, SSimplePointerHash > *pObjectsSet; // do not initialize!

void RegisterInObjectsSet( CObjectBase *pObject )
{
	if ( pObjectsSet == 0 )
		pObjectsSet = new hash_set<CObjectBase *, SSimplePointerHash >;

	pObjectsSet->insert( pObject );
}

void UnRegisterInObjectsSet( CObjectBase *pObject )
{
	pObjectsSet->erase( pObject );
}

struct STypeStats
{
	int nObjectsNumber;
	int nValidsNumber;
	int nInvalidsNumber;
	int nSizeOf;
	int nSerializeSize;

	STypeStats() :
		nObjectsNumber( 0 ),
		nValidsNumber( 0 ),
		nInvalidsNumber( 0 ),
		nSizeOf( 0 ),
		nSerializeSize( 0 )
	{}
};

struct STypeStatsComparer
{
	bool operator()( const pair<string, STypeStats> &left, pair<string, STypeStats> &right ) const
	{
		return left.first < right.first;
	}
};

class CTrivialPointerSerialization: public IPointerSerialization
{
	OBJECT_BASIC_METHODS(CTrivialPointerSerialization);

	virtual int GetObjectID( CObjectBase *p )
	{
		return (int)p;
	}

	virtual CObjectBase *GetObject( int nID )
	{
		return (CObjectBase *)nID;
	}
};

static void CollectObjectsStatistics( hash_map< string, STypeStats > *pTypesMap )
{
	ASSERT( pTypesMap != 0 );
	hash_map< string, STypeStats > &typesMap = *pTypesMap;

	CMemoryStream memoryStream;
	int nStreamSize = memoryStream.GetSize();
	CPtr<CTrivialPointerSerialization> pPointerSerialization = new CTrivialPointerSerialization();
	CPtr<IBinSaver> pSaver = CreateChunklessSaver( pPointerSerialization, &memoryStream, SAVER_MODE_WRITE );

	for ( hash_set<CObjectBase *, SSimplePointerHash >::iterator it = pObjectsSet->begin(); it != pObjectsSet->end(); ++it )
	{
		CObjectBase *pObject = *it;
		const char *szTypeName = pObject->GetFullTypeName();
		STypeStats &typeStats = typesMap[szTypeName];

		++typeStats.nObjectsNumber;
		typeStats.nSizeOf = pObject->GetSizeOf();

		if ( IsValid( pObject ) )
			++typeStats.nValidsNumber;
		else
			++typeStats.nInvalidsNumber;

		pObject->operator&( *pSaver );
		int nNewStreamSize = memoryStream.GetSize();
		ASSERT( nNewStreamSize >= nStreamSize );
		typeStats.nSerializeSize += nNewStreamSize - nStreamSize;
		nStreamSize = nNewStreamSize;
	}
}

void PrintObjectsStatistics()
{
	hash_map< string, STypeStats > typesMap;
	CollectObjectsStatistics( &typesMap );

	vector< pair<string, STypeStats> > statistics;
	statistics.reserve( typesMap.size() );
	for ( hash_map< string, STypeStats >::iterator it = typesMap.begin(); it != typesMap.end(); ++it )
	{
		statistics.push_back( pair<string, STypeStats>( it->first.data(), it->second ) );
	}
	typesMap.clear();

	sort( statistics.begin(), statistics.end(), STypeStatsComparer() );

	char szBuf[1024];
	OutputDebugString( "type name\tobjects number\tvalid\tinvalid\tsizeof\tsizeof*number\tserialize size\n" );
	int nTotalObjects = 0;
	int nTotalValidsNumber = 0;
	int nTotalInvalidsNumber = 0;
	int nTotalSizeOf = 0;
	int nTotalSerializeSize = 0;
	for ( int i = 0; i < statistics.size(); ++i )
	{
		const pair<string, STypeStats> &tmp = statistics[i];
		const STypeStats &stats = tmp.second;
		const int nNumberXSizeOf = stats.nObjectsNumber*stats.nSizeOf;
		sprintf( szBuf, "%s\t%d\t%d\t%d\t%d\t%d\t%d\n", tmp.first.data(), stats.nObjectsNumber, stats.nValidsNumber, stats.nInvalidsNumber, stats.nSizeOf, nNumberXSizeOf, stats.nSerializeSize );
		OutputDebugString( szBuf );
		nTotalObjects += stats.nObjectsNumber;
		nTotalValidsNumber += stats.nValidsNumber;
		nTotalInvalidsNumber += stats.nInvalidsNumber;
		nTotalSizeOf += nNumberXSizeOf;
		nTotalSerializeSize += stats.nSerializeSize;
	}
	sprintf( szBuf, "total (%d types)\t%d\t%d\t%d\t%d\t\t%d\t%d\n", statistics.size(), nTotalObjects, nTotalValidsNumber, nTotalInvalidsNumber, nTotalSizeOf, nTotalSerializeSize );
	OutputDebugString( szBuf );
}

#endif // TRACK_OBJECTS_STATISTICS


