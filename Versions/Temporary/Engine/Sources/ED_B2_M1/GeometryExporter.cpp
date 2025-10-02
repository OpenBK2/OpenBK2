#include "StdAfx.h"

#include "GeometryExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"

#include "ExporterMethods.h"

REGISTER_EXPORTER_IN_DLL( Geometry, CGeometryExporter )

const char *CGeometryExporter::GetAddPath() const
{
	return "bin\\geometries\\";
}

bool CGeometryExporter::FormScript( string *pScriptText,
																		const string &szTypeName,
																		const string &szObjName,
																		const string &szDstPath,
																		const string &szSrcPath,
																		IManipulator *pManipulator )
{
	const string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, StrFmt("Granny exporter settings file is not specified\n") );
		pLogger->Log( LT_ERROR, StrFmt("Check ConstUserData.xml in \"MayaExport\" section\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tExport type: %s\n", szTypeName.c_str()) );
		return false;
	}
	//
	string szRootMesh;
	string szRootJoint;
	const string szScriptTemplate = GetScriptTemplate( "ExportGeometry" );
	if ( CManipulatorManager::GetValue( &szRootMesh, pManipulator, "RootMesh" ) == false )
		szRootMesh.clear();
	if ( CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" ) == false )
		szRootJoint.clear();
	if ( szRootMesh == szRootJoint ) 
		szRootJoint.clear();

	*pScriptText = StrFmt( szScriptTemplate.c_str(),
		szDstPath.c_str(), szSrcPath.c_str(),
		szRootMesh.c_str(), szRootJoint.c_str(),
		szSettingsFileName.c_str() );
	*pScriptText += ";\n";
	//
	return true;
}

bool CGeometryExporter::ImportInfoToDBBeforeRefs( const string &szObjName, 
																									const string &szSrcScenePath,
																									const string &szDstFileName, 
																									IManipulator *pManipulator )
{
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	try
	{
		// read number of meshes from granny geometry file and write it to database
		WaitForFile( szDstFileName, 10000 );
		CGrannyFileInfoGuard fileInfo( szDstFileName );
		if ( fileInfo->MeshCount == 0 ) 
		{
			pLogger->Log( LT_ERROR, StrFmt("Geometry has 0 meshes - invalid scene or root mesh name\n") );
			pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
			pLogger->Log( LT_ERROR, StrFmt("\tDestination file: %s\n", szDstFileName.c_str()) );
			return false;
		}
		for ( int i = 0; i < fileInfo->MeshCount; ++i ) 
		{
			if ( fileInfo->Meshes[i]->PrimaryTopology->GroupCount != 1 ) 
			{
				pLogger->Log( LT_ERROR, StrFmt("Geometry has invalid number of topology groups in mesh, (must have 1)\n") );
				pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
				pLogger->Log( LT_ERROR, StrFmt("\tNumber of topology groups: %d\n", fileInfo->Meshes[i]->PrimaryTopology->GroupCount) );
				pLogger->Log( LT_ERROR, StrFmt("\tMesh: %s\n", fileInfo->Meshes[i]->Name) );
			}
		}
		CVariant value;
		value = (int)fileInfo->MeshCount;
		pManipulator->SetValue( "NumMeshes", value );
		// calculate AABB of the model
		CVec3 vMin( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE ), vMax( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
		if ( GetGrannyMeshBoundingBox(&vMin, &vMax, fileInfo) != false )
		{
			CVec3 vCenter = ( vMax + vMin ) / 2.0f;
			CVec3 vHalfSize = ( vMax - vMin ) / 2.0f;
			CManipulatorManager::SetVec3( vCenter, pManipulator, "Center" );
			CManipulatorManager::SetVec3( vHalfSize * 2.0f, pManipulator, "Size" );
		}
	}
	catch ( ... ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("General fail during importing DB data for geometry\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tFile name: %s\n", szDstFileName.c_str()) );
		return false;
	}
	return true;
}

EXPORT_RESULT CGeometryExporter::CustomCheck( const string &szTypeName,
																						  const string &szObjName, 
																							const string &szSrcScenePath,
																							const string &szDestinationPath, 
																							IManipulator *pManipulator )
{
	ILogger *pLogger = NLog::GetLogger();
	CGrannyFileInfoGuard fileInfo( szDestinationPath );
	// check for number of skeletons in file
	if ( fileInfo->SkeletonCount != 1 )
	{
		pLogger->Log( LT_ERROR, StrFmt("Incorrect number of skeletons\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSkeletons count: %d\n", fileInfo->SkeletonCount) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource File: %s\n", szSrcScenePath.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tDestination File: %s\n", szDestinationPath.c_str()) );
		return ER_FAIL;
	}
	// check for number of bones
	if ( fileInfo->Skeletons[0]->BoneCount == 0 ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("Empty bones list - check \"RootMesh\" and \"RootJoint\"\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource File: %s\n", szSrcScenePath.c_str()) );
		return ER_FAIL;
	}
	// check number of meshes
	if ( fileInfo->MeshCount == 0 )
	{
		pLogger->Log( LT_ERROR, StrFmt("Empty meshes list - check \"RootMesh\"\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tGeometry: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource File: %s\n", szSrcScenePath.c_str()) );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}

// basement storage  


