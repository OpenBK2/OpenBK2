#pragma once
#include "ResourceManager.h"

class CResourceManagerWrapper : public IResourceManager
{
	OBJECT_NOCOPY_METHODS( CResourceManagerWrapper )
	//
	string szSrcPath;
	string szDstPath;


public:
	CResourceManagerWrapper();
	~CResourceManagerWrapper();

	// IResourceManager
	IManipulator* CreateTableManipulator();
	IManipulator* CreateFolderManipulator( const string &szName );
	IManipulator* CreateFolderManipulator( int nID );
	IManipulator* CreateObjectManipulator( const string &szTypeName, const string &szObjectName );
	IManipulator* CreateObjectManipulator( const string &szTypeName, int nID );
	IManipulator* CreateObjectManipulator( int nTypeID, int nID );
	IManipulator* CreateObjectManipulator( const string &szTypeName, const CDBID &rCDBID );
	IManipulator* CreateObjectManipulator( const string &szObjectName );
	IManipulator* CreateObjectManipulator( const CDBID &rCDBID );

	bool SerializeObject( CDataStream *pStream, int nTypeID, int nObjectID );
	void SerializeObjects( const string &szFile );
	void SerializeObjects( const string &szFile, const string &szTypeName, const string &szRootObject );

	void SetConfig( const SDBConfig &config );

	void SetDefControls( const string &szFileName );
	void FillReferencingObjects( bool *pServiceIsReady, const string &szTypeName, const string &szObjectName, list<string> &results );
	bool HasReferencingObjects( bool *pServiceIsReady, const string &szTypeName, int nObjectID );
	void ResetCache();

	virtual void SetDataDir( const string &szDataDir );

	virtual void SyncDB();
	virtual bool CanSyncDB();

	virtual bool CheckIn();
	virtual bool CheckOut();
	virtual bool GetLatest();

	virtual bool InitializeVersionControl();
};


