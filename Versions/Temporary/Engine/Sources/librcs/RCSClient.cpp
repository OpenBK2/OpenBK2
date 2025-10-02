#include "stdafx.h"
#include "RCSClient.h"

#include <svn_client.h>
#include <svn_pools.h>

//#pragma comment (lib, "intl3_svn.lib")
//#pragma comment (lib, "libapr.lib")
//#pragma comment (lib, "libapriconv.lib")
//#pragma comment (lib, "libaprutil.lib")
//#pragma comment (lib, "libneon.lib")
//#pragma comment (lib, "libsvn_client-1.lib")
//#pragma comment (lib, "libsvn_delta-1.lib")
//#pragma comment (lib, "libsvn_diff-1.lib")
//#pragma comment (lib, "libsvn_fs-1.lib")
////pragma comment (lib, "libsvn_fs_base-1.lib")
//#pragma comment (lib, "libsvn_fs_fs-1.lib")
//#pragma comment (lib, "libsvn_ra-1.lib")
//#pragma comment (lib, "libsvn_ra_dav-1.lib")
//#pragma comment (lib, "libsvn_ra_local-1.lib")
//#pragma comment (lib, "libsvn_ra_svn-1.lib")
//#pragma comment (lib, "libsvn_repos-1.lib")
//#pragma comment (lib, "libsvn_subr-1.lib")
//#pragma comment (lib, "libsvn_wc-1.lib")
//#pragma comment (lib, "xml.lib")
//#pragma comment (lib, "zlibstat.lib")
//#pragma comment (lib, "wsock32.lib")

extern "C" void svn_fs_base__init()
{
	ASSERT(0);
}

namespace NRcs
{

//struct CAPRPoolManager {
//	CAPRPoolManager() { apr_pool_initialize(); }
//	~CAPRPoolManager() { apr_pool_terminate(); }
//} theAPRPoolManager;

CError::CError()
{
}

CError::CError( const string &_message )
: message(_message),
	exception(message.c_str())
{
}

CClient::CClient()
{
	Init();
}

CClient::CClient( const string &_dataDir )
: dataDir(_dataDir)
{
	Init();
}

void CClient::Init()
{
//	pool = svn_pool_create(NULL);
//
//	if ( !pool )
//		throw CError();
//
//	svn_error_t *err =	svn_client_create_context( &clientCtx, pool );
//	if ( err )
//	{
//		throw CError( err->message );
//	}

	// TODO initialize message callback function here
}

CClient::~CClient()
{
//	svn_pool_destroy( pool );
}

void CClient::SetFileChanged( const string &_path )
{
	changedList.push_back( _path );
}

void CClient::SetDataDir( const string &_dataDir )
{
	dataDir = _dataDir;
}

void CClient::CheckOut( const string &_repository )
{
//	svn_revnum_t revision;
//	apr_pool_t *workingPool = svn_pool_create( pool );
//	
//	svn_opt_revision_t headRevision;
//	headRevision.kind = svn_opt_revision_head;
//
//	svn_error_t *err = 
//		svn_client_checkout( 
//			&revision, 
//			_repository.c_str(), dataDir.c_str(), 
//			&headRevision,
//			true,
//			clientCtx,
//			workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	svn_pool_destroy( workingPool );
}

void CClient::CheckIn()
{
	// TODO error checking
	//CClient::CheckIn( dataDir );

//	svn_client_commit_info_t *commitInfo;
//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	apr_array_header_t *paths = apr_array_make( workingPool, 0, sizeof(char *) );
//
//	for ( TFileList::iterator it = changedList.begin(); it != changedList.end(); ++it )
//	{
//		const string &name = *it;
//
//		const char **pathloc = (const char **)apr_array_push( paths );
//		*pathloc = name.c_str();
//	}
//
//	svn_error_t *err =
//		svn_client_commit(
//		&commitInfo,
//		paths,
//		false,
//		clientCtx,
//		workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	changedList.clear();
//
//	svn_pool_destroy( workingPool );
}

void CClient::CheckIn( const string &_path )
{
//	svn_client_commit_info_t *commitInfo;
//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	apr_array_header_t *paths = apr_array_make( workingPool, 0, sizeof(char *) );
//	const char **pathloc = (const char **)apr_array_push( paths );
//	*pathloc = _path.c_str();
//	
//	svn_error_t *err =
//		svn_client_commit(
//			&commitInfo,
//			paths,
//			false,
//			clientCtx,
//			workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	svn_pool_destroy( workingPool );
}

void CClient::Update()
{
	// TODO error checking
	Update( dataDir );
}

void CClient::Update( const string &_path )
{
//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	svn_revnum_t revision;
//
//	svn_opt_revision_t headRevision;
//	headRevision.kind = svn_opt_revision_head;
//
//	svn_error_t *err = 
//		svn_client_update(
//			&revision,
//			_path.c_str(),
//			&headRevision,
//			true,
//			clientCtx,
//			workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	svn_pool_destroy( workingPool );
}

void CClient::AddFile( const string &_path )
{
	SetFileChanged( _path );

//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	svn_error_t *err = 
//		svn_client_add2( 
//			_path.c_str(), 
//			false, // recursive
//			true, // force
//			clientCtx, 
//			workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	svn_pool_destroy( workingPool );
}

void CClient::MoveFile( const string &_srcPath, const string &_dstPath )
{
	SetFileChanged( _dstPath );

//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	svn_client_commit_info_t *commitInfo;
//
//	svn_error_t *err =
//		svn_client_move(
//			&commitInfo,
//			_srcPath.c_str(),
//			0, // revision (необходимо только для операций внутри репозитария)
//			_dstPath.c_str(),
//			false, // force (работать с файлом, если он не под контролем версий)
//			clientCtx,
//			workingPool );
//
//	if ( err )
//	{
//		svn_pool_destroy( workingPool );
//		throw CError( err->message );
//	}
//
//	svn_pool_destroy( workingPool );
}

void CClient::RemoveFile( const string &_path )
{
	SetFileChanged( _path );

//	apr_pool_t *workingPool = svn_pool_create( pool );
//
//	svn_client_commit_info_t *commitInfo;
//
//	apr_array_header_t *paths = apr_array_make( workingPool, 0, sizeof(char *) );
//	const char **pathloc = (const char **)apr_array_push( paths );
//	*pathloc = _path.c_str();
//
//	svn_error_t *err =
//		svn_client_delete( 
//			&commitInfo,
//			paths,
//			false, // force
//			clientCtx,
//			workingPool );
//
//	svn_pool_destroy( workingPool );
}

}
