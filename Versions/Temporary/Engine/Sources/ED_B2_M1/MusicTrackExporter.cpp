#include "stdafx.h"
#include <fmt/format.h>
#include "MusicTrackExporter.h"

#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "System/FileUtils.h"

#include <cstdint>

REGISTER_EXPORTER_IN_DLL( MusicTrack, CMusicTrackExporter )


EXPORT_RESULT CMusicTrackExporter::ExportObject( IManipulator* pManipulator,
																							 const std::string &rszObjectTypeName,
																							 const std::string &rszObjectName,
																							 bool bForce,
																							 EXPORT_TYPE exportType )
{
	return ER_SUCCESS;
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//

	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	ILogger *pLogger = NLog::GetLogger();
	std::string szSoundPath;
	if ( !CManipulatorManager::GetValue( &szSoundPath, pManipulator, "SoundPath" ) || szSoundPath.empty() || szSoundPath == " " )
	{
		const std::string szError = fmt::format( "Can't get sound path or sound path empty for MusicTrack object \"{}\"\n", rszObjectName.c_str() );
		pLogger->Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( !pUserData )
	{
		return ER_FAIL;
	}
	const std::string szSourceFolder = pUserData->constUserData.szExportSourceFolder;
	const std::string szDestinationFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION );
	std::string szSource = szSourceFolder + szSoundPath;
	std::string szDestination = szDestinationFolder + szSoundPath;
	//
	// check for source and destination times if not forced mode
	if ( (szSource == szDestination) || CheckFilesUpdated( szSource, szDestination, bForce ) )
		return ER_SUCCESS;
	//
	if ( NFile::CopyFile( szSource, szDestination ) == false )
	{
		uint32_t dwErrorCode = ::GetLastError();
		pLogger->Log( LT_ERROR, fmt::format("Can't copy Music Track object\n") );
		pLogger->Log( LT_ERROR, fmt::format("\tMusicTrack: {}\n", rszObjectName.c_str()) );
		pLogger->Log( LT_ERROR, fmt::format("\tSource file: {}\n", szSource.c_str()) );
		pLogger->Log( LT_ERROR, fmt::format("\tDestination file: {}\n", szDestination.c_str()) );
		pLogger->Log( LT_ERROR, fmt::format("\tError code: {}\n", dwErrorCode) );
		return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}


