#include "StdAfx.h"

#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/DefaultExporter.h"
#include "MapEditorLib/Interface_Logger.h"
#include "ExporterContainer.h"

bool CExporterContainer::StartExport( const string &rszExportTypeName,
																			const string &rszObjectTypeName,
																			bool bExport,
																			bool bForce,
																			bool bStartTools )
{
	if ( IExporter *pExporter = GetExporter( rszExportTypeName ) )
	{
		if ( bStartTools )
		{
			for ( CExportToolList::iterator itExportTool = exportToolList.begin(); itExportTool != exportToolList.end(); ++itExportTool )
			{
				( *itExportTool )->StartExportTool();
			}
		}
		//
		bool bResult = true;
		if ( bExport )
		{
			//DebugTrace( "CExporterContainer::StartExport(): <%s>", rszObjectTypeName.c_str() );
			try
			{
				bResult = pExporter->StartExport( rszObjectTypeName, bForce );
			}
			catch ( ... ) 
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_ERROR, StrFmt( "\nStartExport throw exception\n" ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszExportTypeName.c_str() ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
				bResult = false;
			}
			//DebugTrace( "result: %s", bResult ? "true" : "false" );
		}
		if ( bResult )
		{
			//DebugTrace( "CExporterContainer::StartCheck(): <%s>", rszObjectTypeName.c_str() );
			try
			{
				bResult = pExporter->StartCheck( rszObjectTypeName, bExport );
			}
			catch ( ... ) 
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_ERROR, StrFmt( "\nStartCheck throw exception\n" ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszExportTypeName.c_str() ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
				bResult = false;
			}
			//DebugTrace( "result: %s", bResult ? "true" : "false" );
		}
		return bResult;
	}
	return false;
}


void CExporterContainer::FinishExport( const string &rszExportTypeName,
																			 const string &rszObjectTypeName,
																			 bool bExport,
																			 bool bForce,
																			 bool bFinishTools )
{
	if ( IExporter *pExporter = GetExporter( rszExportTypeName ) )
	{
		if ( bExport )
		{
			//DebugTrace( "CExporterContainer::FinishExport(): <%s>", rszObjectTypeName.c_str() );
			try
			{
				pExporter->FinishExport( rszObjectTypeName, bForce );
			}
			catch ( ... ) 
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_ERROR, StrFmt( "\nFinishExport throw exception\n" ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszExportTypeName.c_str() ) );
				pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
			}
		}
		//DebugTrace( "CExporterContainer::FinishCheck(): <%s>", rszObjectTypeName.c_str() );
		try
		{
			pExporter->FinishCheck( rszObjectTypeName, bExport );
		}
		catch ( ... ) 
		{
			ILogger *pLogger = NLog::GetLogger();
			pLogger->Log( LT_ERROR, StrFmt( "\nFinishCheck throw exception\n" ) );
			pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszExportTypeName.c_str() ) );
			pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
		}
		//
		if ( bFinishTools )
		{
			for ( CExportToolList::iterator itExportTool = exportToolList.begin(); itExportTool != exportToolList.end(); ++itExportTool )
			{
				( *itExportTool )->FinishExportTool();
			}
		}
	}
}


EXPORT_RESULT	CExporterContainer::ExportObject( const string &rszExportTypeName,
																								IManipulator* pManipulator,
																								const string &rszObjectTypeName,
																								const string &rszObjectName,
																								bool bExport,
																								bool bForce )
{
	if ( IExporter *pExporter = GetExporter( rszExportTypeName ) )
	{
		EXPORT_RESULT eResult = ER_SUCCESS;
		if ( bExport )
		{
			//DebugTrace( "CExporterContainer::ExportObject(): <%s:%s>, type: ET_NO_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
//			try
			{
				eResult = pExporter->ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, ET_NO_REF );
			}
			//catch ( ... ) 
			//{
			//	ILogger *pLogger = NLog::GetLogger();
			//	pLogger->Log( LT_ERROR, StrFmt( "\nExportObject throw exception\n" ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_NO_REF\n" ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
			//	eResult = ER_FAIL;
			//}
			//DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : "ER_BREAK" );
		}
		if ( eResult == ER_SUCCESS )
		{
			//DebugTrace( "CExporterContainer::CheckObject(): <%s:%s>, type: ET_NO_REF", rszObjectTypeName.c_str(), rszObjectName.c_str() );
//			try
			{
				eResult = pExporter->CheckObject( pManipulator, rszObjectTypeName, rszObjectName, bExport, ET_NO_REF );
			}
			//catch ( ... ) 
			//{
			//	ILogger *pLogger = NLog::GetLogger();
			//	pLogger->Log( LT_ERROR, StrFmt( "\nCheckObject throw exception\n" ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tLocation: ET_NO_REF\n" ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tExportType: %s\n", rszObjectTypeName.c_str() ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tObjectType: %s\n", rszObjectTypeName.c_str() ) );
			//	pLogger->Log( LT_ERROR, StrFmt( "\tObjectName: %s\n", rszObjectName.c_str() ) );
			//	eResult = ER_FAIL;
			//}
			//DebugTrace( "result: %s", ( eResult == ER_FAIL ) ? "ER_FAIL" : ( eResult == ER_SUCCESS ) ? "ER_SUCCESS" : "ER_BREAK" );
		}
		return eResult;
	}
	return ER_FAIL;
}


