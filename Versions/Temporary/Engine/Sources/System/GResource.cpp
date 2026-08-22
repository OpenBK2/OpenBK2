#include "stdafx.h"
#include "BinaryResources.h"
#include "GResource.h"
#include "VFSOperations.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NGScene
{

std::vector<CPtr<IPrecache> > precacheUpdateList;
void LoadPrecached()
{
	for ( int k = 0; k < precacheUpdateList.size(); ++k )
	{
		IPrecache *p = precacheUpdateList[k];
		if ( IsValid(p) )
			p->ForceUpdate();
	}
	precacheUpdateList.clear();
}

void AddToPrecachedUpdate( IPrecache *pAdd )
{
	precacheUpdateList.push_back( pAdd );
}

inline std::string GetFileResourceName( const char *pszResName, int nFileID )
{
	if ( nFileID == 0 )
		return pszResName;
	char szBuf[1024];
	sprintf( szBuf, "bin\\%s\\%d", pszResName, nFileID );
	return szBuf;
}

inline std::string GetFileResourceUidName( const char *pszResName, const boost::uuids::uuid &fileUID )
{
	if (fileUID.is_nil())
		return pszResName;
	return fmt::format("bin\\{}\\{}", pszResName, boost::uuids::to_string(fileUID));
}

static inline bool DoesFileExist( const char *pszResName, int nID )
{
	return NVFS::GetMainVFS()->DoesFileExist( GetFileResourceName( pszResName, nID ) );
}

static inline bool DoesFileExist( const char *pszResName, const boost::uuids::uuid &fileUID )
{
	if (fileUID.is_nil())
		return false;

	return NVFS::GetMainVFS()->DoesFileExist( GetFileResourceUidName( pszResName, fileUID ) );
}

// CFileResource

CFileResource::CFileResource( const char *pszResName, int nFileID )
: f( NVFS::GetMainVFS(), GetFileResourceName( pszResName, nFileID ) )
{
}

CFileResource::CFileResource( const char *pszResName, const SResKey<int> &key )
: f(	NVFS::GetMainVFS(),
			DoesFileExist( pszResName, key.uidKey ) ? 
				GetFileResourceUidName( pszResName, key.uidKey ) :
				GetFileResourceName( pszResName, key.tKey )
	 )
{
}

struct SDoesExistKey
{
	std::string szRes;
	SResKey<int> key;

	SDoesExistKey() {}
	SDoesExistKey( const char *_pszRes, const SResKey<int> &_k ) : szRes(_pszRes), key(_k) {}
};
inline bool operator ==( const SDoesExistKey &a, const SDoesExistKey &b ) { return a.szRes == b.szRes && a.key == b.key; }
struct SDoesExistKeyHash
{
	int operator()( const SDoesExistKey &a ) const { return std::hash<std::string>()( a.szRes ) ^ std::hash<SResKey<int> >()( a.key ); }
};
typedef std::unordered_map<SDoesExistKey, bool, SDoesExistKeyHash> TDoesExistHash;
static TDoesExistHash deh;
bool CResourceFileOpener::DoesExist( const char *pszResName, const SResKey<int> &key )
{
	SDoesExistKey k( pszResName, key );
	TDoesExistHash::iterator i = deh.find( k );
	if ( i != deh.end() )
		return i->second;
	bool bRes = DoesFileExist( pszResName, key.uidKey );
	if ( !bRes )
		bRes = DoesFileExist( pszResName, key.tKey );
	deh[ k ] = bRes;
	return bRes;
}

void CResourceFileOpener::Clear()
{
	deh.clear();
}


CFileRequest::CFileRequest( const char *_pszResName, int _nID, bool _bDelayedLoad ) 
	: pszResName(_pszResName), nID(_nID), bIsReady(false), bDelayedLoad(_bDelayedLoad)
{
	Zero( uid );
}

CFileRequest::CFileRequest( const char *_pszResName, const SResKey<int> &key, bool _bDelayedLoad )
: pszResName(_pszResName), nID(key.tKey), uid(key.uidKey), bIsReady(false), bDelayedLoad(_bDelayedLoad)
{
}

static std::mutex readResource;
static bool bIsFileReading = false;
void CFileRequest::Read()
{
	ASSERT(!bIsReady);
	if ( bIsReady )
		return;
	std::lock_guard l( readResource );
	bIsFileReading = true;
	const std::string szResourceName = DoesFileExist( pszResName, uid ) ? GetFileResourceUidName( pszResName, uid ) : GetFileResourceName( pszResName, nID );
	
	CFileStream f( NVFS::GetMainVFS(), szResourceName );
	if ( f.IsOk() )
	{
		f.ReadTo( &data, f.GetSize() );
		data.Seek(0);
	}

	bIsFileReading = false;
	bIsReady = true;
}

// Resource loading thread

static std::mutex reqQueue;
static std::condition_variable newRequest;
static std::thread loaderThread;
static std::list<CPtr<CFileRequest> > holdRequests;
static std::list<CFileRequest*> requests;

static void LoaderThread()
{
	for (;;)
	{
		CFileRequest *pRes = 0;
		{
			// the manual-reset event this replaces was signalled on every push
			// and cleared here, so an arrival between the wait and the reset
			// could be missed. Waiting on the queue itself cannot lose one.
			std::unique_lock l( reqQueue );
			newRequest.wait( l, [] { return !requests.empty(); } );
			pRes = requests.front();
			requests.pop_front();
		}
		// a null request means stop, and ~SKillLoaderThread puts it at the
		// front so shutdown does not wait for the queue to drain
		if ( pRes == 0 )
			return;
		if ( !IsValid(pRes) )
			continue;
		pRes->Read();
	}
}

void AddFileRequest( NGScene::CFileRequest *pReq )
{
	if ( pReq->IsReady() )
		return;
	if ( !pReq->IsDelayedLoad() )
	{
		pReq->Read();
		return;
	}
	{
		std::lock_guard l( reqQueue );
		holdRequests.push_front( pReq );
		// newest first: the request made last is the one being waited on
		requests.push_front( pReq );
	}
	newRequest.notify_one();
}

bool HasFileRequestsInFly()
{
	std::lock_guard l( reqQueue );
	return bIsFileReading || !requests.empty();
}

int CountFileRequestsInFly()
{
	std::lock_guard l( reqQueue );
	return requests.size();
}

void ReleaseFileRequestHolder()
{
	std::lock_guard l( reqQueue );
	if ( requests.empty() )
		holdRequests.clear();
}

void SFLB3_RunResourceLoadingThread()
{
	// assigning over a running std::thread calls std::terminate, where
	// CreateThread would only have leaked the old handle
	if ( loaderThread.joinable() )
		return;
	loaderThread = std::thread( LoaderThread );
}

struct SKillLoaderThread
{
	~SKillLoaderThread()
	{
		if ( !loaderThread.joinable() )
			return;
		{
			std::lock_guard l( reqQueue );
			requests.push_front( 0 );
		}
		newRequest.notify_one();
		loaderThread.join();
	}
} killLoaderThread;
}


