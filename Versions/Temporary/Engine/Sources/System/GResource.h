#pragma once

#include "System_export.h"

#include "Dg.h"

#include <boost/uuid/uuid.hpp>

#include "UuidChunk.h"

#include <thread>

namespace NGScene
{

template<class TKey>
struct SResKey
{
	boost::uuids::uuid uidKey;
	TKey tKey;

	SResKey() {}
	SResKey( const boost::uuids::uuid &uid, const TKey &key ): uidKey(uid), tKey(key) {}
	SResKey( const TKey &key ): tKey(key) { Zero(uidKey); }   // TODO: create a uid field in NDb resources and remove this constructor

	int operator&( IBinSaver &f ) { f.Add( 1, &tKey ); AddUuidChunk( f, 2, &uidKey ); return 0; }

	bool operator==( const SResKey<TKey> &a ) const { return uidKey == a.uidKey && tKey == a.tKey; }
	int operator()( const SResKey<TKey> &k ) const { return k.tKey.nID; }
};

template <class TKey, class TValue>
class CResourceLoader: public CHoldedPtrFuncBase<TValue>
{
	SResKey<TKey> key;
protected:
	const SResKey<TKey>& GetKey() const { return key; }
public:
	CResourceLoader() {}
	void SetKey( const TKey &_key, const boost::uuids::uuid &uid ) { key = SResKey<TKey>( uid, _key ); }
	void SetKey( const SResKey<TKey> &_key ) { key = _key; }
	int operator&( IBinSaver &f ) { f.Add( 1, &key ); return 0; }
};

class IPrecache : public CObjectBase
{
public:
	virtual bool TryUpdate() = 0;
	virtual void ForceUpdate() = 0;
};
//extern vector<CPtr<IPrecache> > precacheUpdateList;

template<class T>
class CResourcePrecache : public IPrecache
{
	OBJECT_NOCOPY_METHODS(CResourcePrecache);
	CDGPtr<T> pStuff;
	int operator&( IBinSaver &f ) 
	{ 
		f.Add(2,&pStuff); 
		if ( f.IsReading() )
			AddToPrecachedUpdate( this );
			//precacheUpdateList.push_back( this );
		return 0; 
	}
public:
	CResourcePrecache( T *_pStuff = 0 ) : pStuff(_pStuff) { TryUpdate(); }
	~CResourcePrecache() { TryUpdate(); }
	bool TryUpdate() 
	{
		bool bRes = true;
		if ( IsValid(pStuff) ) 
		{ 
			pStuff.Refresh();
			bRes = pStuff->GetValue() != 0;
		}
		return bRes;
	}
	void ForceUpdate()
	{
		if ( IsValid(pStuff) ) 
		{ 
			pStuff.Refresh();
			while ( !pStuff->GetValue() )
				std::this_thread::yield();
		}
	}
};

class CFileResource
{
	CFileStream f;
public:
	CFileResource( const char *pszResName, int nFileID );
	CFileResource( const char *pszResName, const SResKey<int> &key );
	CFileStream& GetFileStream() { return f; }

	bool IsOk() const { return f.IsOk(); }
};

// for use in CResourceOpener
class SYSTEM_EXPORT CResourceFileOpener
{
public:
	static bool DoesExist( const char *pszResName, const SResKey<int> &key );
	static void Clear();
	CFileResource f;
public:
	CResourceFileOpener( const char *pszResName, const SResKey<int> &key ) : f( pszResName, key ) { }
	CFileStream& GetFileStream() { return f.GetFileStream(); }

	bool IsOk() const { return f.IsOk(); }
};

class CResourceOpener
{
	CResourceFileOpener file;
	CObj<IBinSaver> pSaver;
public:
	CResourceOpener( const char *pszResName, const SResKey<int> &key ) : file( pszResName, key )
	{
		if ( file.IsOk() )
			pSaver = CreateBinSaver( &file.GetFileStream(), SAVER_MODE_READ );
	}
	IBinSaver* operator->() { return pSaver; }

