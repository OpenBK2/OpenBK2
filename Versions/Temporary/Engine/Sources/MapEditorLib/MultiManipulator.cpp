#include "stdafx.h"

#include "MultiManipulator.h"
#include "PCIEMnemonics.h"

bool CMultiManipulator::DescExists( const string &rszName ) const
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( itManipulator->second->GetDesc( rszName ) == 0 )
		{
			return false;
		}
	}
	return true;
}


bool CMultiManipulator::TypeExists( const string &rszName ) const
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	string szType;
	for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( !itManipulator->second->GetType( rszName, &szType ) )
		{
			return false;
		}
	}
	return true;
}


bool CMultiManipulator::IDExists( const string &rszName ) const
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( itManipulator->second->GetID( rszName ) == INVALID_NODE_ID )
		{
			return false;
		}
	}
	return true;
}


bool CMultiManipulator::NameExists( UINT nID ) const
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	string szName;
	for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( !itManipulator->second->GetName( nID, &szName ) )
		{
			return false;
		}
	}
	return true;
}


int CMultiManipulator::GetMinimalCount( const string &rszName, bool *pbMultiVariant ) const
{
	if ( pbMultiVariant )
	{
		( *pbMultiVariant ) = false;
	}
	//
	if ( manipulatorMap.empty() )
	{
		return 0;
	}
	//
	CVariant value;
	CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin();
	if ( !itManipulator->second->GetValue( rszName, &value ) )
	{
		return 0;
	}
	int nMinimalCount = (int)value;
	for ( ++itManipulator; itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( !itManipulator->second->GetValue( rszName, &value ) )
		{
			return 0;
		}
		if ( nMinimalCount != (int)value )
		{
			if ( pbMultiVariant )
			{
				( *pbMultiVariant ) = true;
			}
		}
		if ( nMinimalCount > (int)value )
		{
			nMinimalCount = (int)value;
		}
	}
	return nMinimalCount;
}


bool CMultiManipulator::NameExists( const string &rszName ) const
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		if ( !itManipulator->second->IsNameExists( rszName ) )
		{
			return false;
		}
	}
	return true;
}


bool CMultiManipulator::GetMultiValue( const string &rszName, CVariant *pValue ) const
{
	NI_ASSERT( pValue != 0, "CMultiManipulator::GetMultiValue(): pValue == 0" );
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	CVariant firstValue;
	CVariant::CMultiVariantMap multiVariantMap;
	bool bMultiVariant = false;
	//
	CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin();
	if ( itManipulator->second->GetValue( rszName, &firstValue ) )
	{
		multiVariantMap[itManipulator->first] = firstValue;
	}
	else
	{
		return false;
	}
	//
	for ( ++itManipulator; itManipulator != manipulatorMap.end(); ++itManipulator )
	{
		CVariant value;
		if ( itManipulator->second->GetValue( rszName, &value ) )
		{
			multiVariantMap[itManipulator->first] = value;
			if ( !( firstValue == value ) )
			{
				bMultiVariant = true;
			}
		}
		else
		{
			return false;
		}
	}
	//
	if ( bMultiVariant )
	{
		pValue->SetMultiVariant( &multiVariantMap, true );
	}
	else
	{
		( *pValue ) = firstValue;
	}
	return true;
}


