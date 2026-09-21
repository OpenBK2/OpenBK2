#pragma once

#include "libdb_export.h"

#include "Db.h"
#include <cstdint>

namespace NDb
{
struct IObjMan;
struct IDbObserver;
namespace NTypeDef
{
	struct STypeClass;
}

//! get object manipulator for given object. NOTE: for editor mode only!
LIBDB_EXPORT IObjMan *GetManipulator( const CDBID &dbid );
//! create new object for further edit. NOTE: for editor mode only!
IObjMan *CreateNewObject( const std::string &szClassTypeName );
//! register new object, created with CreateNewObject() function, in database
bool AddNewObject( const std::string &szFilePath, const CDBID &dbid, IObjMan *pObjMan );
//! remove object from database
LIBDB_EXPORT bool RemoveObject( const CDBID &dbid );
//! rename object in database
bool RenameObject( const CDBID &dbidOld, const CDBID &dbidNew );
//! rename several objects at once. Scans for references a single time, which a
//! loop over RenameObject would do once per object.
bool RenameObjects( const std::vector< std::pair<CDBID, CDBID> > &renames );
//! every object in the database whose fields point at dbid.
//! Reads every registered object, so it costs a pass over the database and is
//! meant for the things a person asks for explicitly: renaming an object, and
//! the References dialog. NOTE: for editor mode only!
LIBDB_EXPORT bool GetReferencingObjects( std::vector<CDBID> *pRes, const CDBID &dbid );
//! mark object as changed to save it
LIBDB_EXPORT void MarkChanged( const CDBID &dbid );
// Property-tree dirty labels compare values against the last save, independently
// of the bounded undo history. Querying enables tracking for this resource.
LIBDB_EXPORT bool IsPropertyModified( const CDBID &dbid, const std::string &szName );
LIBDB_EXPORT uint64_t GetPropertyChangeVersion();
//! save all objects, marked as changed
LIBDB_EXPORT void SaveChanges();
//! Persist newly registered files without saving unrelated edited resources.
LIBDB_EXPORT bool SaveChangedIndex();
//! drop all cached resources
void DropCachedResources();
//! check, have we changed DB objects?
bool HasChangedObjects();
//! retrieve all terminal classes list
LIBDB_EXPORT bool GetClassesList( std::vector<NTypeDef::STypeClass*> *pRes );
//! retrieve all objects by type
bool GetObjectsList( std::vector<CDBID> *pRes, const std::string &szClassTypeName );

LIBDB_EXPORT bool RegisterResourceFile( const std::string &szFileName );
LIBDB_EXPORT bool IsFileRegistered( const std::string &szFileName );
//! add database observer
LIBDB_EXPORT void AddDbObserver( IDbObserver *pObserver );

}


