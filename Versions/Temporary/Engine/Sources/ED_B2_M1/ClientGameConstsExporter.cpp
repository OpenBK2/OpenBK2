#include "StdAfx.h"

#include "ClientGameConstsExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "../System/FileUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( ClientGameConsts, CClientGameConstsExporter )

EXPORT_RESULT CClientGameConstsExporter::ExportObject( IManipulator* pManipulator,
																												const string &rszObjectTypeName,
																												const string &rszObjectName,
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
		string szFileName;
		for ( int i = 0; i < nNumCursors; ++i ) 
		{
			const string szName = StrFmt( "Cursors.[%d].FileName", i );
			if ( CManipulatorManager::GetValue( &szFileName, pManipulator, szName ) ) 
			{
				const string szSrcFileName = pUserData->constUserData.szExportSourceFolder + szFileName;
				const string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szFileName;
				if ( CheckFilesUpdated( szSrcFileName, szDstFileName, bForce ) == false )
					NFile::CopyFile( szSrcFileName, szDstFileName );
			}
		}
	}
	// copy noises
	string szNoisesFileName;
	if ( CManipulatorManager::GetValue(&szNoisesFileName, pManipulator, "Noises") != false && !szNoisesFileName.empty() )
		ExportFilesList( szNoisesFileName, bForce, "Noises" );
	//
	return ER_SUCCESS;
}