bool CMultiManipulator::SetMultiValue( const string &rszName, const CVariant &rValue )
{
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	if ( rValue.GetType() == CVariant::VT_MULTIVARIANT )
	{
		for ( CVariant::CMultiVariantMap::const_iterator itMultiVariant = rValue.GetMultiVariant()->begin(); itMultiVariant != rValue.GetMultiVariant()->end(); ++itMultiVariant )
		{
			CManipulatorMap::iterator posManipulator = manipulatorMap.find( itMultiVariant->first );
			if ( posManipulator != manipulatorMap.end() )
			{
				if ( !posManipulator->second->SetValue( rszName, itMultiVariant->second ) )
				{
					return false;
				}
			}
		}
	}
	else
	{
		for ( CManipulatorMap::iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
		{
			if ( !itManipulator->second->SetValue( rszName, rValue ) )
			{
				return false;
			}
		}
	}
	return true;
}


bool CMultiManipulator::CheckMultiValue( const string &rszName, const CVariant &rValue, bool *pResult ) const
{
	NI_ASSERT( pResult != 0, "CMultiManipulator::CheckMultiValue(): pResult == 0" );
	if ( manipulatorMap.empty() )
	{
		return false;
	}
	if ( rValue.GetType() == CVariant::VT_MULTIVARIANT )
	{
		for ( CVariant::CMultiVariantMap::const_iterator itMultiVariant = rValue.GetMultiVariant()->begin(); itMultiVariant != rValue.GetMultiVariant()->end(); ++itMultiVariant )
		{
			CManipulatorMap::const_iterator posManipulator = manipulatorMap.find( itMultiVariant->first );
			if ( posManipulator != manipulatorMap.end() )
			{
				if ( !posManipulator->second->CheckValue( rszName, itMultiVariant->second, pResult ) )
				{
					return false;
				}
				if ( !( *pResult ) )
				{
					return true;
				}
			}
		}
	}
	else
	{
		for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
		{
			if ( !itManipulator->second->CheckValue( rszName, rValue, pResult ) )
			{
				return false;
			}
			if ( !( *pResult ) )
			{
				return true;
			}
		}
	}
	return true;
}


void CMultiManipulator::InsertManipulator( const CDBID &rDBID, IManipulator* pManipulator, bool bActive, bool bPropertyDesc )
{
	manipulatorMap[rDBID] = pManipulator;
	if ( bActive )
	{
		activeDBID = rDBID;
		pActiveManipulator = pManipulator;
	}
	if ( bPropertyDesc )
	{
		propertyDescDBID = rDBID;
		pPropertyDescManipulator = pManipulator;
	}
	if ( manipulatorMap.begin() != manipulatorMap.end() )
	{
		pFirstManipulator = manipulatorMap.begin()->second;
	}
	else
	{
		pFirstManipulator = 0;
	}
}


void CMultiManipulator::RemoveManipulator( const CDBID &rDBID )
{
	CManipulatorMap::iterator posManipulator = manipulatorMap.find( rDBID );
	if ( posManipulator != manipulatorMap.end() )
	{
		manipulatorMap.erase( posManipulator );
		if ( activeDBID == rDBID )
		{
			activeDBID.Clear();
			pActiveManipulator = 0;
		}
		if ( propertyDescDBID == rDBID )
		{
			propertyDescDBID.Clear();
			pPropertyDescManipulator = 0;
		}
		if ( manipulatorMap.begin() != manipulatorMap.end() )
		{
			pFirstManipulator = manipulatorMap.begin()->second;
		}
		else
		{
			pFirstManipulator = 0;
		}
	}
}


bool CMultiManipulator::SetActiveManipulator( const CDBID &rDBID )
{
	CManipulatorMap::const_iterator posManipulator = manipulatorMap.find( rDBID );
	if ( posManipulator != manipulatorMap.end() )
	{
		activeDBID = posManipulator->first;
		pActiveManipulator = posManipulator->second;
		return true;
	}
	return false;
}


bool CMultiManipulator::SetPropertyDescManipulator( const CDBID &rDBID )
{
	CManipulatorMap::const_iterator posManipulator = manipulatorMap.find( rDBID );
	if ( posManipulator != manipulatorMap.end() )
	{
		propertyDescDBID = posManipulator->first;
		pPropertyDescManipulator = posManipulator->second;
		return true;
	}
	return false;
}


IManipulatorIterator* CMultiManipulator::Iterate( bool bShowHidden, ECacheType eCache )
{
	return new CMultiManipulatorIterator( this, bShowHidden, eCache );
}


const SIteratorDesc* CMultiManipulator::GetDesc( const string &rszName ) const
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetDesc( rszName );
	}
	else if ( DescExists( rszName ) )
	{
		if ( pPropertyDescManipulator != 0 )
		{
			return pPropertyDescManipulator->GetDesc( rszName );
		}
		else
		{
			CManipulatorMap::const_iterator posManipulator = manipulatorMap.begin();
			if ( posManipulator->second )
			{
				return posManipulator->second->GetDesc( rszName );
			}
		}
	}
	return 0;
}


bool CMultiManipulator::GetType( const string &rszName, string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMultiManipulator::GetType(): pszType == 0" );
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetType( rszName, pszType );
	}
	else if ( TypeExists( rszName ) )
	{
		if ( pPropertyDescManipulator != 0 )
		{
			return pPropertyDescManipulator->GetType( rszName, pszType );
		}
		else
		{
			CManipulatorMap::const_iterator posManipulator = manipulatorMap.begin();
			if ( posManipulator->second )
			{
				return posManipulator->second->GetType( rszName, pszType );
			}
		}
	}
	return false;
}


