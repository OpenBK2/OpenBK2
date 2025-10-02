#pragma once

#include "UpdatableObject.h"

class CObjectBase * GetObjectByCmd( const struct SAIUnitCmd &cmd );

class CLinkObject : public CUpdatableObj
{
public: 
	ZDATA_( CUpdatableObj )
		int nLink;
		int nUniqueID;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CUpdatableObj *)this); f.Add(2,&nLink); f.Add(3,&nUniqueID); return 0; }
public:
	CLinkObject();
	CLinkObject( const int _nLink ) { SetLink( _nLink ); }
	virtual ~CLinkObject();
	
	void SetUniqueIdForObjects();
	void SetUniqueIdForUnits( const int nUniqueID );
	void SetLink( const int _nLink );
	const int GetLink() const { return nLink; }
	// запомнит ли объект в unitsID2Object
	void Mem2UniqueIdObjs();
	const int GetUniqueId() const { return nUniqueID; }

	static void Clear();
	static void ClearLinks();
	static CLinkObject* GetObjectByLink( const int nLink );
	static void Segment();
	// падает, если передан некорректный nUniqueID
	static CLinkObject* GetObjectByUniqueId( const int nUniqueID );
	
	// возвращает 0, если передан некорректный nUniqueID	
	static CLinkObject* GetObjectByUniqueIdSafe( const int nUniqueID );
	
	//
	static bool IsLinkObjectExists( const int nUniqueID );
	
	// даёт nSize свободных линков
	static void GetFreeLinks( list<int> *pLinks, const int nSize );
	
	// for Saving/Loading of static members
	friend class CStaticMembers;
};

// возвращает 0, если передан некорректный nUniqueID	
template<class T>
inline T* GetObjectByUniqueIdSafe( const int nUniqueID )
{
	CLinkObject *pLinkObject = CLinkObject::GetObjectByUniqueIdSafe( nUniqueID );
	return 
		pLinkObject ? checked_cast<T*>( pLinkObject ) : 0;
}

struct SLinkObjData
{
	vector< CObj<CLinkObject> > link2object;
	list<int> deletedObjects;
	list<int> deletedUniqueObjects;

	hash_map< int, CObj<CLinkObject> > unitsID2object;
	int nCurUniqueID;
	SLinkObjData() : nCurUniqueID( 0 ) {  }
};

struct SLinkObjDataAutoMagic
{
	static SLinkObjData * pLinkObjData;
	SLinkObjDataAutoMagic()
	{
		pLinkObjData = new SLinkObjData;
	}
	~SLinkObjDataAutoMagic()
	{
		delete pLinkObjData;
		pLinkObjData = 0;
	}
};

