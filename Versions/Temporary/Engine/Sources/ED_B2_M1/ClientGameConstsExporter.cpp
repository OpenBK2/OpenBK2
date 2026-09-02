#include "stdafx.h"

#include "ClientGameConstsExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "System/FileUtils.h"

REGISTER_EXPORTER_IN_DLL( ClientGameConsts, CClientGameConstsExporter )

EXPORT_RESULT CClientGameConstsExporter::ExportObject( IManipulator* pManipulator,
																												const std::string &rszObjectTypeName,
																												const std::string &rszObjectName,
																												bool bForce,
																												EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// copy cursors
	int nNumCursors = 0;
	if ( CManipulatorManager::GetValue( &nNumCursors, pManipulator, "Cursors" ) != false )
	{
		std::string szFileName;
		for ( int i = 0; i < nNumCursors; ++i ) 
		{
			const std::string szName = StrFmt( "Cursors.[%d].FileName", i );
			if ( CManipulatorManager::GetValue( &szFileName, pManipulator, szName ) ) 
			{
				const std::string szSrcFileName = pUserData->constUserData.szExportSourceFolder + szFileName;
				const std::string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szFileName;
				if ( CheckFilesUpdated( szSrcFileName, szDstFileName, bForce ) == false )
					NFile::CopyFile( szSrcFileName, szDstFileName );
			}
		}
	}
	// copy noises
	std::string szNoisesFileName;
	if ( CManipulatorManager::GetValue(&szNoisesFileName, pManipulator, "Noises") != false && !szNoisesFileName.empty() )
		ExportFilesList( szNoisesFileName, bForce, "Noises" );
	//
	return ER_SUCCESS;
}


