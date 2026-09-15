#include "stdafx.h"

#include "FolderManipulator.h"
#include "ObjMan.h"
#include "EditorDb.h"
#include "Bind.h"
#include "Database.h"
#include "System/FilePath.h"
#include "Misc/HPTimer.h"
#include <filesystem>

static bool IsFolderName( const std::string &szName )
{
	return !szName.empty() && ( szName[szName.size() - 1] == '\\' || szName[szName.size() - 1] == '/' );
}

namespace
{
namespace fs = std::filesystem;

// Folder names in the database are relative to the configured data roots.
bool GetFolderPaths( const std::string &name, const std::string &src,
					 const std::string &dst, std::vector<fs::path> *paths )
{
	std::string relative = name;
	std::replace( relative.begin(), relative.end(), '\\', '/' );
	while ( !relative.empty() && relative.back() == '/' )
		relative.pop_back();
	const fs::path suffix( relative );
	if ( suffix.empty() || suffix.has_root_path() || relative.find(':') != std::string::npos )
		return false;
	for ( const auto &part : suffix )
		if ( part == ".." || part == "." )
			return false;

	paths->clear();
	if ( !src.empty() )
		paths->push_back( fs::path(src) / suffix );
	if ( !dst.empty() && (src.empty() || !NFile::ComparePathEq(src, dst)) )
		paths->push_back( fs::path(dst) / suffix );
	return !paths->empty();
}

bool CreateFolders( const std::vector<fs::path> &paths )
{
	for ( const auto &path : paths )
	{
		std::error_code error;
		fs::create_directories( path, error );
		if ( error )
			return false;
	}
	return true;
}

// Keep loose empty directories visible even though the DB index contains only
// resources. Do not follow directory links or enumerate files inside packages.
std::vector<fs::path> GetSubfolders( const fs::path &root )
{
	std::vector<fs::path> folders;
	std::error_code error;
	fs::recursive_directory_iterator it( root, fs::directory_options::skip_permission_denied, error ), end;
	while ( !error && it != end )
	{
		if ( it->is_symlink(error) )
			it.disable_recursion_pending();
		else if ( it->is_directory(error) )
			folders.push_back( it->path() );
		it.increment( error );
	}
	return folders;
}

void RemoveEmptyFolders( const std::vector<fs::path> &paths )
{
	// DB resources are removed/saved by the editor separately. Never remove
	// their files here; only clean up directories that are already empty.
	for ( auto it = paths.rbegin(); it != paths.rend(); ++it )
	{
		std::error_code error;
		if ( fs::is_directory(*it, error) && fs::is_empty(*it, error) && !error )
			fs::remove( *it, error );
	}
}
}

CFolderManipulatorWrapper::CFolderManipulatorWrapper( const std::string &_szClassTypeName, const std::string &_szSrcPath, const std::string &_szDstPath )
: szClassTypeName( _szClassTypeName ), szSrcPath( _szSrcPath ), szDstPath( _szDstPath )
{
}

IManipulatorIterator* CFolderManipulatorWrapper::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CFolderManipulatorIteratorWrapper( szSrcPath, szDstPath, szClassTypeName );
}

bool CFolderManipulatorWrapper::InsertNode( const std::string &szName, int nNodeIndex )
{
	if ( szName.empty() )
		return false;
	if ( IsFolderName(szName) )
	{
		// Creating just a tree item loses the folder on the next reload.
		std::vector<fs::path> paths;
		return GetFolderPaths( szName, szSrcPath, szDstPath, &paths ) && CreateFolders(paths);
	}
	else
	{
		if ( NDb::IObjMan *pObjMan = NDb::CreateNewObject( szClassTypeName ) )
			return NDb::AddNewObject( szName, CDBID(szName), pObjMan );
	}
	return false;
}

bool CFolderManipulatorWrapper::RemoveNode( const std::string &szName, int nNodeIndex )
{
	if ( szName.empty() )
		return false;
	if ( IsFolderName(szName) )
	{
		std::vector<fs::path> paths;
		if ( !GetFolderPaths(szName, szSrcPath, szDstPath, &paths) )
			return false;
		RemoveEmptyFolders( paths );
		return true;
	}
	else
		return NDb::RemoveObject( CDBID(szName) );
}

namespace NFolderManipulator { bool RenameNode( const std::string &szName, const std::string &szNewName ); }

bool CFolderManipulatorWrapper::RenameNode( const std::string &szName, const std::string &szNewName )
{
	if ( !IsFolderName(szName) )
		return NFolderManipulator::RenameNode( szName, szNewName );

	std::vector<fs::path> oldRoots, newRoots, oldFolders, newFolders;
	if ( !IsFolderName(szNewName) ||
		 !GetFolderPaths(szName, szSrcPath, szDstPath, &oldRoots) ||
		 !GetFolderPaths(szNewName, szSrcPath, szDstPath, &newRoots) )
		return false;
	if ( NFile::ComparePathEq(szName, szNewName) )
		return true;
	for ( size_t i = 0; i < oldRoots.size(); ++i )
	{
		oldFolders.push_back( oldRoots[i] );
		newFolders.push_back( newRoots[i] );
		for ( const auto &folder : GetSubfolders(oldRoots[i]) )
		{
			oldFolders.push_back( folder );
			newFolders.push_back( newRoots[i] / folder.lexically_relative(oldRoots[i]) );
		}
	}
	// Preserve the directory structure, including empty children. Resource
	// files and references still go through the existing DB rename/save path.
	if ( !CreateFolders(newFolders) || !NFolderManipulator::RenameNode(szName, szNewName) )
		return false;
	RemoveEmptyFolders( oldFolders );
	return true;
}

