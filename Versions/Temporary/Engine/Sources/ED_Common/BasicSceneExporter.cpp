#include "stdafx.h"
#include <fmt/format.h>

#include "BasicSceneExporter.h"
#include "GltfExporter.h"
#include "MapEditorLib/MessageBoxes.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"

#include "MapEditorLib/CommonExporterMethods.h"
#include "System/FileUtils.h"
#include "System/FilePath.h"
#include "Misc/StrProc.h"
//#include "../System/BinaryResources.h"

bool CBasicSceneExporter::Validate( IManipulator *pManipulator )
{
	bool bStructureValid = true;

	std::string szSrcScenePath;
	CManipulatorManager::GetValue( &szSrcScenePath, pManipulator, "SrcName" );
	if ( szSrcScenePath.empty() )
	{
		Log( LT_ERROR, "Geometry contain empty path to source scene\n" );
		bStructureValid = false;
	}

	return bStructureValid;
}


bool CBasicSceneExporter::ExportFromMaya( const std::string &szTypeName, 
																				  const std::string &szObjName, 
																				  const std::string &szDstPath, 
																				  const std::string &szSrcPath,
																				  IManipulator *pManipulator )
{
	std::string szScript;
	try
	{
		if ( FormScript( &szScript, szTypeName, szObjName, szDstPath, szSrcPath, pManipulator ) == false )
			return false;
	}
	catch ( ... ) 
	{
		const std::string szError( "Script formation failed - wrong script template - check it\n" );
		Log( LT_ERROR, szError );
		return false;
	}

	return ExecuteMayaScript( szScript );
}


EXPORT_RESULT CBasicSceneExporter::ExportObject( IManipulator* pManipulator,
                                                 const std::string &rszObjectTypeName,
                                                 const std::string &rszObjectName,
                                                 bool bForce,
                                                 EXPORT_TYPE exportType )
{
	// Export consumes engine-readable GLB/GLTF packages. Legacy GR2 loading
	// and checking remain available, but this path never launches Maya.
	if ( exportType == ET_AFTER_REF ) return ER_SUCCESS;
	if ( !NEditorGltf::ValidateForExport(pManipulator, rszObjectTypeName) ) return ER_BREAK;
	if ( NEditorGltf::Export(pManipulator, rszObjectTypeName, true) && ImportGltfInfo(pManipulator) )
		return ER_SUCCESS;
	NMessage::Error("Could not export " + rszObjectName +
		". Check the model source, selectors and destination permissions; details are in the log.", "Model export");
	return ER_BREAK;
}

EXPORT_RESULT CBasicSceneExporter::CheckObject( IManipulator* pManipulator,
																								const std::string &rszObjectTypeName,
																								const std::string &rszObjectName,
																								bool bExport,
																								EXPORT_TYPE exportType )
{
	if ( NEditorGltf::IsGltf(pManipulator) )
		return NEditorGltf::Export(pManipulator, rszObjectTypeName, false) ? ER_SUCCESS : ER_BREAK;
	if ( exportType == ET_BEFORE_REF ) 
		return ER_SUCCESS;
	//
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// Export this object
	std::string szSrcScenePath;
	BuildSrcFilePath( &szSrcScenePath, pManipulator, "SrcName" );
	std::string szDestinationFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + GetAddPath();
	//std::string szDestinationPath = szDestinationFolder + std::to_string(  nObjectID );
	std::string szDestinationPath = BuildDestFilePath( pManipulator, szDestinationFolder );
	NStr::ReplaceAllChars( &szDestinationPath, '\\', '/' );
	std::string szObjName( rszObjectName );
	NStr::ReplaceAllChars( &szObjName, '\\', '/' );
	// check source sife existance
	if ( !NFile::DoesFileExist( szSrcScenePath ) ) 
	{
		const std::string szError = fmt::format( "Object \"{}\" (of type \"{}\") source file \"{}\" doesn't exist!\n", rszObjectName.c_str(), rszObjectTypeName.c_str(), szSrcScenePath.c_str() );
		Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	// check for destination file
	if ( WaitForFile( szDestinationPath, 10000, false ) == false ) 
	{
		// old file name style ?
		std::string _szDestinationPath = szDestinationFolder + std::to_string(  pManipulator->GetID( "" ) );
		if ( !NFile::DoesFileExist( _szDestinationPath ) ) 
		{
			const std::string szError = fmt::format( "Object \"{}\" (of type \"{}\") destination file \"{}\" doesn't exist (not exported yet?)\n", rszObjectName.c_str(), rszObjectTypeName.c_str(), szDestinationPath.c_str() );
			Log( LT_ERROR, szError );
			return ER_FAIL;
		}
		szDestinationFolder = _szDestinationPath;
	}
	//
	try
	{
		return CustomCheck( rszObjectTypeName, szObjName, szSrcScenePath, szDestinationPath, pManipulator );
	}
	catch ( ... ) 
	{
		const std::string szError = fmt::format( "General fail during check object \"{}\" of type \"{}\"\n", szObjName.c_str(), rszObjectTypeName.c_str() );
		Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}

// basement storage  