bool CExporterContainer::CanExportObject( const string &rszObjectTypeName )
{
	return NExporterFactory::CanCreateExporter( rszObjectTypeName );
}


IExporter* CExporterContainer::GetExporter( const string &rszObjectTypeName )
{
	if ( !CanExportObject( rszObjectTypeName ) )
	{
		return 0;
	}
	CExporterMap::iterator posExporter = exporterMap.find( rszObjectTypeName );
	if( posExporter == exporterMap.end() )
	{
		exporterMap[rszObjectTypeName] = NExporterFactory::CreateExporter( rszObjectTypeName );
		posExporter = exporterMap.find( rszObjectTypeName );
	}
	return posExporter->second;
}


void CExporterContainer::Create( const string &rszObjectTypeName )
{
	IExporter *pExporter = GetExporter( rszObjectTypeName );
	NI_ASSERT( pExporter != 0, "CExporterContainer::Create() pExporter == 0" );
}


void CExporterContainer::Destroy( const string &rszObjectTypeName )
{
	CExporterMap::iterator posExporter = exporterMap.find( rszObjectTypeName );
	if ( posExporter != exporterMap.end() )
	{
		exporterMap.erase( posExporter );
	}
}


void CExporterContainer::RegisterExportTool( IExportTool *pExportTool )
{
	UnRegisterExportTool( pExportTool );
	exportToolList.push_back( pExportTool );
}


void CExporterContainer::UnRegisterExportTool( IExportTool *pExportTool )
{
	CExportToolList::iterator itExportTool = find( exportToolList.begin(), exportToolList.end(), pExportTool );
	if ( itExportTool != exportToolList.end() )
	{
		*itExportTool = 0;
		exportToolList.erase( itExportTool );
	}
}


bool CExporterContainer::StartExport( const string &rszObjectTypeName, bool bForce, bool bStartTools, bool bExportReferences )
{
	return StartExport( bExportReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
											rszObjectTypeName,
											true,
											bForce,
											bStartTools );
}


void CExporterContainer::FinishExport( const string &rszObjectTypeName, bool bForce, bool bFinishTools, bool bExportReferences )
{
	FinishExport( bExportReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
								rszObjectTypeName,
								true,
								bForce,
								bFinishTools );
}


EXPORT_RESULT	CExporterContainer::ExportObject( IManipulator* pManipulator,
																								const string &rszObjectTypeName,
																								const string &rszObjectName,
																								bool bForce,
																								bool bExportReferences )
{
	return ExportObject( bExportReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
											 pManipulator,
											 rszObjectTypeName,
											 rszObjectName,
											 true,
											 bForce );
}


bool CExporterContainer::StartCheck( const string &rszObjectTypeName, bool bStartTools, bool bCheckReferences )
{
	return StartExport( bCheckReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
											rszObjectTypeName,
											false,
											false,
											bStartTools );
}


void CExporterContainer::FinishCheck( const string &rszObjectTypeName, bool bFinishTools, bool bCheckReferences )
{
	FinishExport( bCheckReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
								rszObjectTypeName,
								false,
								false,
								bFinishTools );
}


EXPORT_RESULT CExporterContainer::CheckObject( IManipulator* pManipulator,
																							 const string &rszObjectTypeName,
																							 const string &rszObjectName,
																							 bool bCheckReferences )
{
	return ExportObject( bCheckReferences ? DEFAULT_EXPORTER_LABEL_TXT : rszObjectTypeName,
											 pManipulator,
											 rszObjectTypeName,
											 rszObjectName,
											 false,
											 false );
}


EXPORT_RESULT CExporterContainer::GetExportResult( const string &rszObjectRefName )
{
	if ( IExporter *pExporter = GetExporter( DEFAULT_EXPORTER_LABEL_TXT ) )
	{
		if ( CExporterBase *pExporterBase = checked_cast<CExporterBase*>( pExporter ) )
		{
			return pExporterBase->GetExportObjectResult( rszObjectRefName );
		}
	}
	return ER_UNKNOWN;
}


EXPORT_RESULT CExporterContainer::GetExportResult( const string &rszObjectTypeName, const string &rszObjectName )
{
	if ( IExporter *pExporter = GetExporter( DEFAULT_EXPORTER_LABEL_TXT ) )
	{
		if ( CExporterBase *pExporterBase = checked_cast<CExporterBase*>( pExporter ) )
		{
			string szObjectRefName;
			CStringManager::GetRefValueFromTypeAndName( &szObjectRefName, rszObjectTypeName, rszObjectName, TYPE_SEPARATOR_CHAR );
			return pExporterBase->GetExportObjectResult( szObjectRefName );
		}
	}
	return ER_UNKNOWN;
}


// basement storage  


