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
	~CResourceManagerWrapper() override;

	// IResourceManager
	IManipulator *CreateTableManipulator() override;
	IManipulator *CreateFolderManipulator( const std::string &szName ) override;
	IManipulator *CreateFolderManipulator( int nID ) override;
	IManipulator *CreateObjectManipulator( const std::string &szTypeName, const std::string &szObjectName ) override;
	IManipulator *CreateObjectManipulator( const std::string &szTypeName, int nID );
	IManipulator *CreateObjectManipulator( int nTypeID, int nID );
	IManipulator *CreateObjectManipulator( const std::string &szTypeName, const CDBID &rCDBID ) override;
	IManipulator *CreateObjectManipulator( const std::string &szObjectName ) override;
	IManipulator *CreateObjectManipulator( const CDBID &rCDBID ) override;

	bool SerializeObject( CDataStream *pStream, int nTypeID, int nObjectID );
	void SerializeObjects( const std::string &szFile );
	void SerializeObjects( const std::string &szFile, const std::string &szTypeName, const std::string &szRootObject );

	void SetConfig( const SDBConfig &config ) override;

	void SetDefControls( const std::string &szFileName );
	void FillReferencingObjects( bool *pServiceIsReady,
	                             const std::string &szTypeName,
	                             const std::string &szObjectName,
	                             std::list<std::string> &results ) override;
	bool HasReferencingObjects( bool *pServiceIsReady, const std::string &szTypeName, int nObjectID );
	void ResetCache() override;

	void SetDataDir( const std::string &szDataDir ) override;

	void SyncDB() override;
	bool CanSyncDB() override;

	bool CheckIn() override;
	bool CheckOut() override;
	bool GetLatest() override;

	bool InitializeVersionControl() override;
};
