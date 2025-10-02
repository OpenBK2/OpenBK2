#include "StdAfx.h"

#include "LightExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "../System/FileUtils.h"
#include "../System/FilePath.h"

REGISTER_EXPORTER_IN_DLL( AnimLight, CLightExporter )


CLightExporter::CLightExporter() : bNeedExport( false )
{
}


bool CLightExporter::StartExport( const string &rszObjectTypeName, bool bForce )
{
	bNeedExport = false;
	MEStartScript( &szScriptText, false );
	//
	szScriptText += StrFmt( "loadPlugin \"%s\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginFileName).c_str() );
	return true;
}


void CLightExporter::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	MEFinishScript( &szScriptText, false );
	MERunScript( szScriptText, rszObjectTypeName, bNeedExport, false );
}


EXPORT_RESULT CLightExporter::ExportObject( IManipulator* pManipulator,
																						const string &rszObjectTypeName,
																						const string &rszObjectName,
																						bool bForce,
																						EXPORT_TYPE exportType )
{
	NI_ASSERT( pManipulator != 0, "CLightExporter::ExportObject() pManipulator == 0 )" );
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	// Формируем тело скрипта
	string szSorceValue;
	CManipulatorManager::GetValue( &szSorceValue, pManipulator, "SrcName" );
	const string szSource				= pUserData->constUserData.szExportSourceFolder + szSorceValue;
	const string szLightsFolder	= Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Lights\\";
	const string szDestination = BuildDestFilePath( pManipulator, szLightsFolder );
	// check for source and destination times if not forced mode
	if ( CheckFilesUpdated( szSource, szDestination, bForce ) )
	{
		return ER_SUCCESS;
	}
	//
	NFile::CreatePath( szLightsFolder.c_str() );
	//
	string szSelectNode;
	CManipulatorManager::GetValue( &szSelectNode, pManipulator, "SelectNode" );
	//
	szScriptText += StrFmt( "print \"%s\";\r\n", rszObjectName.c_str() );
	szScriptText += StrFmt( "sysFile -del \"%s\";\r\n", szDestination.c_str() );
	szScriptText += StrFmt( "file -o -f \"%s\";\r\n", szSource.c_str() );
	szScriptText += StrFmt( "select -cl;\r\n" );
	szScriptText += StrFmt( "select \"%s\";\r\n", szSelectNode.c_str() );
	szScriptText += StrFmt( "file -es -f -typ \"%s\" -op \"light=1;\" \"%s\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginName).c_str(), szDestination.c_str() );
	//
	bNeedExport = true;
	return ER_SUCCESS;
}

// basement storage  


