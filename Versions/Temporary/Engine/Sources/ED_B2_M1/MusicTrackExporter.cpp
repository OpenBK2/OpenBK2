#include "StdAfx.h"
#include "./musictrackexporter.h"

#include "stdafx.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "../System/FileUtils.h"


REGISTER_EXPORTER_IN_DLL( MusicTrack, CMusicTrackExporter )


EXPORT_RESULT CMusicTrackExporter::ExportObject( IManipulator* pManipulator,
																							 const string &rszObjectTypeName,
																							 const string &rszObjectName,
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
	string szSoundPath;
	if ( !CManipulatorManager::GetValue( &szSoundPath, pManipulator, "SoundPath" ) || szSoundPath.empty() || szSoundPath == " " )
	{
		const string szError = StrFmt( "Can't get sound path or sound path empty for MusicTrack object \"%s\"\n", rszObjectName.c_str() );
		pLogger->Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( !pUserData )
	{
		return ER_FAIL;
	}
	const string szSourceFolder = pUserData->constUserData.szExportSourceFolder;
	const string szDestinationFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION );
	string szSource = szSourceFolder + szSoundPath;
	string szDestination = szDestinationFolder + szSoundPath;
	//
	// check for source and destination times if not forced mode
	if ( (szSource == szDestination) || CheckFilesUpdated( szSource, szDestination, bForce ) )
		return ER_SUCCESS;
	//
	if ( NFile::CopyFile( szSource, szDestination ) == false )
	{
		DWORD dwErrorCode = ::GetLastError();
		pLogger->Log( LT_ERROR, StrFmt("Can't copy Music Track object\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tMusicTrack: %s\n", rszObjectName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource file: %s\n", szSource.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tDestination file: %s\n", szDestination.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tError code: %d\n", dwErrorCode) );
		return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}