UINT CMultiManipulator::GetID( const string &rszName ) const
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetID( rszName );
	}
	else if ( IDExists( rszName ) )
	{
		if ( pPropertyDescManipulator != 0 )
		{
			return pPropertyDescManipulator->GetID( rszName );
		}
		else
		{
			CManipulatorMap::const_iterator posManipulator = manipulatorMap.begin();
			if ( posManipulator->second )
			{
				return posManipulator->second->GetID( rszName );
			}
		}
	}
	return INVALID_NODE_ID;
}


bool CMultiManipulator::GetName( UINT nID, string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMultiManipulator::GetName(): pszName == 0" );
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetName( nID, pszName );
	}
	else if ( NameExists( nID ) )
	{
		if ( pPropertyDescManipulator != 0 )
		{
			return pPropertyDescManipulator->GetName( nID, pszName );
		}
		else
		{
			CManipulatorMap::const_iterator posManipulator = manipulatorMap.begin();
			if ( posManipulator->second )
			{
				return posManipulator->second->GetName( nID, pszName );
			}
		}
	}
	return false;
}


bool CMultiManipulator::InsertNode( const string &rszName, int nNodeIndex )
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->InsertNode( rszName, nNodeIndex );
	}
	else
	{
		if ( NameExists( rszName ) )
		{
			if ( nNodeIndex == NODE_ADD_INDEX )
			{
				nNodeIndex = GetMinimalCount( rszName, 0 );
			}
			//
			bool bResult = true;
			for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
			{
				if ( !itManipulator->second->InsertNode( rszName, nNodeIndex ) )
				{
					bResult = false;
				}
			}
			return bResult;
		}
		return false;
	}
}


bool CMultiManipulator::RemoveNode( const string &rszName, int nNodeIndex )
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->RemoveNode( rszName, nNodeIndex );
	}
	else
	{
		if ( NameExists( rszName ) )
		{
			bool bResult = true;
			if ( nNodeIndex == NODE_REMOVEALL_INDEX )
			{
				bool bMultiVariant = false;
				int nMinimalCount = GetMinimalCount( rszName, &bMultiVariant );
				if ( nMinimalCount > 0 )
				{
					if ( bMultiVariant )
					{
						for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
						{
							CVariant nodeCount = 0;
							itManipulator->second->GetValue( rszName, &nodeCount );
							if ( (int)nodeCount == nMinimalCount )
							{
								if ( !itManipulator->second->RemoveNode( rszName, nNodeIndex ) )
								{
									bResult = false;
								}
							}
							else
							{
								for ( int nLocalNodeIndex = 0; nLocalNodeIndex < nMinimalCount; ++nLocalNodeIndex )
								{
									if ( !itManipulator->second->RemoveNode( rszName, 0 ) )
									{
										bResult = false;
									}
								}
							}
						}
						return bResult;
					}
				}
				else
				{
					return true;
				}
			}
			for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
			{
				if ( !itManipulator->second->RemoveNode( rszName, nNodeIndex ) )
				{
					bResult = false;
				}
			}
			return bResult;
		}
		return false;
	}
}


bool CMultiManipulator::RenameNode( const string &rszName, const string &rszNewName )
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->RenameNode( rszName, rszNewName );
	}
	else
	{
		if ( NameExists( rszName ) )
		{
			bool bResult = true;
			for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
			{
				if ( !itManipulator->second->RenameNode( rszName, rszNewName ) )
				{
					bResult = false;
				}
			}
			return bResult;
		}
		return false;
	}
}


bool CMultiManipulator::GetValue( const string &rszName, CVariant *pValue ) const
{
	NI_ASSERT( pValue != 0, "CMultiManipulator::GetValue(): pValue == 0" );
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetValue( rszName, pValue );
	}
	else
	{
		if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( GetDesc( rszName ) ) )
		{
			if ( typePCIEMnemonics.Get( pDesc, rszName ) == PCIE_LIST )
			{
				( *pValue ) = GetMinimalCount( rszName, 0 );
				return true;
			}
		}
		if ( NameExists( rszName ) )
		{
			return GetMultiValue( rszName, pValue );
		}
		return false;
	}
}


bool CMultiManipulator::SetValue( const string &rszName, const CVariant &rValue )
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->SetValue( rszName, rValue );
	}
	else
	{
		if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc *>( GetDesc( rszName ) ) )
		{
			if ( !typePCIEMnemonics.IsLeaf( typePCIEMnemonics.Get( pDesc, rszName ) ) )
			{
				return false;
			}
		}
		if ( NameExists( rszName ) )
		{
			return SetMultiValue( rszName, rValue );
		}
		return false;
	}
}


