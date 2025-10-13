#include "stdafx.h"
#include "BinaryResources.h"
#include "GResource.h"
#include "VFSOperations.h"
#include "Misc/Win32Helper.h"

#include <cstdint>

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

inline std::string GetFileResourceUidName( const char *pszResName, const GUID &fileUID )
{
	if ( NBinResources::IsEmptyGUID( fileUID ) )
		return pszResName;
	char szBuf[1024];
	sprintf( szBuf, "bin\\%s\\%s", pszResName, NBinResources::GUIDToString( fileUID ).c_str() );
	return szBuf;
}

static inline bool DoesFileExist( const char *pszResName, int nID )
{
	return NVFS::GetMainVFS()->DoesFileExist( GetFileResourceName( pszResName, nID ) );
}

static inline bool DoesFileExist( const char *pszResName, const GUID &fileUID )
{
	if ( NBinResources::IsEmptyGUID( fileUID ) )
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

static void TypeReq( const char *pszResName, int nID )
{
	char szBuf[1024];
	sprintf( szBuf, "%s %x\n", pszResName, nID );
	OutputDebugString( szBuf );
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

static NWin32Helper::CCriticalSection readResource;
static bool bIsFileReading = false;
void CFileRequest::Read()
{
	ASSERT(!bIsReady);
	if ( bIsReady )
		return;
	NWin32Helper::CCriticalSectionLock l( readResource );
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

static NWin32Helper::CCriticalSection reqQueue, pendingCheck;
static NWin32Helper::CEvent newRequest;
static HANDLE hLoaderThread;
static std::list<CPtr<CFileRequest> > holdRequests;
static std::list<CFileRequest*> requests;

static unsigned long WINAPI LoaderThread( void* )
{
	for (;;)
	{
		newRequest.Wait();
		newRequest.Reset();
		// process new requests
		CFileRequest *pRes;
		for(;;)
		{
			NWin32Helper::CCriticalSectionLock lp( pendingCheck );
			{
				NWin32Helper::CCriticalSectionLock l( reqQueue );
				if ( requests.empty() )
					break;
				pRes = requests.front();
				requests.pop_front();
			}
			if ( pRes == 0 )
				return 0;
			if ( !IsValid(pRes) )
				continue;
			pRes->Read();
		}
	}
	return 0;
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
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	holdRequests.push_front( pReq );
	requests.push_front( pReq );
	newRequest.Set();
}

bool HasFileRequestsInFly()
{
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	return bIsFileReading || !requests.empty();
}

int CountFileRequestsInFly()
{
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	return requests.size();
}

void ReleaseFileRequestHolder()
{
	NWin32Helper::CCriticalSectionLock l( reqQueue );
	if ( requests.empty() )
		holdRequests.clear();
}

void __declspec(dllexport) SFLB3_RunResourceLoadingThread()
{
	unsigned long dwThread;
	hLoaderThread = CreateThread( 0, 102400, LoaderThread, 0, 0, &dwThread );
}

struct SKillLoaderThread
{
	~SKillLoaderThread()
	{
		if ( hLoaderThread )
		{
			{
				NWin32Helper::CCriticalSectionLock l( reqQueue );
				newRequest.Set();
				requests.push_front( 0 );
			}
			WaitForSingleObject( hLoaderThread, INFINITE );
		}
	}
} killLoaderThread;
}


