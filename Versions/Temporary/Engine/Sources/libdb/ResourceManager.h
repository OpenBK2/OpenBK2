#pragma once

#include "Manipulator.h"


//interface for manipulating db data and related services
interface IResourceManager : CObjectBase
{
	enum { tidTypeID = 0x10074AC0 };
	//
	struct SDBConfig
	{
		string szSrcFilePath;
		string szDstFilePath;
		string szObjectRecordIDsPath;
	};

	// starts up ResourceManager, return true if it has been bootstrapped and ready
	//virtual bool Startup() = 0;

	//get manipulator for key-referenced types
	virtual IManipulator* CreateTableManipulator() = 0;
	//get manipulator for objects tree structure (arg - type name)
	virtual IManipulator* CreateFolderManipulator( const string &szTypeName ) = 0;
	virtual IManipulator* CreateFolderManipulator( int nID ) = 0;
	//get db object manipulator
	virtual IManipulator* CreateObjectManipulator( const string &szTypeName, const string &szObjectName ) = 0;
	virtual IManipulator* CreateObjectManipulator( const string &szTypeName, const CDBID &rCDBID ) = 0;
	virtual IManipulator* CreateObjectManipulator( const string &szObjectName ) = 0;
	virtual IManipulator* CreateObjectManipulator( const CDBID &rCDBID ) = 0;


	//Set DBConfig file full path. Should be done right after initializing singleton.
	virtual void SetConfig( const SDBConfig &config ) = 0;

//	enum EDb
//	{
//		DB_LOCAL,
//		DB_ANY_EXISTENT,
//		DB_MAIN
//	};
 
	virtual void FillReferencingObjects( bool *bServiceIsReady, const string &szTypeName, const string &szObjectName, list<string> &results ) = 0;
	virtual void ResetCache() = 0;

	virtual void SetDataDir( const string &dataDir ) {}

	virtual void SyncDB()    = 0;
	virtual bool CanSyncDB() = 0;
	virtual bool CheckIn()   = 0;
	virtual bool CheckOut()  = 0;
	virtual bool GetLatest() = 0;

	virtual bool InitializeVersionControl() = 0;

	//// FIXME: used by dbconvertor only
	////alters current db tables state to new one, according to new xsd files (arg - full paths to xsd files)
	//virtual void CreateTables( list<NCodeGen::SXSDDesc> &vNS, bool bRemoveOld ) = 0;
	////generates sources with db structs descriptions
	//virtual void GenerateHeaders( hash_map< string , SCppHeadDesc > &heads, const SCodeSettings &settings, list<string> &generatedFiles ) = 0;
	//virtual void GenerateHeaders( list<NCodeGen::SXSDDesc> &vNS, hash_map< string , SCppHeadDesc > &heads, const SCodeSettings &settings, list<string> &generatedFiles ) = 0;
	//virtual void RestartChunkIDs( const string &szProject ) = 0;
	//virtual void CleanDB() = 0;
	//// FIXME: dev temp
	//void TransformDB( bool bFinal );
	//// FIXME: unused legacy, must be removed
	//// Take string with data for GraphViz
	//virtual string TakeVizString( const list<NCodeGen::SXSDDesc> &vNS ) = 0;


	//Static functions for singleton init/uninit. Should be called manually.
	static void InitSingleton();
	static void UninitSingleton();
};

