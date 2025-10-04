#include "stdafx.h"

#include "Bind.h"
#include "BindArray.h"
#include "System/LightXML.h"
#include "Database.h"

namespace NDb
{
  //
  void SetDatabaseDataChanged( const CDBID &dbid );

//
namespace NBind
{

// ************************************************************************************************************************ //
// **
// ** bind struct
// **
// **
// **
// ************************************************************************************************************************ //

CBindStruct::CBindStruct( CResource *_pStruct, NMetaInfo::SStructMetaInfo *_pMetaInfo )
: pStruct( _pStruct ), pMetaInfo( _pMetaInfo ), ownValues( _pMetaInfo->nNumOwnValues ),
  bindProcessor( (BYTE*)_pStruct, _pMetaInfo->nNumOwnValues == 0 ? 0 : &(ownValues[0]), _pMetaInfo ),
	bLoaded( false ), bChanged( false ), bNewObject( false )
{
	if ( pMetaInfo->nNumOwnValues > 0 )
		pMetaInfo->ConstructStruct( (BYTE*)(pStruct.GetPtr()), ownValues.empty() ? 0 : &(ownValues[0]), true );
	bool bSuccess = pMetaInfo->fields.find( "flags" ) != pMetaInfo->fields.end();
}

CBindStruct::~CBindStruct()
{
	if ( pMetaInfo && pMetaInfo->nNumOwnValues > 0 )
		pMetaInfo->DestructStruct( (BYTE*)(pStruct.GetPtr()), ownValues.empty() ? 0 : &(ownValues[0]), true );
}

void CBindStruct::SetDBID( const CDBID &_dbid ) 
{ 
	dbidMain = _dbid; 
	CResourceHelper::SetDBID( pStruct.GetPtr(), _dbid );
}

void CBindStruct::SetChanged() 
{ 
	if ( IsLoaded() )
	{
		bChanged = true; 
		SetDatabaseDataChanged( dbidMain );
	}
}

wstring CBindStruct::GetAttribute( const string &szName ) const
{
	for ( CAttributesList::const_iterator it = attributes.begin(); it != attributes.end(); ++it )
	{
		if ( it->first == szName )
			return it->second;
	}
	return L"";
}

void CBindStruct::SetAttribute( const string &szName, const wstring &szValue )
{
	// first, try to find existing attribute
	for ( CAttributesList::iterator it = attributes.begin(); it != attributes.end(); ++it )
	{
		if ( it->first == szName )
		{
			it->second = szValue;
			return;
		}
	}
	// add new one
	attributes.push_back( pair<string, wstring>(szName, szValue) );
}

// ************************************************************************************************************************ //
// **
// ** struct iterator
// **
// **
// **
// ************************************************************************************************************************ //

bool CStructIterator::GotoNextFieldInLevels()
{
	while ( ++levels.back().nCurrField >= levels.back().pTypeStruct->fields.size() )
	{
		levels.pop_back();
		if ( levels.empty() )
			return false;
	}
	//
	if ( !bShowHidden && levels.back().pTypeStruct->fields[levels.back().nCurrField].HasAttribute( "hidden" ) != false )
		return GotoNextFieldInLevels();
	return true;
}

bool CStructIterator::AddLevel( const string &_szAddName, NTypeDef::STypeStructBase *_pTypeStruct )
{
	list<SLevel>::iterator pos = levels.insert( levels.end(), SLevel() );
	pos->szAddName = _szAddName;
	pos->pTypeStruct = _pTypeStruct;
	if ( _pTypeStruct->pBaseType != 0 )
	{
		pos->nCurrField = -1;
		return AddLevel( _szAddName, _pTypeStruct->pBaseType );
	}
	else
	{
		pos->nCurrField = -1;
		return GotoNextFieldInLevels();
	}
}

bool CStructIterator::Next()
{
	if ( levels.empty() )
		return false;
	SLevel &level = levels.back();
	if ( level.pAggregatedIterator )
	{
		if ( level.pAggregatedIterator->Next() == false )
		{
			level.pAggregatedIterator = 0;
			return GotoNextFieldInLevels();
		}
		else
			return true;
	}
	else if ( level.nCurrField < level.pTypeStruct->fields.size() )
	{
		if ( !bShowHidden && level.pTypeStruct->fields[level.nCurrField].HasAttribute( "hidden" ) != false )
			return GotoNextFieldInLevels();
		//
		switch ( level.pTypeStruct->fields[level.nCurrField].pType->eType )
		{
		case NTypeDef::TYPE_TYPE_STRUCT:
			if ( AddLevel( level.szAddName + level.pTypeStruct->fields[level.nCurrField].szName + ".",
				             checked_cast_ptr<NTypeDef::STypeStruct *>( level.pTypeStruct->fields[level.nCurrField].pType ) ) == false )
			{
				return GotoNextFieldInLevels();
			}
			else
				return true;

		case NTypeDef::TYPE_TYPE_ARRAY:
			level.pAggregatedIterator = new CArrayIterator( -1, level.szAddName + level.pTypeStruct->fields[level.nCurrField].szName,
					                                            checked_cast_ptr<NTypeDef::STypeArray *>(level.pTypeStruct->fields[level.nCurrField].pType),
												                              pObjMan, bShowHidden );
			if ( level.pAggregatedIterator->IsEnd() )
			{
				level.pAggregatedIterator = 0;
				return GotoNextFieldInLevels();
			}
			else
				return true;

		default:
			return GotoNextFieldInLevels();
		}
	}
	else
		return GotoNextFieldInLevels();
}

}
}


