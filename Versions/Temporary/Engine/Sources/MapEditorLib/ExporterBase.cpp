#include "StdAfx.h"

#include "libdb/ResourceManager.h"
#include "ManipulatorManager.h"
#include "StringManager.h"
#include "ExporterBase.h"
#include "PCIEMnemonics.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_Editor.h"
#include "System/Commands.h"

static bool s_bReportSafeRefs = true;

EXPORT_RESULT CExporterBase::GetStartExportResult( const string &rszObjectTypeName )
{
	CResultMap::const_iterator posResult = startExportResultMap.find( rszObjectTypeName );
	if ( posResult == startExportResultMap.end() )
	{
		return ER_UNKNOWN;
	}
	else
	{
		return posResult->second;
	}
}


void CExporterBase::SetStartExportResult( const string &rszObjectTypeName, EXPORT_RESULT eResult )
{
	startExportResultMap[rszObjectTypeName] = eResult;
}


EXPORT_RESULT CExporterBase::GetExportObjectResult( const string &rszObjectRefName )
{
	CResultMap::const_iterator posResult = exportObjectResultMap.find( rszObjectRefName );
	if ( posResult == exportObjectResultMap.end() )
	{
		return ER_UNKNOWN;
	}
	else
	{
		return posResult->second;
	}
}


void CExporterBase::SetExportObjectResult( const string &rszObjectRefName, EXPORT_RESULT eResult )
{
	exportObjectResultMap[rszObjectRefName] = eResult;
}


// Собираем новые типы требующие вызова StartExport и FinishExport
// Дополнительно запоминаем тип и имя объекта ( может быть не указан в имени свойства )
bool CExporterBase::GetObjectTypeNameSet( IManipulator* pManipulator,
																					const string &rszObjectTypeName,
																					const string &rszObjectName,
																					CObjectTypeNameList *pObjectTypeNameList,
																					CExportObjectTypeNameMap *pExportObjectTypeNameMap,
																					CExportObjectNameMap *pExportObjectNameMap,
																					CInvalidLinkList* pInvalidLinkList )
{
  NI_ASSERT( pManipulator != 0, "GetObjectTypeNameSet() pManipulator == 0" );
  NI_ASSERT( pObjectTypeNameList != 0, "GetObjectTypeNameSet() pObjectTypeNameList == 0" );
  NI_ASSERT( pExportObjectTypeNameMap != 0, "GetObjectTypeNameSet() pExportObjectTypeNameMap == 0" );
  NI_ASSERT( pExportObjectNameMap != 0, "GetObjectTypeNameSet() pExportObjectNameMap == 0" );

	if ( CPtr<IManipulatorIterator> pManipulatorIterator = pManipulator->Iterate( true, ECT_CACHE_GLOBAL ) )
	{
		string szName;
		while ( !pManipulatorIterator->IsEnd() )
		{
			szName.clear();
			pManipulatorIterator->GetName( &szName );
			if ( const SPropertyDesc *pDesc = dynamic_cast<const SPropertyDesc*>( pManipulator->GetDesc( szName ) ) )
			{
				EPCIEType nType = typePCIEMnemonics.Get( pDesc, szName );
				if ( typePCIEMnemonics.IsRef( nType ) )
				{
					string szRef;
					CManipulatorManager::GetValue( &szRef, pManipulator, szName );
					if ( !szRef.empty() )
					{
						string szObjectTypeName;
						string szObjectName;
						CStringManager::GetTypeAndNameFromRefValue( &szObjectTypeName, &szObjectName, szRef, TYPE_SEPARATOR_CHAR, pDesc->refTypes.begin()->first );
						//
						pObjectTypeNameList->Insert( szObjectTypeName, UNIQUE_LIST_INSERT_BACK );
						( *pExportObjectTypeNameMap )[szName] = szObjectTypeName;
						( *pExportObjectNameMap )[szName] = szObjectName;
					}
					else
					{
						if ( !pDesc->bUnsafe )
						{
							if ( pInvalidLinkList != 0 )
							{
								CInvalidLinkList::iterator posInvalidLink = pInvalidLinkList->insert( pInvalidLinkList->end(), SInvalidLink() );
								posInvalidLink->szObjectTypeName = rszObjectTypeName;
								posInvalidLink->szObjectName = rszObjectName;
								posInvalidLink->szPropertyName = szName;
								// CRAP{ PLAIN_TEXT
								if ( s_bReportSafeRefs )
								{
									const string szMessage = StrFmt( "Empty safe ref: Object: %s:%s, Poperty:<%s>\r\n",
																										posInvalidLink->szObjectTypeName.c_str(),
																										posInvalidLink->szObjectName.c_str(),
																										posInvalidLink->szPropertyName.c_str() );
									NLog::GetLogger()->Log( LT_ERROR, szMessage );
								}
								// CRAP} PLAIN_TEXT
							}
						}
					}
				}
			}
			pManipulatorIterator->Next();
		}
	}
	return true;
}


