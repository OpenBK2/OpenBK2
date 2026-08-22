#include "stdafx.h"

#include "TableManipulator.h"

#include <fmt/format.h>

CTableManipulatorWrapper::CTableManipulatorWrapper( std::vector<STypeClass *> &classes )
{
	for ( int i = 0; i < classes.size(); ++i )
	{
		STypeClass *pClass = classes[i];
		if ( pClass->nClassTypeID == -1 )
			continue;
		namesMap[pClass->szTypeName] = pClass;
		idsMap[pClass->nClassTypeID] = pClass;
	}
}

IManipulatorIterator* CTableManipulatorWrapper::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CTableManipulatorIteratorWrapper( this, bShowHidden );
}

const STypeClass* CTableManipulatorWrapper::GetType( const std::string &szName ) const
{
	CNamesMap::const_iterator pos = namesMap.find( szName );
	return pos == namesMap.end() ? 0 : pos->second;
}

const STypeClass* CTableManipulatorWrapper::GetType( int nTypeID ) const
{
	CIDsMap::const_iterator pos = idsMap.find( nTypeID );
	return pos == idsMap.end() ? 0 : pos->second;
}

unsigned CTableManipulatorWrapper::GetID( const std::string &szName ) const
{
	const STypeClass *pType = GetType( szName );
	if ( pType )
		return pType->nClassTypeID;
	else
	{
		NI_ASSERT( pType, fmt::format("CTableManipulatorWrapper::GetID( \"{}\" )", szName ) );
		return -1;
	}
}


bool CTableManipulatorWrapper::GetName( unsigned nID, std::string *pszName ) const
{
	const STypeClass *pType = GetType( nID );
	if ( pType )
	{
		*pszName = pType->szTypeName;
		return true;
	}
	return false;
}


bool CTableManipulatorWrapper::GetValue( const std::string &szName, CVariant *pValue ) const
{
	const STypeClass *pType = GetType( szName );
	if ( pType )
	{
		*pValue = pType->szTypeName;//pType->desc.szTypePath;
		return true;
	}
	return false;
}


bool CTableManipulatorWrapper::IsNameExists( const std::string &rszName ) const
{
	return namesMap.find( rszName ) != namesMap.end();
}


// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

CTableManipulatorIteratorWrapper::CTableManipulatorIteratorWrapper( CTableManipulatorWrapper *pMan, bool _bShowHidden )
{
	pTableMan = pMan;
	bShowHidden = _bShowHidden;
	itCurrType = pTableMan->namesMap.begin();
//	if ( !IsEnd() )
//	{
//		const STypeClass *pType = itCurrType->second;
//		if ( !pType->IsFinal() || ( !bShowHidden && pType->desc.bHidden ) )
//		{
//			Next();
//		}
//	}
}

bool CTableManipulatorIteratorWrapper::Next()
{
	if ( !IsEnd() )
	{
		++itCurrType;
//		if ( !IsEnd() )
//		{
//			const STypeClass *pType = itCurrType->second;
//			if ( !pType->IsFinal() || (!bShowHidden && pType->desc.bHidden) )
//			{
//				Next();
//			}
//		}
		return !IsEnd();
	}
	return false;
}

bool CTableManipulatorIteratorWrapper::IsEnd() const
{
	return itCurrType == pTableMan->namesMap.end();
}

bool CTableManipulatorIteratorWrapper::GetName( std::string *pszName ) const
{
	if ( !IsEnd() )
	{
		*pszName = itCurrType->second->szTypeName;
		return true;
	}
	return false;
}


