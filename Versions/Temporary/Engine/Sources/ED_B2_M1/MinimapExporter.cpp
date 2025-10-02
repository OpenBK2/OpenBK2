#include "StdAfx.h"

#include "MinimapExporter.h"

#include "ExporterMethods.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../System/FileUtils.h"
#include "../MapEditorLib/Interface_MOD.h"

REGISTER_EXPORTER_IN_DLL( Minimap, CMinimapExporter )

EXPORT_RESULT CMinimapExporter::ExportObject( IManipulator* pManipulator,
																							const string &rszObjectTypeName,
																							const string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
		return ER_SUCCESS;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	int nNumLayers = 0;
	CManipulatorManager::GetValue( &nNumLayers, pManipulator, "Layers" );
	for ( int i = 0; i < nNumLayers; ++i )
	{
		string szNoiseImageFileName;
		CManipulatorManager::GetValue( &szNoiseImageFileName, pManipulator, StrFmt("Layers.[%d].NoiseImage", i) );
		if ( !szNoiseImageFileName.empty() && szNoiseImageFileName != " " )
		{
			const string szSrcFileName = pUserData->constUserData.szExportSourceFolder + szNoiseImageFileName;
			const string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szNoiseImageFileName;
			if ( CheckFilesUpdated(szSrcFileName, szDstFileName, bForce) == false )
				NFile::CopyFile( szSrcFileName, szDstFileName );
		}
	}
	return ER_SUCCESS;
}


