#pragma once
#include "ResourceManager.h"

class CResourceManagerWrapper : public IResourceManager
{
	OBJECT_NOCOPY_METHODS( CResourceManagerWrapper )
	//
	std::string szSrcPath;
	std::string szDstPath;


public:
	CResourceManagerWrapper();
	~CResourceManagerWrapper();

	// IResourceManager
	IManipulator* CreateTableManipulator();
	IManipulator* CreateFolderManipulator( const std::string &szName );
	IManipulator* CreateFolderManipulator( int nID );
	IManipulator* CreateObjectManipulator( const std::string &szTypeName, const std::string &szObjectName );
	IManipulator* CreateObjectManipulator( const std::string &szTypeName, int nID );
	IManipulator* CreateObjectManipulator( int nTypeID, int nID );
	IManipulator* CreateObjectManipulator( const std::string &szTypeName, const CDBID &rCDBID );
	IManipulator* CreateObjectManipulator( const std::string &szObjectName );
	IManipulator* CreateObjectManipulator( const CDBID &rCDBID );

	bool SerializeObject( CDataStream *pStream, int nTypeID, int nObjectID );
	void SerializeObjects( const std::string &szFile );
	void SerializeObjects( const std::string &szFile, const std::string &szTypeName, const std::string &szRootObject );

	void SetConfig( const SDBConfig &config );

	void SetDefControls( const std::string &szFileName );
	void FillReferencingObjects( bool *pServiceIsReady, const std::string &szTypeName, const std::string &szObjectName, std::list<std::string> &results );
	bool HasReferencingObjects( bool *pServiceIsReady, const std::string &szTypeName, int nObjectID );
	void ResetCache();

	virtual void SetDataDir( const std::string &szDataDir );

	virtual void SyncDB();
	virtual bool CanSyncDB();

	virtual bool CheckIn();
	virtual bool CheckOut();
	virtual bool GetLatest();

	virtual bool InitializeVersionControl();
};