void CExporterBase::InnerStartExport( const CObjectTypeNameList &rNewObjectTypeNameList, bool bExport, bool bForce )
{
	IExporterContainer *pExporterContainer = Singleton<IExporterContainer>();
	for ( list<string>::const_iterator itNewObjectTypeName = rNewObjectTypeNameList.GetList().begin(); itNewObjectTypeName != rNewObjectTypeNameList.GetList().end(); ++itNewObjectTypeName )
	{
		const string &rszObjectTypeName = ( *itNewObjectTypeName );
		if ( GetStartExportResult( rszObjectTypeName ) == ER_UNKNOWN )
		{
			if ( IExporter *pExporter = pExporterContainer->GetExporter( rszObjectTypeName ) )
			{
				bool bResult = true;
				if ( bExport )
				{
					DebugTrace( "CExporterBase::StartExport(): <%s>", rszObjectTypeName.c_str() );
					try
					{
						bResult = pExporter->StartExport( rszObjectTypeName, bForce );
					}
					catch ( ... ) 
					{
						ILogger *pLogger = NLog::GetLogger();
						pLogger->Log( LT_ERROR, StrFmt( "\nStartExport throw exception\n" ) );
						pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
						pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
						bResult = false;
					}
					DebugTrace( "result: %s", bResult ? "true" : "false" );
				}
				if ( bResult )
				{
					DebugTrace( "CExporterBase::StartCheck(): <%s>", rszObjectTypeName.c_str() );
					try
					{
						bResult = pExporter->StartCheck( rszObjectTypeName, bExport );
					}
					catch ( ... ) 
					{
						ILogger *pLogger = NLog::GetLogger();
						pLogger->Log( LT_ERROR, StrFmt( "\nStartCheck throw exception\n" ) );
						pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
						pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
						bResult = false;
					}
					DebugTrace( "result: %s", bResult ? "true" : "false" );
				}
				if ( bResult )
				{
					objectTypeNameList.Insert( rszObjectTypeName, UNIQUE_LIST_INSERT_BACK );
				}
				SetStartExportResult( rszObjectTypeName, ( bResult ? ER_SUCCESS : ER_FAIL ) );
			}
		}
	}
}


