#include "StdAfx.h"

#include "MaskManipulator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMaskManipulator::CMaskManipulator( const string& rszMask,  IManipulator* _pTargetManipulator, EMaskMode _maskMode )
	: szMask( rszMask ), pTargetManipulator( _pTargetManipulator ), maskMode( _maskMode )
{
	NI_ASSERT( _pTargetManipulator != 0, "CMaskManipulator::CMaskManipulator() pTargetManipulator == 0" );
}
	

bool CMaskManipulator::AddName( const string &rszName, bool bFilled, const string& rszType, UINT nID, bool bHidden )
{
	propertyList.push_back( rszName );
	//
	propertyMap[rszName] = SProperty();
	CPropertyMap::iterator posProperty = propertyMap.find( rszName );
	if ( posProperty != propertyMap.end() )
	{
		posProperty->second.szName = rszName;
		posProperty->second.bFilled = bFilled;
		if ( bFilled )
		{
			posProperty->second.szType = rszType;
			posProperty->second.nID = nID;
			posProperty->second.bHidden = bHidden;

			if ( nID != INVALID_NODE_ID )
			{
				propertyIDMap[nID] = rszName;
			}
		}
		return true;
	}
	return false;
}


bool CMaskManipulator::SetToOriginalName( string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulator::SetToOriginalName() pszName == 0" );
	string szName = ( *pszName );
	if ( GetMode() == ORIGINAL_MODE )
	{
		if ( szName.find( szMask ) != 0 )
		{
			return false;
		}
		szName = szName.substr( szMask.size() );
	}
	else if ( GetMode() == SMART_MODE )
	{
		if ( szName.compare( 0, szMask.size(), szMask ) == 0 )
		{
			szName = szName.substr( szMask.size() );
		}
	}
	if ( propertyMap.find( szName ) == propertyMap.end() )
	{
		return false;
	}
	( *pszName ) = szMask + szName;
	return true;
}


bool CMaskManipulator::SetToMaskName( string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulator::SetToMaskName() pszName == 0" );
	string szName = ( *pszName );
	if ( GetMode() == ORIGINAL_MODE )
	{
		if ( szName.find( szMask ) != 0 )
		{
			return false;
		}
		szName = szName.substr( szMask.size() );
	}
	else if ( GetMode() == SMART_MODE )
	{
		if ( szName.compare( 0, szMask.size(), szMask ) == 0 )
		{
			szName = szName.substr( szMask.size() );
		}
	}
	if ( propertyMap.find( szName ) == propertyMap.end() )
	{
		return false;
	}
	( *pszName ) = szName;
	return true;
}


IManipulatorIterator* CMaskManipulator::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CMaskManipulatorIterator( this );
}


const SIteratorDesc* CMaskManipulator::GetDesc( const string &rszName ) const
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return 0;
	}
	return pTargetManipulator->GetDesc( szOriginalName );
}


bool CMaskManipulator::GetType( const string &rszName, string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMaskManipulator::GetType() pszType == 0" );
	string szMaskName = rszName;
	if ( !SetToMaskName( &szMaskName ) )
	{
		return false;
	}
	CPropertyMap::const_iterator posProperty = propertyMap.find( szMaskName );
	NI_ASSERT( posProperty != propertyMap.end(), "CMaskManipulator::GetType() posProperty == propertyMap.end()" );
	if ( posProperty->second.bFilled )
	{
		( *pszType ) = posProperty->second.szType;
		return true;
	}
	else
	{
		return pTargetManipulator->GetType( szMask + szMaskName, pszType );
	}
}


UINT CMaskManipulator::GetID( const string &rszName ) const
{
	string szMaskName = rszName;
	if ( !SetToMaskName( &szMaskName ) )
	{
		return INVALID_NODE_ID;
	}
	CPropertyMap::const_iterator posProperty = propertyMap.find( szMaskName );
	NI_ASSERT( posProperty != propertyMap.end(), "CMaskManipulator::GetType() posProperty == propertyMap.end()" );
	if ( posProperty->second.bFilled )
	{
		return posProperty->second.nID;
	}
	else
	{
		return pTargetManipulator->GetID( szMask + szMaskName );
	}
}


bool CMaskManipulator::GetName( UINT nID, string *pszName ) const
{
	NI_ASSERT( pszName != 0, "GetName::GetType() pszName == 0" );
	string szName;
	CPropertyIDMap::const_iterator posPropertyID = propertyIDMap.find( nID );
	if ( posPropertyID != propertyIDMap.end() )
	{
		szName = posPropertyID->second;
		if ( szName.empty() )
		{
			return false;
		}
	}
	else
	{
		if ( !pTargetManipulator->GetName( nID, &szName ) )
		{
			return false;
		}
		if ( szName.find( szMask ) != 0 )
		{
			return false;
		}
		szName = szName.substr( szMask.size() );
		if ( propertyMap.find( szName ) == propertyMap.end() )
		{
			return false;
		}
	}
	if ( GetMode() == ORIGINAL_MODE )
	{
		szName = szMask + szName;
	}
	( *pszName ) = szName;
	return true;
}


