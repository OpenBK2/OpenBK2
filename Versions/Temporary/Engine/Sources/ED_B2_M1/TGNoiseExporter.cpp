#include "stdafx.h"
#include <fmt/format.h>

#include "TGNoiseExporter.h"

#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "System/FileUtils.h"

REGISTER_EXPORTER_IN_DLL( TGNoise, CTGNoiseExporter )

EXPORT_RESULT CTGNoiseExporter::ExportObject( IManipulator* pManipulator,
																							const std::string &rszObjectTypeName,
																							const std::string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	ILogger *pLogger = NLog::GetLogger();
	//
	std::string szNoiseFileName;
	if ( CManipulatorManager::GetValue(&szNoiseFileName, pManipulator, "FileName") != false && !szNoiseFileName.empty() )
	{
		if ( NFile::CopyFile( pUserData->constUserData.szExportSourceFolder + szNoiseFileName, 
			                    Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szNoiseFileName ) == false )
		{
			pLogger->Log( LT_ERROR, "Can't export noise\n" );
			pLogger->Log( LT_ERROR, fmt::format("\tNoise: {}\n", szNoiseFileName.c_str()) );
			return ER_FAIL;
		}
	}
	//
	return ER_SUCCESS;
}


