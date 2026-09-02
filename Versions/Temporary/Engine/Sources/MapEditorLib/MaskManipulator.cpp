#include "stdafx.h"

#include "MaskManipulator.h"

CMaskManipulator::CMaskManipulator( const std::string& rszMask,  IManipulator* _pTargetManipulator, EMaskMode _maskMode )
	: szMask( rszMask ), pTargetManipulator( _pTargetManipulator ), maskMode( _maskMode )
{
	NI_ASSERT( _pTargetManipulator != 0, "CMaskManipulator::CMaskManipulator() pTargetManipulator == 0" );
}
	

bool CMaskManipulator::AddName( const std::string &rszName, bool bFilled, const std::string& rszType, unsigned nID, bool bHidden )
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


bool CMaskManipulator::SetToOriginalName( std::string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulator::SetToOriginalName() pszName == 0" );
	std::string szName = ( *pszName );
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


bool CMaskManipulator::SetToMaskName( std::string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulator::SetToMaskName() pszName == 0" );
	std::string szName = ( *pszName );
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


const SIteratorDesc* CMaskManipulator::GetDesc( const std::string &rszName ) const
{
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return 0;
	}
	return pTargetManipulator->GetDesc( szOriginalName );
}


bool CMaskManipulator::GetType( const std::string &rszName, std::string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMaskManipulator::GetType() pszType == 0" );
	std::string szMaskName = rszName;
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


unsigned CMaskManipulator::GetID( const std::string &rszName ) const
{
	std::string szMaskName = rszName;
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


bool CMaskManipulator::GetName( unsigned nID, std::string *pszName ) const
{
	NI_ASSERT( pszName != 0, "GetName::GetType() pszName == 0" );
	std::string szName;
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


bool CMaskManipulator::InsertNode( const std::string &rszName, int nNodeIndex )
{
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->InsertNode( szOriginalName, nNodeIndex );
}


bool CMaskManipulator::RemoveNode( const std::string &rszName, int nNodeIndex )
{
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->RemoveNode( szOriginalName, nNodeIndex );
}


bool CMaskManipulator::RenameNode( const std::string &rszName, const std::string &rszNewName )
{
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	std::string szOriginalNewName = rszNewName;
	if ( !SetToOriginalName( &szOriginalNewName ) )
	{
		return false;
	}
	return pTargetManipulator->RenameNode( szOriginalName, szOriginalNewName );
}


bool CMaskManipulator::GetValue( const std::string &rszName, CVariant *pValue ) const
{
	NI_ASSERT( pValue != 0, "CMaskManipulator::GetValue() pValue == 0" );
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->GetValue( szOriginalName, pValue );
}


bool CMaskManipulator::SetValue( const std::string &rszName, const CVariant &rValue )
{
	std::string szOriginalName = rszName;
	if ( !SetToOriginalName( &szOriginalName ) )
	{
		return false;
	}
	return pTargetManipulator->SetValue( szOriginalName, rValue );
}


bool CMaskManipulator::CheckValue( const std::string &rszName, const CVariant &rValue, bool *pResult ) const
{
	NI_ASSERT( pResult != 0, "CMaskManipulator::CheckValue() pResult == 0" );
	std::string szOriginalName = rszName;
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


bool CMaskManipulator::IsNameExists( const std::string &rszName ) const
{
	std::string szOriginalName = rszName;
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
	std::string szOriginalName = pMaskManipulator->szMask + ( *propertyIterator );
	return pMaskManipulator->GetDesc( szOriginalName );
}


bool CMaskManipulatorIterator::GetName( std::string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMaskManipulatorIterator::GetName() pszName == 0" );
	std::string szMaskName = ( *propertyIterator );
	if ( pMaskManipulator->GetMode() == CMaskManipulator::ORIGINAL_MODE )
	{
		szMaskName = pMaskManipulator->szMask + szMaskName;
	}
	( *pszName ) = szMaskName;
	return true;
}


bool CMaskManipulatorIterator::GetType( std::string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMaskManipulatorIterator::GetType() pszType == 0" );
	const std::string szMaskName = ( *propertyIterator );
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


unsigned CMaskManipulatorIterator::GetID() const
{
	const std::string szMaskName = ( *propertyIterator );
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


