#include "stdafx.h"

#include "ChapterExporter.h"

#include "ExporterMethods.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/FileUtils.h"
#include "MapEditorLib/Interface_MOD.h"


REGISTER_EXPORTER_IN_DLL( Chapter, CChapterExporter )

EXPORT_RESULT CChapterExporter::ExportObject( IManipulator* pManipulator,
																							const std::string &rszObjectTypeName,
																							const std::string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	//
	const std::string szChapterPath = NFile::GetFilePath( rszObjectName );
	// Noise/sea map
	std::string szNoiseMap;
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( CManipulatorManager::GetValue(&szNoiseMap, pManipulator, "SeaNoiseMask") != false && !szNoiseMap.empty() )
	{
		const std::string szSrcFileName = pUserData->constUserData.szExportSourceFolder + szNoiseMap;
		const std::string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szChapterPath + NFile::GetFileName( szSrcFileName );
		if ( CheckFilesUpdated( szSrcFileName, szDstFileName, bForce ) == false )
			NFile::CopyFile( szSrcFileName, szDstFileName );
	}
	if ( CManipulatorManager::GetValue(&szNoiseMap, pManipulator, "DifferentColourMap") != false && !szNoiseMap.empty() )
	{
		const std::string szSrcFileName = pUserData->constUserData.szExportSourceFolder + szNoiseMap;
		const std::string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szChapterPath + NFile::GetFileName( szSrcFileName );
		if ( CheckFilesUpdated( szSrcFileName, szDstFileName, bForce ) == false )
			NFile::CopyFile( szSrcFileName, szDstFileName );
	}
	//
	return ER_SUCCESS;
}


