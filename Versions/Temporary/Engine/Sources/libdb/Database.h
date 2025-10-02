#pragma once

#include "System/Db.h"
#include "DBObserverContainer.h"

#define INDEX_FILE_NAME "index.bin"
#define TYPES_FILE_NAME "types.xml"

namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

namespace NDb
{

struct IObjMan;
struct IDbObserver;
struct STypeObjectHeader;
struct SFullTypeHeader;
namespace NTypeDef
{
	struct STypeClass;
}

class COldTypeIDs
{
	typedef hash_map<int, CDBID> CRecordID2DBIDMap;
	typedef hash_map< int, CRecordID2DBIDMap> COldTypesMap;
	COldTypesMap oldTypesMap;
public:
	bool RegisterOldType( const int nTypeID, const int nRecordID, const CDBID &dbid )
	{
		if ( oldTypesMap[nTypeID].find( nRecordID ) != oldTypesMap[nTypeID].end() )
		{
			DebugTrace( "object with ID = %d already registered (old \"%s\", new \"%s\")", nRecordID, oldTypesMap[nTypeID][nRecordID].ToString().c_str(), dbid.ToString().c_str() );
			return false;
		}
		oldTypesMap[nTypeID][nRecordID] = dbid;
		return true;
	}
	void UnRegisterOldType( const int nTypeID, const int nRecordID )
	{
		COldTypesMap::iterator posType = oldTypesMap.find( nTypeID );
		if ( posType != oldTypesMap.end() )
			posType->second.erase( nRecordID );
	}
	const CDBID *GetDBID( const int nTypeID, const int nRecordID ) const
	{
		COldTypesMap::const_iterator posType = oldTypesMap.find( nTypeID );
		if ( posType == oldTypesMap.end() )
			return 0;
		hash_map<int, CDBID>::const_iterator posDBID = posType->second.find( nRecordID );
		if ( posDBID == posType->second.end() )
			return 0;
		return &( posDBID->second );
	}
};

class CBasicDatabase : public CObjectBase, public CDbObserverContainer
{
	CPtr<NVFS::IVFS> pVFS;
	CPtr<NVFS::IFileCreator> pFileCreator;
	bool bDataChanged;
protected:
	NVFS::IFileCreator *GetFileCreator() const { return pFileCreator; }
	NVFS::IVFS *GetVFS() const { return pVFS; }
	// read resource header. there is no dbid normalization inside
	bool ReadResourceHeader( STypeObjectHeader *pHeader, const CDBID &dbid );
	bool LoadIndex();
	virtual void RegisterObject( const SFullTypeHeader &hdr ) = 0;
	virtual void ResetIndexChanged() = 0;
	//
	void SetFileSystem( NVFS::IVFS *_pVFS, NVFS::IFileCreator *_pFileCreator )
	{
		pVFS = _pVFS;
		pFileCreator = _pFileCreator;
	}
	//
public:
	CBasicDatabase(): bDataChanged( false ) {}
	//
	virtual bool OpenDatabase( NVFS::IVFS *pVFS, NVFS::IFileCreator *pFileCreator ) = 0;
	virtual bool RegisterResourceFile( const string &szFileName ) = 0;
	virtual bool IsFileRegistered( const string &szFileName ) = 0;
	virtual void SetLoadDepth( int nLoadDepth ) = 0;
	//
	virtual IObjMan *GetManipulator( const CDBID &dbid ) = 0;
	virtual CResource *GetObject( const CDBID &dbid ) = 0;
	virtual bool DoesObjectExist( const CDBID &dbid ) = 0;
	//
	virtual IObjMan *CreateNewObject( const string &szClassTypeName ) = 0;
	virtual bool AddNewObject( const string &szFilePath, const CDBID &dbid, IObjMan *pObjMan ) = 0;
	virtual bool RemoveObject( const CDBID &dbid ) = 0;
	virtual bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew ) = 0;
	//
	virtual void MarkChanged( const CDBID &dbid ) = 0;
	virtual void SaveChanges() = 0;
	virtual void DropCachedResources() = 0;
	//
	// retrieve all terminal classes list
	virtual bool GetClassesList( vector<NTypeDef::STypeClass*> *pRes ) = 0;
	// retrieve all objects by type
	virtual bool GetObjectsList( vector<CDBID> *pRes, const string &szClassTypeName ) = 0;
	virtual bool GetObjectsList( vector<CDBID> *pRes, const int nClassTypeID ) = 0;
	// retrieve class type name for requested object
	virtual string GetClassTypeName( const CDBID &dbid ) = 0;
	// changed DB objects - checks and reports
	void SetDataChanged() { bDataChanged = true; }
	void SetDataChanged( const CDBID &dbid ) { SetDataChanged(); ReportObjectChanged(dbid); }
	void ResetDataChanged() { bDataChanged = false; }
	bool HasChangedObjects() const { return bDataChanged; }
};

void NormalizeDBID( CDBID *pRes, const CDBID &dbid );
void SetDBID( CResource *pRes, const CDBID &_dbid );

class CResourceHelper
{
public:
	static void SetDBID( CResource *pRes, const CDBID &_dbid );
	static void SetLoaded( CResource *pRes );
	static void CallPostLoad( CResource *pRes, bool bInEditor );
};

}

