#ifndef __BASICSHARE_H_
#define __BASICSHARE_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "System_export.h"


class CBasicShareBase
{
	SYSTEM_EXPORT friend void RegisterBasicShareBase( class CBasicShareBase *pBase );
	int nID;
	CBasicShareBase *pNext;
protected:
	int GetID() { return nID; }
	virtual void CreateHolder( list<CObj<CObjectBase> > *pHolder ) = 0;
public:
	CBasicShareBase( int _nID ): nID( _nID ) { RegisterBasicShareBase(this); }
	virtual int operator&( IBinSaver &f ) = 0;
	//
	SYSTEM_EXPORT friend void SerializeShared( IBinSaver *pFile );
	friend void CreateSharedHolder( class CSharedHolder *pHolder );
};

template <class TKey, class TValue, class THash = hash<TKey> >
class CBasicShare: public CBasicShareBase
{
public:
	typedef hash_map< TKey, CPtr<TValue>, THash > CDataHash;
private:
	bool bKeepData;
	CDataHash data;
	//
	virtual void CreateHolder( list<CObj<CObjectBase> > *pHolder )
	{
		for ( CDataHash::const_iterator i = data.begin(); i != data.end(); ++i )
			pHolder->push_back( i->second.GetPtr() );
	}
protected:
	virtual TValue* Create( const TKey &key ) { TValue *pRes = new TValue; pRes->SetKey(key); return pRes; }
public:
	CBasicShare( int nID, bool _bKeepData = true ): CBasicShareBase( nID ), bKeepData(_bKeepData) {}
	//DEBUG{
	void Clear() { data.clear(); }
	//DEBUG}
	TValue* Get( const TKey &key )
	{
		CDataHash::iterator i = data.find( key );
		if ( i == data.end() )
		{
			TValue *pRes = Create( key );
			data[key] = pRes;
			return pRes;
		}
		if ( !IsValid( i->second ) )
			i->second = Create( key );
		return i->second;
	}
	const CDataHash& GetAll() { return data; }
	int operator&( IBinSaver &f )
	{ 
		if ( !bKeepData )
			return 0;
		if ( f.IsReading() )
		{
			CDataHash keeper( data );
			f.Add( GetID(), &data ); 
			for ( CDataHash::const_iterator i = keeper.begin(); i != keeper.end(); ++i )
			{
				CDataHash::iterator r = data.find( i->first );
				if ( r != data.end() && IsValid( i->second ) )
					*r->second = *i->second;
			}
		}
		else
		{
			CDataHash saveData;
			for ( CDataHash::const_iterator i = data.begin(); i != data.end(); ++i )
			{
				if ( IsValid( i->second ) )
					saveData.insert( *i ); 
			}

			f.Add( GetID(), &saveData ); 
		}
		return 0;
	}
};

class CSharedHolder
{
	friend void CreateSharedHolder( CSharedHolder *pHolder );
	list<CObj<CObjectBase> > objs;
public:
	CSharedHolder() { CreateSharedHolder(this); }
};

SYSTEM_EXPORT void SerializeShared( IBinSaver *pFile );

#endif