void CExporterBase::InnerFinishExport( bool bExport, bool bForce )
{
	IExporterContainer *pExporterContainer = Singleton<IExporterContainer>();
	// Необходимо заканчивать экспорт в обратном порядке - переделываем лист
	list<string> backObjectTypeNameList;
	for ( list<string>::const_iterator itObjectTypeName = objectTypeNameList.GetList().begin(); itObjectTypeName != objectTypeNameList.GetList().end(); ++itObjectTypeName )
	{
		backObjectTypeNameList.push_front( *itObjectTypeName );
	}
	//
	for ( list<string>::const_iterator itObjectTypeName = backObjectTypeNameList.begin(); itObjectTypeName != backObjectTypeNameList.end(); ++itObjectTypeName )
	{
		const string &rszObjectTypeName = ( *itObjectTypeName );
		if ( IExporter *pExporter = pExporterContainer->GetExporter( rszObjectTypeName ) )
		{
			if ( bExport )
			{
				DebugTrace( "CExporterBase::FinishExport(): <%s>", rszObjectTypeName.c_str() );
				try
				{
					pExporter->FinishExport( rszObjectTypeName, bForce );
				}
				catch ( ... ) 
				{
					ILogger *pLogger = NLog::GetLogger();
					pLogger->Log( LT_ERROR, StrFmt( "\nFinishExport throw exception\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
				}
			}
			DebugTrace( "CExporterBase::FinishCheck(): <%s>", rszObjectTypeName.c_str() );
			try
			{
				pExporter->FinishCheck( rszObjectTypeName, bExport );
			}
			catch ( ... ) 
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_ERROR, StrFmt( "\nFinishCheck throw exception\n" ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
			}
		}
	}
	// CRAP{ PLAIN_TEXT
	/*
	if ( !invalidLinkList.empty() )
	{
		string szMessage = "Invalid ref found:\r\n";
		NLog::GetLogger()->Log( LT_ERROR, szMessage );
		for ( CInvalidLinkList::const_iterator itInvalidLink = invalidLinkList.begin(); itInvalidLink != invalidLinkList.end(); ++itInvalidLink )
		{
			szMessage = StrFmt( "Object:<%s:%s>, Poperty:<%s>\r\n",
													itInvalidLink->szObjectTypeName.c_str(),
													itInvalidLink->szObjectName.c_str(),
													itInvalidLink->szPropertyName.c_str() );
			NLog::GetLogger()->Log( LT_ERROR, szMessage );
		}
	}
	*/
	invalidLinkList.clear();
	// CRAP} PLAIN_TEXT
}


EXPORT_RESULT CExporterBase::InnerExportObject( IManipulator* pManipulator,
																								const string &rszObjectTypeName,
																								const string &rszObjectName,
																								bool bExport,
																								bool bForce )
{
	NI_ASSERT( pManipulator != 0, "CExporterBase::InnerExportObject() pManipulator == 0" );
	//
	IExporterContainer *pExporterContainer = Singleton<IExporterContainer>();
	string szObjectRefName;
	CStringManager::GetRefValueFromTypeAndName( &szObjectRefName, rszObjectTypeName, rszObjectName, TYPE_SEPARATOR_CHAR );
	// проверяем что у этого объекта прошел StartExport
	EXPORT_RESULT eResult = GetStartExportResult( rszObjectTypeName );
	if ( eResult == ER_FAIL )
	{
		SetExportObjectResult( szObjectRefName, eResult );
		return ER_FAIL;
	}
	// Проверяем что мы еще не экспортировали этот объект
	eResult = GetExportObjectResult( szObjectRefName );
	if ( eResult == ER_UNKNOWN )
	{
		// Экспортируем обьект до экспорта ссылок
		eResult = ER_SUCCESS;
		if ( IExporter *pExporter = pExporterContainer->GetExporter( rszObjectTypeName ) )
		{
			if ( bExport )
			{
				DebugTrace( "CExporterBase::ExportObject(): <%s:%s>, type: ET_BEFORE_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
				try
				{
					eResult = pExporter->ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, ET_BEFORE_REF );
				}
				catch ( ... ) 
				{
					ILogger *pLogger = NLog::GetLogger();
					pLogger->Log( LT_ERROR, StrFmt( "\nExportObject throw exception\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_BEFORE_REF\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
					eResult = ER_FAIL;
				}
				DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : ( eResult == ER_NOT_CHANGED ) ? "ER_NOT_CHANGED" : "ER_BREAK" );
			}
			if ( eResult == ER_SUCCESS )
			{
				DebugTrace( "CExporterBase::CheckObject(): <%s:%s>, type: ET_BEFORE_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
				try
				{
					eResult = pExporter->CheckObject( pManipulator, rszObjectTypeName, rszObjectName, bExport, ET_BEFORE_REF );
				}
				catch ( ... ) 
				{
					ILogger *pLogger = NLog::GetLogger();
					pLogger->Log( LT_ERROR, StrFmt( "\nCheckObject throw exception\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_BEFORE_REF\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
					eResult = ER_FAIL;
				}
				DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : ( eResult == ER_NOT_CHANGED ) ? "ER_NOT_CHANGED" : "ER_BREAK" );
			}
			SetExportObjectResult( szObjectRefName, eResult );
			if ( eResult == ER_BREAK )
			{
				return eResult;
			}
		}
		// Экспортируем ссылочные типы
		{
			eResult = HierarchyExportObject( pManipulator, rszObjectTypeName, rszObjectName, bExport, bForce );
			if ( eResult == ER_BREAK )
			{
				SetExportObjectResult( szObjectRefName, eResult );
				return eResult;
			}
		}
		// Экспортируем обьект после экспорта ссылок
		if ( IExporter *pExporter = pExporterContainer->GetExporter( rszObjectTypeName ) )
		{
			if ( bExport )
			{
				DebugTrace( "CExporterBase::ExportObject(): <%s:%s>, type: ET_AFTER_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
				try
				{
					eResult = pExporter->ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, ET_AFTER_REF );
				}
				catch ( ... ) 
				{
					ILogger *pLogger = NLog::GetLogger();
					pLogger->Log( LT_ERROR, StrFmt( "\nExportObject throw exception\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_AFTER_REF\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
					eResult = ER_FAIL;
				}
				DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : ( eResult == ER_NOT_CHANGED ) ? "ER_NOT_CHANGED" : "ER_BREAK" );
			}
			if ( eResult == ER_SUCCESS )
			{
				DebugTrace( "CExporterBase::CheckObject(): <%s:%s>, type: ET_AFTER_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
				try
				{
					eResult = pExporter->CheckObject( pManipulator, rszObjectTypeName, rszObjectName, bExport, ET_AFTER_REF );
				}
				catch ( ... ) 
				{
					ILogger *pLogger = NLog::GetLogger();
					pLogger->Log( LT_ERROR, StrFmt( "\nCheckObject throw exception\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_AFTER_REF\n" ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
					pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
					eResult = ER_FAIL;
				}
				DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : ( eResult == ER_NOT_CHANGED ) ? "ER_NOT_CHANGED" : "ER_BREAK" );
			}
			SetExportObjectResult( szObjectRefName, eResult );
		}
	}
	return eResult;
}


EXPORT_RESULT CExporterBase::HierarchyExportObject( IManipulator* pManipulator,
																										const string &rszObjectTypeName,
																										const string &rszObjectName,
																										bool bExport,
																										bool bForce )
{
  NI_ASSERT( pManipulator != 0, "CExporterBase::HierarchyExportObject() pManipulator == 0" );
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	// Получаем список типов ссылок
	CObjectTypeNameList currentObjectTypeNameList;
	CExportObjectTypeNameMap currentExportObjectTypeNameMap;
	CExportObjectNameMap currentExportObjectNameMap;
	GetObjectTypeNameSet( pManipulator,
												rszObjectTypeName,
												rszObjectName,
												&currentObjectTypeNameList,
												&currentExportObjectTypeNameMap,
												&currentExportObjectNameMap,
												( !bExport ) ? &invalidLinkList : 0 );
	InnerStartExport( currentObjectTypeNameList, bExport, bForce );
	// Экпортируем ссылочные поля
	for ( CExportObjectNameMap::const_iterator itCurrentExportObjectName = currentExportObjectNameMap.begin(); itCurrentExportObjectName != currentExportObjectNameMap.end(); ++itCurrentExportObjectName )
	{
		const string szObjectTypeName = currentExportObjectTypeNameMap[itCurrentExportObjectName->first];
		const string szObjectName = itCurrentExportObjectName->second;
		string szObjectRefName;
		CStringManager::GetRefValueFromTypeAndName( &szObjectRefName, szObjectTypeName, szObjectName, TYPE_SEPARATOR_CHAR );
		if ( GetExportObjectResult( szObjectRefName ) == ER_UNKNOWN )
		{
			if ( CPtr<IManipulator> pObjectManpulator = pResourceManager->CreateObjectManipulator( szObjectTypeName, szObjectName ) )
			{
				if ( InnerExportObject( pObjectManpulator, szObjectTypeName, szObjectName, bExport, bForce ) == ER_BREAK )
				{
					return ER_BREAK;
				}
			}
		}
	}
	return ER_SUCCESS;
}


bool CExporterBase::StartExport( const string &rszObjectTypeName, bool bForce )
{
	objectTypeNameList.Clear();
	startExportResultMap.clear();
	exportObjectResultMap.clear();
	invalidLinkList.clear();
	//
	CObjectTypeNameList newObjectTypeNameList;
	newObjectTypeNameList.Insert( rszObjectTypeName, UNIQUE_LIST_INSERT_BACK );
	InnerStartExport( newObjectTypeNameList, true, bForce );
	return true;
}


void CExporterBase::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	InnerFinishExport( true, bForce );
	//
	objectTypeNameList.Clear();
	startExportResultMap.clear();
	exportObjectResultMap.clear();
	invalidLinkList.clear();
	//
	//Singleton<IEditorContainer>()->ReloadActiveEditor( true );
}


EXPORT_RESULT CExporterBase::ExportObject( IManipulator* pManipulator,
																					 const string &rszObjectTypeName,
																					 const string &rszObjectName,
																					 bool bForce,
																					 EXPORT_TYPE exportType )
{
	if ( exportType == ET_NO_REF )
	{
		return InnerExportObject( pManipulator, rszObjectTypeName, rszObjectName, true, bForce );
	}
	return ER_SUCCESS;
}


bool CExporterBase::StartCheck( const string &rszObjectTypeName, bool bExport )
{
	if ( !bExport )
	{
		objectTypeNameList.Clear();
		startExportResultMap.clear();
		exportObjectResultMap.clear();
		invalidLinkList.clear();
		//
		CObjectTypeNameList newObjectTypeNameList;
		newObjectTypeNameList.Insert( rszObjectTypeName, UNIQUE_LIST_INSERT_BACK );
		InnerStartExport( newObjectTypeNameList, false, false );
	}
	return true;
}


void CExporterBase::FinishCheck( const string &rszObjectTypeName, bool bExport )
{
	if ( !bExport )
	{
		InnerFinishExport( false, false );

		objectTypeNameList.Clear();
		startExportResultMap.clear();
		exportObjectResultMap.clear();
		invalidLinkList.clear();
	}
}


EXPORT_RESULT CExporterBase::CheckObject( IManipulator* pManipulator,
																					const string &rszObjectTypeName,
																					const string &rszObjectName,
																					bool bExport,
																					EXPORT_TYPE exportType )
{
	if ( exportType == ET_NO_REF )
	{
		return InnerExportObject( pManipulator, rszObjectTypeName, rszObjectName, false, false );
	}
	return ER_SUCCESS;
}

// basement storage  

START_REGISTER(DefaultExporterCommands)
REGISTER_VAR_EX( "report_safe_refs", NGlobal::VarBoolHandler, &s_bReportSafeRefs, true, STORAGE_NONE );
FINISH_REGISTER