bool CMaskManipulator::InsertNode( const string &rszName, int nNodeIndex )
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->InsertNode( szOriginalName, nNodeIndex );
}


bool CMaskManipulator::RemoveNode( const string &rszName, int nNodeIndex )
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->RemoveNode( szOriginalName, nNodeIndex );
}


bool CMaskManipulator::RenameNode( const string &rszName, const string &rszNewName )
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	string szOriginalNewName = rszNewName;
	if ( !SetToOriginalName( &szOriginalNewName ) )
	{
		return false;
	}
	return pTargetManipulator->RenameNode( szOriginalName, szOriginalNewName );
}


bool CMaskManipulator::GetValue( const string &rszName, CVariant *pValue ) const
{
	NI_ASSERT( pValue != 0, "CMaskManipulator::GetValue() pValue == 0" );
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->GetValue( szOriginalName, pValue );
}


bool CMaskManipulator::SetValue( const string &rszName, const CVariant &rValue )
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->SetValue( szOriginalName, rValue );
}


bool CMaskManipulator::CheckValue( const string &rszName, const CVariant &rValue, bool *pResult ) const
{
	NI_ASSERT( pResult != 0, "CMaskManipulator::CheckValue() pResult == 0" );
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->CheckValue( szOriginalName, rValue, pResult );
}


NDb::IObjMan* CMaskManipulator::GetObjMan()
{
	return pTargetManipulator->GetObjMan();
}


bool CMaskManipulator::IsNameExists( const string &rszName ) const
{
	string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->IsNameExists( szOriginalName );
}


void CMaskManipulator::GetNameList( IManipulator::CNameMap *pNameMap ) const
{
	if ( pNameMap )
	{
		( *pNameMap )[szMask] = 0;
	}
}


CMaskManipulatorIterator::CMaskManipulatorIterator( CMaskManipulator *_pMaskManipulator )
	:	pMaskManipulator( _pMaskManipulator )
{
	NI_ASSERT( pMaskManipulator != 0, "CMaskManipulatorIterator::CMaskManipulatorIterator() pMaskManipulator == 0" );
	propertyIterator = pMaskManipulator->propertyList.begin();
}


bool CMaskManipulatorIterator::Next()
{
	if ( IsEnd() )
	{
		return false;
	}
	++propertyIterator;
	return true;
}


bool CMaskManipulatorIterator::IsEnd() const
{
	return ( propertyIterator == pMaskManipulator->propertyList.end() );
}


const SIteratorDesc* CMaskManipulatorIterator::GetDesc() const
{
	string szOriginalName = pMaskManipulator->szMask + ( *propertyIterator );
	return pMaskManipulator->GetDesc( szOriginalName );
}


bool CMaskManipulatorIterator::GetName( string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulatorIterator::GetName() pszName == 0" );
	string szMaskName = ( *propertyIterator );
	if ( pMaskManipulator->GetMode() == CMaskManipulator::ORIGINAL_MODE )
	{
		szMaskName = pMaskManipulator->szMask + szMaskName;
	}
	( *pszName ) = szMaskName;
	return true;
}


bool CMaskManipulatorIterator::GetType( string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMaskManipulatorIterator::GetType() pszType == 0" );
	const string szMaskName = ( *propertyIterator );
	CMaskManipulator::CPropertyMap::const_iterator posProperty = pMaskManipulator->propertyMap.find( szMaskName );
	NI_ASSERT( posProperty != pMaskManipulator->propertyMap.end(), "CMaskManipulatorIterator::GetType() posProperty == pMaskManipulator->propertyMap.end()" );
	if ( posProperty->second.bFilled )
	{
		( *pszType ) = posProperty->second.szType;
		return true;
	}
	else
	{
		return pMaskManipulator->pTargetManipulator->GetType( pMaskManipulator->szMask + szMaskName, pszType );
	}
}


UINT CMaskManipulatorIterator::GetID() const
{
	const string szMaskName = ( *propertyIterator );
	CMaskManipulator::CPropertyMap::const_iterator posProperty = pMaskManipulator->propertyMap.find( szMaskName );
	NI_ASSERT( posProperty != pMaskManipulator->propertyMap.end(), "CMaskManipulatorIterator::GetType() posProperty == pMaskManipulator->propertyMap.end()" );
	if ( posProperty->second.bFilled )
	{
		return posProperty->second.nID;
	}
	else
	{
		return pMaskManipulator->pTargetManipulator->GetID( pMaskManipulator->szMask + szMaskName );
	}
}

// basement storage  

