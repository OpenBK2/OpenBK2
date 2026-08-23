#pragma once

#include "libdb_export.h"


namespace NVFS
{
	struct IVFS;
	struct IFileCreator;
}

namespace NDb
{

enum EDatabaseMode : int
{
	DATABASE_MODE_EDITOR = 1,
	DATABASE_MODE_GAME = 2,
};

//! open database - prepare it for usage
LIBDB_EXPORT bool OpenDatabase( NVFS::IVFS *pVFS, NVFS::IFileCreator *pFileCreator, EDatabaseMode eMode );
//! close database - no database usage allowed after this call
LIBDB_EXPORT void CloseDatabase();
//! set new load depth - how many (how deeply) hierarchical objects will be loaded by refs during resource load
LIBDB_EXPORT void SetLoadDepth( int nLoadDepth );
//! get object from database
class CResource *GetObject( const CDBID &dbid );
//! does object exist?
bool DoesObjectExist( const CDBID &dbid );
//! retrieve class type name for requested object
LIBDB_EXPORT std::string GetClassTypeName( const CDBID &dbid );
//! retrieve all objects by type
bool GetObjectsList( std::vector<CDBID> *pRes, const int nClassTypeID );

//! check, that DBID correctly composed (NOTE: this doesn't check for object existance!)
bool IsDBIDValid( const CDBID &dbid );

//! special function for object load-time profiling
LIBDB_EXPORT void SegmentProfiler();

}


