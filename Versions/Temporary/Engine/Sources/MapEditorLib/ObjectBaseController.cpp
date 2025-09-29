#include "StdAfx.h"

#include "PCIEMnemonics.h"
#include "ObjectBaseController.h"
#include "MultiManipulator.h"

#include "../libdb/ResourceManager.h"
#include "../libdb/ObjMan.h"
#include "../libdb/ObjManIterator.h"
#include "..\Misc\HPTimer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CObjectBaseController::SUndoData::FillLists( const string &szStartNodeName, IManipulator *pObjectManipulator )
{
	NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddInsertOperation() pObjectManipulator == 0" );
	//
	//NHPTimer::STime time = 0;
	//NHPTimer::GetTime( &time );
	//
	NDb::IObjMan *pObjMan = pObjectManipulator->GetObjMan();
	CObj<NDb::IObjMan> pSubObjMan;
	if ( pObjMan )
	{
		pSubObjMan = pObjMan->CreateManipulator( szStartNodeName );
	}
	if ( pSubObjMan )
	{
		if ( CObj<NDb::IObjManIterator> pSubObjManIterator = pSubObjMan->CreateIterator( true ) )
		{
			while ( !pSubObjManIterator->IsEnd() )
			{
				string szName = szStartNodeName + LEVEL_SEPARATOR_CHAR + pSubObjManIterator->GetName();
				if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pObjectManipulator->GetDesc( szName ) ) )
				{
					EPCIEType nType = typePCIEMnemonics.Get( pDesc, szName );
					if ( nType == PCIE_LIST )
					{
						int nNodeCount = 0;
						if ( CManipulatorManager::GetValue( &nNodeCount, pObjectManipulator, szName ) )
						{
							CArrayDataList::iterator posArrayData = arrayList.insert( arrayList.end(), SArrayData() );		
							posArrayData->szName = szName;
							posArrayData->nCount = nNodeCount;
						}
					}
					else if ( nType != PCIE_STRUCT )
					{
						CValueDataList::iterator posValueData = valueList.insert( valueList.end(), SValueData() );		
						posValueData->szName = szName;
						pObjectManipulator->GetValue( posValueData->szName, &( posValueData->value ) );
					}
				}
				pSubObjManIterator->Next();
			}
		}
	}
	else
	{
		if ( CPtr<IManipulatorIterator> pObjectManipulatorIterator = pObjectManipulator->Iterate( true, ECT_NO_CACHE ) )
		{
			bool bFound = false;
			while ( !pObjectManipulatorIterator->IsEnd() )
			{
				string szName;
				pObjectManipulatorIterator->GetName( &szName );
				if ( szName.compare( 0, szStartNodeName.size(), szStartNodeName ) == 0 )
				{
					bFound = true;
					if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pObjectManipulatorIterator->GetDesc() ) )
					{
						EPCIEType nType = typePCIEMnemonics.Get( pDesc, szName );
						if ( nType == PCIE_LIST )
						{
							int nNodeCount = 0;
							if ( CManipulatorManager::GetValue( &nNodeCount, pObjectManipulator, szName ) )
							{
								CArrayDataList::iterator posArrayData = arrayList.insert( arrayList.end(), SArrayData() );		
								posArrayData->szName = szName;
								posArrayData->nCount = nNodeCount;
							}
						}
						else if ( nType != PCIE_STRUCT )
						{
							CValueDataList::iterator posValueData = valueList.insert( valueList.end(), SValueData() );		
							posValueData->szName = szName;
							pObjectManipulator->GetValue( posValueData->szName, &( posValueData->value ) );
						}
					}
				}
				else if ( bFound )
				{
					break;
				}
				pObjectManipulatorIterator->Next();
			}
		}
	}
	//DebugTrace( "FillLists() %s: %g", szStartNodeName.c_str(), NHPTimer::GetTimePassed( &time ) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::SUndoData::Undo( IManipulator *pObjectManipulator, const IManipulator::CNameMap *pNameMap ) const
{
	bool bResult = true;
	switch ( eType )
	{
		///////////////////////////////////////////
		case TYPE_INSERT:
			//DebugTrace( "Undo: TYPE_INSERT %s %d", szName.c_str(), (int)( oldValue ) );
			if ( pNameMap && !pNameMap->empty() )
			{
				for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
				{
					if ( !pObjectManipulator->RemoveNode( itNamePrefix->first + szName, (int)( oldValue ) ) )
					{
						bResult = false;
					}
				}
			}
			else
			{
				bResult = pObjectManipulator->RemoveNode( szName, (int)( oldValue ) );
			}
			break;
		///////////////////////////////////////////
		case TYPE_REMOVE:
			//DebugTrace( "Undo: TYPE_REMOVE %s  %d", szName.c_str(), (int)( newValue ) );
			if ( (int)( newValue ) != NODE_REMOVEALL_INDEX )
			{
				if ( pNameMap && !pNameMap->empty() )
				{
					for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
					{
						if ( !pObjectManipulator->InsertNode( itNamePrefix->first + szName, (int)( oldValue ) ) )
						{
							bResult = false;
						}
					}
				}
				else
				{
					bResult = pObjectManipulator->InsertNode( szName, (int)( oldValue ) );
				}
			}
			//создаем заново массивы
			for ( CArrayDataList::const_iterator itArrayData = arrayList.begin(); itArrayData != arrayList.end(); ++itArrayData )
			{
				for ( int nNodeIndex = 0; nNodeIndex < itArrayData->nCount; ++nNodeIndex )
				{
					if ( pNameMap && !pNameMap->empty() )
					{
						for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
						{
							if ( !pObjectManipulator->InsertNode( itNamePrefix->first + itArrayData->szName, 0 ) )
							{
								bResult = false;
							}
						}
					}
					else
					{
						bResult = pObjectManipulator->InsertNode( itArrayData->szName, 0 );
					}
					if ( !bResult )
					{
						break;
					}
				}
			}
			//проставляем удаленные элементы
			for ( CValueDataList::const_iterator itValueData = valueList.begin(); itValueData != valueList.end(); ++itValueData )
			{
				if ( pNameMap && !pNameMap->empty() )
				{
					for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
					{
						if ( !pObjectManipulator->SetValue( itNamePrefix->first + itValueData->szName, itValueData->value ) )
						{
							bResult = false;
						}
					}
				}
				else
				{
					bResult = pObjectManipulator->SetValue( itValueData->szName, itValueData->value );
				}
				if ( !bResult )
				{
					break;
				}
			}
			break;
		///////////////////////////////////////////
		case TYPE_CHANGE:
			//DebugTrace( "Undo: TYPE_CHANGE %s", szName.c_str() );
			if ( pNameMap && !pNameMap->empty() )
			{
				for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
				{
					if ( !pObjectManipulator->SetValue( itNamePrefix->first + szName.c_str(), oldValue ) )
					{
						bResult = false;
					}
				}
			}
			else
			{
				bResult = pObjectManipulator->SetValue( szName.c_str(), oldValue );	
			}
			break;
		///////////////////////////////////////////
		case TYPE_EXPAND:
			bResult = true;
			break;
		///////////////////////////////////////////
		default:
		bResult = false;
		break;
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::SUndoData::Redo( IManipulator *pObjectManipulator, const IManipulator::CNameMap *pNameMap ) const
{
	bool bResult = true;
	switch ( eType )
	{
		///////////////////////////////////////////
		case TYPE_INSERT:
			//DebugTrace( "Redo: TYPE_INSERT %s %d", szName.c_str(), (int)( newValue ) );
			if ( pNameMap && !pNameMap->empty() )
			{
				for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
				{
					if ( !pObjectManipulator->InsertNode( itNamePrefix->first + szName, (int)( newValue ) ) )
					{
						bResult = false;
					}
				}
			}
			else
			{
				bResult = pObjectManipulator->InsertNode( szName, (int)( newValue ) );
			}
			break;
		///////////////////////////////////////////
		case TYPE_REMOVE:
			//DebugTrace( "Redo: TYPE_REMOVE %s  %d", szName.c_str(), (int)( newValue ) );
			if ( pNameMap && !pNameMap->empty() )
			{
				for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
				{
					if ( !pObjectManipulator->RemoveNode( itNamePrefix->first + szName, (int)( newValue ) ) )
					{
						bResult = false;
					}
				}
			}
			else
			{
				bResult = pObjectManipulator->RemoveNode( szName, (int)( newValue ) );
			}
			break;
		///////////////////////////////////////////
		case TYPE_CHANGE:
			if ( pNameMap && !pNameMap->empty() )
			{
				for ( IManipulator::CNameMap::const_iterator itNamePrefix = pNameMap->begin(); itNamePrefix != pNameMap->end(); ++itNamePrefix )
				{
					if ( !pObjectManipulator->SetValue( itNamePrefix->first + szName.c_str(), newValue ) )
					{
						bResult = false;
					}
				}
			}
			else
			{
				bResult = pObjectManipulator->SetValue( szName.c_str(), newValue );	
			}
			break;
		///////////////////////////////////////////
		case TYPE_EXPAND:
			bResult = true;
			break;
		///////////////////////////////////////////
		default:
			bResult = false;
			break;
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CObjectBaseController::Trace() const
{
	for ( CUndoDataList::const_iterator posUndoData = undoDataList.begin(); posUndoData != undoDataList.end(); ++posUndoData )
	{
		switch ( posUndoData->eType )
		{
			///////////////////////////////////////////
			case SUndoData::TYPE_INSERT:
				DebugTrace( "Undo: TYPE_INSERT name: %s, new: %d, old: %d", posUndoData->szName.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
				break;
			///////////////////////////////////////////
			case SUndoData::TYPE_REMOVE:
				DebugTrace( "Undo: TYPE_REMOVE name: %s, new: %d, old: %d", posUndoData->szName.c_str(), (int)( posUndoData->newValue ), (int)( posUndoData->oldValue ) );
				break;
			///////////////////////////////////////////
			case SUndoData::TYPE_CHANGE:
				DebugTrace( "Redo: TYPE_CHANGE name: %s", posUndoData->szName.c_str() );
				break;
			///////////////////////////////////////////
			default:
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::IsAbsolute() const
{
	if ( !undoDataList.empty() )
	{
		if ( NGlobal::GetVar( "disable_remove_undo", 0 ) > 0 )
		{
			for ( CObjectBaseController::CUndoDataList::const_iterator itUndoData = undoDataList.begin(); itUndoData != undoDataList.end(); ++itUndoData )
			{
				if ( itUndoData->eType == SUndoData::TYPE_REMOVE )
				{
					return true;
				}
			}
		}
	}
	return false;
}

//CRAP{ PLAIN_TEXT
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CObjectBaseController::GetDescription( CString *pstrDescription ) const
{
	if ( pstrDescription )
	{
		pstrDescription->Empty();
		CDefaultController::GetDescription( pstrDescription );
		if ( pstrDescription->IsEmpty() )
		{
			const int nSize = undoDataList.size();
			
			if ( nSize == 1 )
			{
				const SUndoData &rUndoData = undoDataList.front();
				switch( rUndoData.eType )
				{
					case SUndoData::TYPE_INSERT:
						if ( (int)rUndoData.newValue >= 0 )
						{
							pstrDescription->Format( "Insert [%d]", (int)rUndoData.newValue );
						}
						else
						{
							pstrDescription->Format( "Add", rUndoData.szName );
						}
						return;
					case SUndoData::TYPE_REMOVE:
						if ( (int)rUndoData.newValue >= 0 )
						{
							pstrDescription->Format( "Delete [%d]", (int)rUndoData.newValue );
						}
						else
						{
							pstrDescription->Format( "Delete all" );
						}
						return;
					case SUndoData::TYPE_CHANGE:
					{	string szText;
						rUndoData.newValue.ToText( &szText );
						pstrDescription->Format( "Set to %s", szText.c_str() );
						return;
					}
					default:
						pstrDescription->Format( "Unknown" );
						return;
				}
			}
			else
			{
				pstrDescription->Format( "%d operations", nSize );
			}
		}
	}
}
//CRAP} PLAIN_TEXT

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::UndoWithoutUpdateViews()
{
	bool bResult = true;
	if ( !undoDataList.empty() )
	{
		if ( !GetObjectSet().objectNameSet.empty() )
		{
			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			CPtr<CMultiManipulator> pMultiManipulator = new CMultiManipulator();
			for ( CObjectNameSet::const_iterator itObjectName = GetObjectSet().objectNameSet.begin(); itObjectName != GetObjectSet().objectNameSet.end(); ++itObjectName )
			{
				pMultiManipulator->InsertManipulator( itObjectName->first, pResourceManager->CreateObjectManipulator( GetObjectSet().szObjectTypeName, itObjectName->first ), false, false );
			}
			for ( CObjectBaseController::CUndoDataList::const_iterator itUndoData = undoDataList.end(); itUndoData != undoDataList.begin(); )
			{
				--itUndoData;
				bResult = itUndoData->Undo( pMultiManipulator, &GetNameList() );
				if ( !bResult )
				{
					break;
				}
			}
		}
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::RedoWithoutUpdateViews()
{
	bool bResult = true;
	if ( !undoDataList.empty() )
	{
		if ( !GetObjectSet().objectNameSet.empty() )
		{
			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			CPtr<CMultiManipulator> pMultiManipulator = new CMultiManipulator();
			for ( CObjectNameSet::const_iterator itObjectName = GetObjectSet().objectNameSet.begin(); itObjectName != GetObjectSet().objectNameSet.end(); ++itObjectName )
			{
				pMultiManipulator->InsertManipulator( itObjectName->first, pResourceManager->CreateObjectManipulator( GetObjectSet().szObjectTypeName, itObjectName->first ), false, false );
			}
			for ( CObjectBaseController::CUndoDataList::const_iterator itUndoData = undoDataList.begin(); itUndoData != undoDataList.end(); ++itUndoData )
			{
				bResult = itUndoData->Redo( pMultiManipulator, &GetNameList() );
				if ( !bResult )
				{
					break;
				}
			}
		}
	}
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::AddInsertOperation( const string &rszArrayName, const int nIndex, IManipulator *pObjectManipulator )
{
	NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddInsertOperation() pObjectManipulator == 0" );
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	int nNodeCount = 0;
	CManipulatorManager::GetValue( &nNodeCount, pObjectManipulator, rszArrayName );
	//
	//DebugTrace( "AddInsertOperation(), CManipulatorManager::GetValue(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	posNewUndoData->eType = SUndoData::TYPE_INSERT;
	posNewUndoData->szName = rszArrayName;
	if ( ( nIndex == NODE_ADD_INDEX ) || ( nIndex >= nNodeCount ) )
	{
		posNewUndoData->newValue = NODE_ADD_INDEX;
		posNewUndoData->oldValue = nNodeCount;
	}
	else
	{
		posNewUndoData->newValue = nIndex;
		posNewUndoData->oldValue = nIndex;
	}
	//
	//DebugTrace( "AddInsertOperation(), CManipulatorManager::FillData(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	const bool bResult = posNewUndoData->Redo( pObjectManipulator, 0 );
	//
	//DebugTrace( "AddInsertOperation(), CManipulatorManager::Redo(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::AddRemoveOperation( const string &rszArrayName, const int nIndex, IManipulator *pObjectManipulator )
{
	NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddInsertOperation() pObjectManipulator == 0" );
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	int nNodeCount = 0;
	CManipulatorManager::GetValue( &nNodeCount, pObjectManipulator, rszArrayName );
	//
	//DebugTrace( "AddRemoveOperation(), CManipulatorManager::GetValue(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	int nIndexToDelete = nIndex;
	if ( nIndexToDelete >= nNodeCount )
	{
		nIndexToDelete = nNodeCount - 1;
	}
	//
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	posNewUndoData->eType = CObjectBaseController::SUndoData::TYPE_REMOVE;
	posNewUndoData->szName = rszArrayName;
	posNewUndoData->newValue = nIndexToDelete;
	if ( nIndexToDelete == NODE_REMOVEALL_INDEX )
	{
		if ( NGlobal::GetVar( "disable_remove_undo", 0 ) == 0 )
		{
			posNewUndoData->FillLists( posNewUndoData->szName, pObjectManipulator );
		}
	}
	else
	{
		if ( nIndexToDelete == ( nNodeCount - 1 ) )
		{
			posNewUndoData->oldValue = NODE_ADD_INDEX;
		}
		else
		{
			posNewUndoData->oldValue = nIndex;
		}
		if ( NGlobal::GetVar( "disable_remove_undo", 0 ) == 0 )
		{
			const string szStartNodeName = StrFmt( "%s%c%c%d%c",
																						posNewUndoData->szName.c_str(),
																						LEVEL_SEPARATOR_CHAR,
																						ARRAY_NODE_START_CHAR,
																						nIndexToDelete,
																						ARRAY_NODE_END_CHAR );
			posNewUndoData->FillLists( szStartNodeName, pObjectManipulator );
		}
	}
	//
	//DebugTrace( "AddRemoveOperation(), posNewUndoData->FillData(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	const bool bResult = posNewUndoData->Redo( pObjectManipulator, 0 );
	//
	//DebugTrace( "AddRemoveOperation(), posNewUndoData->Redo(): %g", NHPTimer::GetTimePassed( &time ) );
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::AddChangeOperation( const string &rszPropertyName, const CVariant &rValue, IManipulator *pObjectManipulator )
{
	NI_ASSERT( pObjectManipulator != 0, "CObjectBaseController::AddInsertOperation() pObjectManipulator == 0" );
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	CVariant propertyValue;
	if ( !pObjectManipulator->GetValue( rszPropertyName, &propertyValue ) )
	{
		return false;
	}
	//
	//DebugTrace( "AddChangeOperation(), CManipulatorManager::GetValue(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	posNewUndoData->eType = SUndoData::TYPE_CHANGE;
	posNewUndoData->szName = rszPropertyName;
	posNewUndoData->newValue = rValue;
	posNewUndoData->oldValue = propertyValue;
	//
	//DebugTrace( "AddChangeOperation(), posNewUndoData->FillData(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	const bool bResult = posNewUndoData->Redo( pObjectManipulator, 0 );
	//
	//DebugTrace( "AddChangeOperation(), posNewUndoData->Redo(): %g", NHPTimer::GetTimePassed( &time ) );
	return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CObjectBaseController::AddExpandOperation( const string &rszPropertyName, bool bExpand, IManipulator *pObjectManipulator )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	posNewUndoData->eType = SUndoData::TYPE_EXPAND;
	posNewUndoData->szName = rszPropertyName;
	posNewUndoData->newValue = bExpand;
	//
	return posNewUndoData->Redo( pObjectManipulator, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
