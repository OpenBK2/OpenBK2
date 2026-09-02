#include "stdafx.h"
#include <fmt/format.h>
#include <fmt/printf.h>

#include "AIGeometryExporter.h"
#include "3Dmotor/aiObject.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "Misc/StrProc.h"

#include "MapEditorLib/CommonExporterMethods.h"
#include "Tools_Granny.h"

REGISTER_EXPORTER_IN_DLL( AIGeometry, CAIGeometryExporter )


const char *CAIGeometryExporter::GetAddPath() const
{
	return "bin\\aigeometries\\";
}

bool CAIGeometryExporter::FormScript( std::string *pScriptText,
																			const std::string &szTypeName,
																			const std::string &szObjName,
																			const std::string &szDstPath,
																			const std::string &szSrcPath,
																 	    IManipulator *pManipulator )
{
	std::string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() ) 
	{
		szSettingsFileName = GetOption( &SUserData::SMayaExportData::szAIGeomSettingsFileName );
		NStr::ReplaceAllChars( &szSettingsFileName, '\\', '/' );
	}
	//
	if ( szSettingsFileName.empty() )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, fmt::format("Granny exporter settings file is not specified\n") );
		pLogger->Log( LT_ERROR, fmt::format("Check ConstUserData.xml in \"MayaExport\" section\n") );
		pLogger->Log( LT_ERROR, fmt::format("\tExport type: {}\n", szTypeName.c_str()) );
		return false;
	}
	//
	std::string szRootMesh;
	std::string szRootJoint;
	const std::string szScriptTemplate = GetScriptTemplate( "ExportAIGeometry" );
	if ( CManipulatorManager::GetValue( &szRootMesh, pManipulator, "RootMesh" ) == false )
		szRootMesh.clear();
	if ( CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" ) == false )
		szRootJoint.clear();
	if ( szRootMesh == szRootJoint ) 
		szRootJoint.clear();

	*pScriptText = fmt::sprintf( szScriptTemplate.c_str(),
		szObjName.c_str(), szDstPath.c_str(), szSrcPath.c_str(),
		szRootMesh.c_str(), szRootJoint.c_str(),
		szSettingsFileName.c_str() );
	*pScriptText += ";\n";
	//
	return true;
}

bool CAIGeometryExporter::ImportInfoToDBBeforeRefs( const std::string &szGeomObjName, 
																									  const std::string &szSrcScenePath,
																										const std::string &szDstFileName,
																										IManipulator *pManipulator )
{
	try
	{
		CVec3 vMin = VNULL3;
		CVec3 vMax = VNULL3;
		{
			WaitForFile( szDstFileName, 10000 );
			CGrannyFileInfoGuard fileInfo( szDstFileName );
			GetGrannyMeshBoundingBox( &vMin, &vMax, fileInfo );
		}
		const CVec3 vAABBCenter = ( vMax + vMin ) / 2.0f;
		const CVec3 vAABBHalfSize = ( vMax - vMin ) / 2.0f;
		CManipulatorManager::SetVec3( vAABBCenter, pManipulator, "AABBCenter" );
		CManipulatorManager::SetVec3( vAABBHalfSize, pManipulator, "AABBHalfSize" );
		return true;
	}
	catch ( ... ) 
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, "Error while retrieving AABB from AI geometry\n" );
		pLogger->Log( LT_ERROR, fmt::format("\tObject name: {}\n", szGeomObjName.c_str()) );
		pLogger->Log( LT_ERROR, fmt::format("\tFile name: {}\n", szDstFileName.c_str()) );
	}
	return false;
}

EXPORT_RESULT CAIGeometryExporter::CustomCheck( const std::string &szTypeName,
																								const std::string &szObjName, 
																								const std::string &szSrcScenePath,
																								const std::string &szDestinationPath, 
																								IManipulator *pManipulator )
{
	CGrannyFileInfoGuard fileInfo( szDestinationPath );
	for ( int nMeshIndex = 0; nMeshIndex < fileInfo->MeshCount; ++nMeshIndex )
	{
		std::vector<CVec3> vertexList;
		std::vector<STriangle> triangleList;
		granny_mesh *pMesh = fileInfo->Meshes[nMeshIndex];
		GetVerticesFromGrannyMesh( pMesh, &vertexList );
		GetTrianglesFromGrannyMesh( pMesh, &triangleList );
		NAI::CEdgesInfo edgesInfo;
		edgesInfo.GenerateEdgeList( triangleList, vertexList );
		if ( !edgesInfo.IsClosed() )
		{
			ILogger *pLogger = NLog::GetLogger();
			std::string szSrcScenePath;
			pLogger->Log( LT_ERROR, "AI Geometry is not closed\n" );
			pLogger->Log( LT_ERROR, fmt::format("\tObject name: {}\n", szObjName.c_str()) );
			pLogger->Log( LT_ERROR, fmt::format("\tSource file: {}\n", szSrcScenePath.c_str()) );
			return ER_FAIL;
		}
	}
	return ER_SUCCESS;
}


