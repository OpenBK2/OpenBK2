#pragma once

struct svn_client_ctx_t;
struct apr_pool_t;

namespace NRcs
{

class CError: public exception
{
	string message;
public:
	CError();
	CError( const string &_message );
};

/**
Revision Control System Client
*/
class CClient: public CObjectBase
{
	// TODO add logging support
	OBJECT_NOCOPY_METHODS(CClient);

protected:
	string dataDir;
	svn_client_ctx_t *clientCtx;
	apr_pool_t *pool;

	typedef vector<string> TFileList;
	TFileList changedList;

	void Init();
public:

	CClient();

	/**
	@param _dataDir root directory for game resources
	*/
	CClient( const string &_dataDir );

	virtual ~CClient();

	void SetDataDir( const string &_dataDir );

	/**
	Add given filename to changed list
	*/
	void SetFileChanged( const string &_path );

	/**
	Get clean copy of resources from specified URL
	@param _repository svn repository URL
	*/
	void CheckOut( const string &_repository );

	/**
	Update local repository with changes from main
	@param _path filename or subtree of data directory to update
	*/
	void Update();
	void Update( const string &_path );

	/**
	Push changes of files in changedList to main repository
	*/
	void CheckIn();

	/**
	Push local changes to main repository
	@param _path filename or subtree of data directory to check in
	*/
	void CheckIn( const string &_path );

	/**
	Push local changes to main repository
	@param _paths list of files or directories to check in
	*/
	void CheckIn( const vector<string> &_paths );

	/**
	Add file to repository
	@param _path filename to add
	*/
	void AddFile( const string &_path );

	/**
	Move file or directory
	@param _srcPath source location
	@param _dstPath destination location
	*/
	void MoveFile( const string &_srcPath, const string &_dstPath );

	/**
	Remove file
	*/
	void RemoveFile( const string &_path );
};

}