bool CMultiManipulator::CheckValue( const string &rszName, const CVariant &rValue, bool *pResult ) const
{
	NI_ASSERT( pResult != 0, "CMultiManipulator::CheckValue(): pResult == 0" );
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->CheckValue( rszName, rValue, pResult );
	}
	else
	{
		if ( NameExists( rszName ) )
		{
			return CheckMultiValue( rszName, rValue, pResult );
		}
		return false;
	}
}


NDb::IObjMan* CMultiManipulator::GetObjMan()
{
	if ( pActiveManipulator != 0 )
	{
		return pActiveManipulator->GetObjMan();
	}
	else
	{
		if ( manipulatorMap.size() == 1 )
		{
			if( manipulatorMap.begin()->second )
			{
				return manipulatorMap.begin()->second->GetObjMan();
			}
		}
	}
	return 0;
}


bool CMultiManipulator::IsNameExists( const string &rszName ) const
{
	if ( pActiveManipulator != 0 )
	{
			return pActiveManipulator->IsNameExists( rszName );
	}
	else
	{
		return NameExists( rszName );
	}
}


void CMultiManipulator::GetNameList( IManipulator::CNameMap *pNameMap ) const
{
	if ( pNameMap )
	{
		for ( CManipulatorMap::const_iterator itManipulator = manipulatorMap.begin(); itManipulator != manipulatorMap.end(); ++itManipulator )
		{
			itManipulator->second->GetNameList( pNameMap );
		}
	}
}


CMultiManipulatorIterator::CMultiManipulatorIterator( CMultiManipulator *_pMultiManipulator, bool bShowHidden, ECacheType eCache ) : pMultiManipulator( _pMultiManipulator )  
{
	NI_ASSERT( pMultiManipulator != 0, "CMultiManipulatorIterator::CMultiManipulatorIterator(): pMultiManipulator == 0" );
	pManipulatorIterator = 0;
	if ( pMultiManipulator->pActiveManipulator != 0 )
	{
		pManipulatorIterator = pMultiManipulator->pActiveManipulator->Iterate( bShowHidden, eCache );
	}
	else if ( pMultiManipulator->pPropertyDescManipulator != 0 )
	{
		pManipulatorIterator = pMultiManipulator->pPropertyDescManipulator->Iterate( bShowHidden, eCache );
	}
	else
	{
		CMultiManipulator::CManipulatorMap::iterator posManipulator = pMultiManipulator->manipulatorMap.begin();
		if ( posManipulator->second )
		{
			pManipulatorIterator = posManipulator->second->Iterate( bShowHidden, eCache );
		}
	}
}


bool CMultiManipulatorIterator::Next()
{
	if ( pManipulatorIterator != 0 )
	{
		if ( pMultiManipulator->pActiveManipulator != 0 )
		{
			return pManipulatorIterator->Next();
		}
		else
		{
			string szName;
			while( pManipulatorIterator->Next() )
			{
				if ( pManipulatorIterator->IsEnd() )
				{
					return true;
				}
				if ( pManipulatorIterator->GetName( &szName ) )
				{
					if ( pMultiManipulator->NameExists( szName ) )
					{
						return true;
					}
				}
				else
				{
					return false;
				}
			}
		}
	}
	return false;
}


bool CMultiManipulatorIterator::IsEnd() const
{
	if ( pManipulatorIterator != 0 )
	{
		return pManipulatorIterator->IsEnd();
	}
	return true;
}


const SIteratorDesc* CMultiManipulatorIterator::GetDesc() const
{
	if ( pManipulatorIterator != 0 )
	{
		return pManipulatorIterator->GetDesc();
	}
	return 0;
}


bool CMultiManipulatorIterator::GetName( string *pszName ) const
{
	NI_ASSERT( pszName != 0, "CMultiManipulatorIterator::GetName(): pszName == 0" );
	if ( pManipulatorIterator != 0 )
	{
		return pManipulatorIterator->GetName( pszName );
	}
	return false;
}


bool CMultiManipulatorIterator::GetType( string *pszType ) const
{
	NI_ASSERT( pszType != 0, "CMultiManipulatorIterator::GetType(): pszType == 0" );
	if ( pManipulatorIterator != 0 )
	{
		return pManipulatorIterator->GetType( pszType );
	}
	return false;
}


UINT CMultiManipulatorIterator::GetID() const
{
	if ( pManipulatorIterator != 0 )
	{
		return pManipulatorIterator->GetID();
	}
	return INVALID_NODE_ID;
}

// basement storage  