bool CFolderManipulatorWrapper::GetValue( const std::string &szName, CVariant *pValue ) const
{
	*pValue = 0;
	return true;
}

bool CFolderManipulatorWrapper::SetValue( const std::string &szName, const CVariant &value )
{
	return true;
}

unsigned CFolderManipulatorWrapper::GetID( const std::string &szName ) const
{
	return INVALID_NODE_ID;
}

bool CFolderManipulatorWrapper::GetName( unsigned nID, std::string *pszName ) const
{
	return false;
}

bool CFolderManipulatorWrapper::IsNameExists( const std::string &rszName ) const
{
	if ( rszName.empty() )
		return false;
	else if ( IsFolderName(rszName) )
	{
		std::vector<fs::path> paths;
		if ( GetFolderPaths(rszName, szSrcPath, szDstPath, &paths) )
			for ( const auto &path : paths )
			{
				std::error_code error;
				if ( fs::exists(path, error) )
					return true;
			}
		return false;
	}
	else
	{
		CObj<NDb::IObjMan> pObjMan = NDb::GetManipulator( CDBID(rszName) );
		return pObjMan != 0;
	}
}

void CFolderManipulatorWrapper::GetNameList( CNameMap *pNameMap ) const
{
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

CFolderManipulatorIteratorWrapper::CFolderManipulatorIteratorWrapper( const std::string &szSrcPath, const std::string &szDstPath, const std::string &szTypeName )
{
	NHPTimer::STime hptime;
	NHPTimer::GetTime( &hptime );
	// retrieve all objects by type
	bool bShowFullTree = NGlobal::GetVar( "show_full_tree", 0 ) != 0;
	std::vector<CDBID> objectsList;
	const std::string szTypeName2Retrieve = bShowFullTree ? "" : szTypeName;
	NDb::GetObjectsList( &objectsList, szTypeName2Retrieve );
	std::unordered_map<NFile::CFilePath, int> checks;
	//
	int nCounter = 0;
	entriesList.reserve( objectsList.size() );
	for ( std::vector<CDBID>::const_iterator itDBID = objectsList.begin(); itDBID != objectsList.end(); ++itDBID )
	{
//		SEntry &entry = entriesList[nCounter];
//		entry.dbid = *itDBID;
		const int nLastCounter = nCounter;
		const bool bObject = bShowFullTree ? ( NDb::GetClassTypeName(*itDBID) == szTypeName ) : true;
		if ( !bObject )
		{
			if ( nCounter > 0 )
			{
				const NFile::CFilePath path = NFile::GetFilePath( NDb::GetFileName(*itDBID) );
				if ( checks.find(path) == checks.end() )
				{
					checks[path] = 1;
					++nCounter;
				}
			}
			else
				++nCounter;
		}
		else
			++nCounter;
		//
		if ( nLastCounter != nCounter )
		{
			CEntriesList::iterator pos = entriesList.insert( entriesList.end(), SEntry() );
			pos->dbid = *itDBID;
			pos->bObject = bObject;
		}
	}
	//
	// Empty folders have no resource type, so show them in every resource
	// browser. Ancestors are reconstructed by the tree as for indexed objects.
	const std::string roots[] = { szSrcPath, szDstPath };
	for ( int i = 0; i < 2; ++i )
	{
		if ( roots[i].empty() || (i == 1 && NFile::ComparePathEq(roots[0], roots[1])) )
			continue;
		for ( const auto &folder : GetSubfolders(fs::path(roots[i])) )
		{
			std::error_code error;
			if ( !fs::is_empty(folder, error) || error )
				continue;
			const std::string name = folder.lexically_relative(fs::path(roots[i])).generic_string() + "/";
			if ( checks.emplace(NFile::CFilePath(name), 1).second )
			{
				SEntry entry;
				entry.bObject = false;
				entry.szFolderName = name;
				entriesList.push_back( entry );
			}
		}
	}
	posCurrElement = entriesList.begin();

	const float fTimePassed = NHPTimer::GetTimePassed( &hptime );
	DebugTrace( "***** Time passed for full tree retrieve: %g msec *****", fTimePassed * 1000.0f );
}

bool CFolderManipulatorIteratorWrapper::Next()
{
	if ( IsEnd() )
		return false;
	++posCurrElement;
	return !IsEnd();
}

bool CFolderManipulatorIteratorWrapper::IsEnd() const
{
	return posCurrElement == entriesList.end();
}

const SIteratorDesc* CFolderManipulatorIteratorWrapper::GetDesc() const
{
	return 0;
}

bool CFolderManipulatorIteratorWrapper::GetName( std::string *pszName ) const
{
	if ( IsEnd() )
		return false;

	if ( posCurrElement->IsObject() )
		*pszName = NDb::GetFileName( posCurrElement->dbid );
	else if ( !posCurrElement->szFolderName.empty() )
		*pszName = posCurrElement->szFolderName;
	else
		*pszName = NFile::GetFilePath( NDb::GetFileName(posCurrElement->dbid) );
	NStr::ReplaceAllChars( pszName, '/', '\\' );
	return !pszName->empty();
}

bool CFolderManipulatorIteratorWrapper::GetType( std::string *pszType ) const
{
	*pszType = posCurrElement->IsObject() ? "object" : "folder";
	return true;
}

unsigned CFolderManipulatorIteratorWrapper::GetID() const
{
	return INVALID_NODE_ID;
}

bool CFolderManipulatorIteratorWrapper::IsFolder() const
{
	return !posCurrElement->IsObject();
}

void CFolderManipulatorIteratorWrapper::Reset()
{
	posCurrElement = entriesList.empty() ? entriesList.end() : entriesList.begin();
}