	bool IsOk() const { return file.IsOk(); }
};

// if for some package CFileRequest scheme is used all access to that resource should be
// through CFileRequest system
class SYSTEM_EXPORT CFileRequest : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CFileRequest);
	const char *pszResName;
	int  nID;
	boost::uuids::uuid uid;
	bool bIsReady, bDelayedLoad;
	CMemoryStream data;
public:
	CFileRequest() : pszResName(0), nID(0), bIsReady(true), bDelayedLoad(true) {}
	CFileRequest( const char *pszResName, int nID, bool bDelayedLoad );
	CFileRequest( const char *pszResName, const SResKey<int> &key, bool bDelayedLoad );
	//CFileRequest( const char *pszResName, const SPartKey &key, bDelayedLoad(true) );
	CMemoryStream* operator->() { return &data; }
	CMemoryStream* GetStream() { return &data; }
	void Read();
	bool IsReady() const { return bIsReady; }
	bool IsDelayedLoad() const { return bDelayedLoad; }
};
inline CFileRequest* CreateFileRequiest( const char *pszResName, int nID, bool bDelayedLoad = true ) { return new CFileRequest( pszResName, nID, bDelayedLoad ); }
inline CFileRequest* CreateFileRequiest( const char *pszResName, const SResKey<int> &key, bool bDelayedLoad = true ) { return new CFileRequest( pszResName, key, bDelayedLoad ); }
//inline CFileRequest* CreateFileRequiest( const char *pszResName, const SPartKey &key, bool bDelayedLoad = true ) { return new CFileRequest( pszResName, key, bDelayLoad ); }

SYSTEM_EXPORT void AddToPrecachedUpdate( IPrecache *pAdd );
//void AddResourceDir( const char *pszName );
//void ClearResourceDirs();
//void CloseAllResources();
SYSTEM_EXPORT void SFLB3_RunResourceLoadingThread();
SYSTEM_EXPORT void ReleaseFileRequestHolder();
SYSTEM_EXPORT void AddFileRequest( NGScene::CFileRequest *pReq );
SYSTEM_EXPORT bool HasFileRequestsInFly();
SYSTEM_EXPORT int CountFileRequestsInFly();
SYSTEM_EXPORT void LoadPrecached();

// Below CFileRequest and the free functions rather than above them: this template
// calls ReleaseFileRequestHolder and AddFileRequest and dereferences a
// CObj<CFileRequest>, none of which depend on its parameters, so they are looked up
// where the template is written and not where it is instantiated. MSVC parses the
// body late and never noticed. It used to sit directly under CResourceLoader with a
// forward declaration of CFileRequest for company, which is not enough to call a
// member on one.
template <class TKey, class TValue>
class CLazyResourceLoader : public CResourceLoader<TKey,TValue>
{
	typedef CResourceLoader<TKey,TValue> TParent;
	CObj<CFileRequest> pRequest;
protected:
	virtual CFileRequest* CreateRequest() = 0;
	virtual void RecalcValue( CFileRequest *p ) = 0;
	virtual void Recalc()
	{
		if ( IsValid(pRequest) )
		{
			if ( !pRequest->IsReady() )
				return;
			RecalcValue( pRequest );
			pRequest = 0;
			ReleaseFileRequestHolder();
		}
		else
		{
			pRequest = CreateRequest();
			if ( pRequest )
				AddFileRequest( pRequest );
		}
	}
	bool NeedUpdate() { TParent::NeedUpdate(); if ( !IsValid(this->pValue) && IsValid(pRequest) && pRequest->IsReady() ) return true; return false; }
public:
};
}

typedef NGScene::SResKey<int> SIntResKey;

template<> struct std::hash<SIntResKey>
{
	size_t operator()(const SIntResKey &key ) const { return key.tKey; }
};

template<class T>
SIntResKey GetIntResKey( T *pResource ) { return SIntResKey( pResource->uid, pResource->GetRecordID() ); }
