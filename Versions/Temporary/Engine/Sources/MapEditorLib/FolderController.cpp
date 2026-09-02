#include "stdafx.h"

#include "FolderController.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"

#include "port/time.h"

#include <cstdint>

bool CFolderController::UndoWithoutUpdateViews()
{
	if ( undoDataList.empty() )
	{
		return true;
	}
	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( GetObjectSet().szObjectTypeName );
	if ( pFolderManipulator == 0 )
	{
		return true;
	}
	bool bResult = true;
	for ( CFolderController::CUndoDataList::const_iterator itUndoData = undoDataList.begin(); itUndoData != undoDataList.end(); ++itUndoData )
	{
		switch ( itUndoData->eType )
		{
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_INSERT:
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_REMOVE:
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_COPY:
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_RENAME:
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_COLOR:
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_EXPAND:
				break;
			///////////////////////////////////////////
			default:
				bResult = false;
		}
	}
	return bResult;
}


bool CFolderController::RedoWithoutUpdateViews()
{
	if ( undoDataList.empty() )
	{
		return true;
	}
	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( GetObjectSet().szObjectTypeName );
	if ( pFolderManipulator == 0 )
	{
		return true;
	}
	bool bResult = true;
	for ( CFolderController::CUndoDataList::const_iterator itUndoData = undoDataList.begin(); itUndoData != undoDataList.end(); ++itUndoData )
	{
		uint32_t dwTime = GetCurrentTimeMilliseconds();
		switch ( itUndoData->eType )
		{
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_INSERT:
				if ( !itUndoData->szDestination.empty() )
				{
					bResult = pFolderManipulator->InsertNode( itUndoData->szDestination );
				}
				else
				{
					bResult = false;
				}
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_REMOVE:
				if ( !itUndoData->szDestination.empty() )
				{
					bResult = pFolderManipulator->RemoveNode( itUndoData->szDestination );
				}
				else
				{
					bResult = false;
				}
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_COPY:
				if ( ( !itUndoData->szDestination.empty() ) &&
							( !itUndoData->szSource.empty() ) )
				{
					if ( itUndoData->szDestination != itUndoData->szSource )
					{
						bResult = pFolderManipulator->IsNameExists( itUndoData->szDestination );
						if ( !bResult )
							bResult = pFolderManipulator->InsertNode( itUndoData->szDestination );
						if ( bResult )
						{
							if ( ( itUndoData->szDestination[itUndoData->szDestination.size() - 1] != PATH_SEPARATOR_CHAR ) &&
								   ( itUndoData->szSource[itUndoData->szSource.size() - 1] != PATH_SEPARATOR_CHAR ) )
							{
								CPtr<IManipulator> pDestinationManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, itUndoData->szDestination );
								CPtr<IManipulator> pSourceManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, itUndoData->szSource );
								bResult = CManipulatorManager::CloneDBManipulator( pDestinationManipulator, pSourceManipulator, true );
							}
						}
					}
				}
				else
				{
					bResult = false;
				}
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_RENAME:
				if ( ( !itUndoData->szDestination.empty() ) &&
							( !itUndoData->szSource.empty() ) )
				{
					if ( itUndoData->szDestination != itUndoData->szSource )
					{
						bResult = pFolderManipulator->RenameNode( itUndoData->szSource, itUndoData->szDestination );
					}
				}
				else
				{
					bResult = false;
				}
				break;
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_COLOR:
			{
				if ( !itUndoData->szDestination.empty() )
				{
					bResult = pFolderManipulator->SetValue( itUndoData->szDestination, itUndoData->newValue );
				}
				else
				{
					bResult = false;
				}
				break;
			}
			///////////////////////////////////////////
			case CFolderController::SUndoData::TYPE_EXPAND:
				break;
			///////////////////////////////////////////
			default:
				bResult = false;
		}
		dwTime = GetCurrentTimeMilliseconds() - dwTime;
		DebugTrace( "CFolderController: operation:%d, type: <%s>, dest:<%s>, source:<%s>, result:%s, time: %dmc",
								static_cast<int>( itUndoData->eType ),
								GetObjectSet().szObjectTypeName.c_str(),
								itUndoData->szDestination.c_str(),
								itUndoData->szSource.c_str(),
								bResult ? "true" : "false",
								dwTime );
	}
	return bResult;
}


bool CFolderController::AddInsertOperation( const std::string &rszObjectName )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = SUndoData::TYPE_INSERT;
	posNewUndoData->szDestination = rszObjectName;
	//
	return true;
}


bool CFolderController::AddRemoveOperation( const std::string &rszObjectName )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = CFolderController::SUndoData::TYPE_REMOVE;
	posNewUndoData->szDestination = rszObjectName;
	//
	return true;
}


bool CFolderController::AddCopyOperation( const std::string &rszDestination, const std::string &rszSource )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = CFolderController::SUndoData::TYPE_COPY;
	posNewUndoData->szSource = rszSource;
	posNewUndoData->szDestination = rszDestination;
	//
	return true;
}


bool CFolderController::AddRenameOperation( const std::string &rszDestination, const std::string &rszSource, bool bNewHTREEITEM )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = CFolderController::SUndoData::TYPE_RENAME;
	posNewUndoData->szSource = rszSource;
	posNewUndoData->szDestination = rszDestination;
	posNewUndoData->newValue = bNewHTREEITEM;
	//
	return true;
}


bool CFolderController::AddColorOperation( const std::string &rszObjectName, int nNewColor )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = SUndoData::TYPE_COLOR;
	posNewUndoData->szDestination = rszObjectName;
	posNewUndoData->newValue = nNewColor;
	//
	return true;
}


bool CFolderController::AddExpandOperation( const std::string &rszObjectName, bool bExpand )
{
	CUndoDataList::iterator posNewUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
	//
	posNewUndoData->eType = SUndoData::TYPE_EXPAND;
	posNewUndoData->szDestination = rszObjectName;
	posNewUndoData->newValue = bExpand;
	//
	return true;
}

// basement storage  


