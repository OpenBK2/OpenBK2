#include "stdafx.h"
#include <fmt/format.h>

#include "LightExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"

REGISTER_EXPORTER_IN_DLL( AnimLight, CLightExporter )


CLightExporter::CLightExporter() : bNeedExport( false )
{
}


bool CLightExporter::StartExport( const std::string &rszObjectTypeName, bool bForce )
{
	bNeedExport = false;
	MEStartScript( &szScriptText, false );
	//
	szScriptText += fmt::format( "loadPlugin \"{}\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginFileName).c_str() );
	return true;
}


void CLightExporter::FinishExport( const std::string &rszObjectTypeName, bool bForce )
{
	MEFinishScript( &szScriptText, false );
	MERunScript( szScriptText, rszObjectTypeName, bNeedExport, false );
}


EXPORT_RESULT CLightExporter::ExportObject( IManipulator* pManipulator,
																						const std::string &rszObjectTypeName,
																						const std::string &rszObjectName,
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
	std::string szSorceValue;
	CManipulatorManager::GetValue( &szSorceValue, pManipulator, "SrcName" );
	const std::string szSource				= pUserData->constUserData.szExportSourceFolder + szSorceValue;
	const std::string szLightsFolder	= Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Lights\\";
	const std::string szDestination = BuildDestFilePath( pManipulator, szLightsFolder );
	// check for source and destination times if not forced mode
	if ( CheckFilesUpdated( szSource, szDestination, bForce ) )
	{
		return ER_SUCCESS;
	}
	//
	NFile::CreatePath( szLightsFolder.c_str() );
	//
	std::string szSelectNode;
	CManipulatorManager::GetValue( &szSelectNode, pManipulator, "SelectNode" );
	//
	szScriptText += fmt::format( "print \"{}\";\r\n", rszObjectName.c_str() );
	szScriptText += fmt::format( "sysFile -del \"{}\";\r\n", szDestination.c_str() );
	szScriptText += fmt::format( "file -o -f \"{}\";\r\n", szSource.c_str() );
	szScriptText += fmt::format( "select -cl;\r\n" );
	szScriptText += fmt::format( "select \"{}\";\r\n", szSelectNode.c_str() );
	szScriptText += fmt::format( "file -es -f -typ \"{}\" -op \"light=1;\" \"{}\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginName).c_str(), szDestination.c_str() );
	//
	bNeedExport = true;
	return ER_SUCCESS;
}

// basement storage  


