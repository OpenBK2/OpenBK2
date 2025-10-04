#include "stdafx.h"

#include "FolderManipulator.h"
#include "ObjMan.h"
#include "EditorDb.h"
#include "Bind.h"
#include "Database.h"
#include "System/FilePath.h"
#include "Misc/HPTimer.h"

//#include <vtuneapi.h>
//#pragma comment(lib, "vtuneapi.lib")


static bool IsFolderName( const string &szName )
{
	return !szName.empty() && ( szName[szName.size() - 1] == '\\' || szName[szName.size() - 1] == '/' );
}

CFolderManipulatorWrapper::CFolderManipulatorWrapper( const string &_szClassTypeName, const string &_szSrcPath, const string &_szDstPath ) 
: szClassTypeName( _szClassTypeName ), szSrcPath( _szSrcPath ), szDstPath( _szDstPath )
{
}

IManipulatorIterator* CFolderManipulatorWrapper::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CFolderManipulatorIteratorWrapper( szSrcPath, szClassTypeName );
}

bool CFolderManipulatorWrapper::InsertNode( const string &szName, int nNodeIndex )
{
	if ( szName.empty() )
		return false;
	if ( IsFolderName(szName) )
	{
//		if ( !szSrcPath.empty() )
//			NFile::CreatePath( szSrcPath + szName );
//		if ( !szDstPath.empty() )
//			NFile::CreatePath( szDstPath + szName );
		return true;
	}
	else
	{
		if ( NDb::IObjMan *pObjMan = NDb::CreateNewObject( szClassTypeName ) )
			return NDb::AddNewObject( szName, CDBID(szName), pObjMan );
	}
	return false;
}

bool CFolderManipulatorWrapper::RemoveNode( const string &szName, int nNodeIndex )
{
	if ( szName.empty() )
		return false;
	if ( IsFolderName(szName) )
		return true;
	else
		return NDb::RemoveObject( CDBID(szName) );
}

namespace NFolderManipulator { bool RenameNode( const string &szName, const string &szNewName ); }

bool CFolderManipulatorWrapper::RenameNode( const string &szName, const string &szNewName )
{
	return NFolderManipulator::RenameNode( szName, szNewName );
}

bool CFolderManipulatorWrapper::GetValue( const string &szName, CVariant *pValue ) const
{
	*pValue = 0;
	return true;
}

bool CFolderManipulatorWrapper::SetValue( const string &szName, const CVariant &value )
{
	return true;
}

UINT CFolderManipulatorWrapper::GetID( const string &szName ) const
{
	return INVALID_NODE_ID;
}

bool CFolderManipulatorWrapper::GetName( UINT nID, string *pszName ) const
{
	return false;
}

bool CFolderManipulatorWrapper::IsNameExists( const string &rszName ) const
{
	if ( rszName.empty() )
		return false;
	else if ( IsFolderName(rszName) )
	{
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

CFolderManipulatorIteratorWrapper::CFolderManipulatorIteratorWrapper( const string &szSrcPath, const string &szTypeName )
{
	NHPTimer::STime hptime;
	NHPTimer::GetTime( &hptime );
//	VTResume();
	// retrieve all objects by type
	bool bShowFullTree = NGlobal::GetVar( "show_full_tree", 0 ) != 0;
	vector<CDBID> objectsList;
	const string szTypeName2Retrieve = bShowFullTree ? "" : szTypeName;
	NDb::GetObjectsList( &objectsList, szTypeName2Retrieve );
	hash_map<NFile::CFilePath, int> checks;
	//
	int nCounter = 0;
	entriesList.reserve( objectsList.size() );
	for ( vector<CDBID>::const_iterator itDBID = objectsList.begin(); itDBID != objectsList.end(); ++itDBID )
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
	posCurrElement = entriesList.begin();
	//
//	VTPause();
	//
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

bool CFolderManipulatorIteratorWrapper::GetName( string *pszName ) const
{
	if ( IsEnd() )
		return false;

	if ( posCurrElement->IsObject() )
		*pszName = NDb::GetFileName( posCurrElement->dbid );
	else
		*pszName = NFile::GetFilePath( NDb::GetFileName(posCurrElement->dbid) );
	NStr::ReplaceAllChars( pszName, '/', '\\' );
	return !pszName->empty();
}

bool CFolderManipulatorIteratorWrapper::GetType( string *pszType ) const
{
	*pszType = posCurrElement->IsObject() ? "object" : "folder";
	return true;
}

UINT CFolderManipulatorIteratorWrapper::GetID() const
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


