#include "StdAfx.h"

#include "AIGeometryExporter.h"
#include "../3Dmotor/aiObject.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../Misc/StrProc.h"

#include "../MapEditorLib/CommonExporterMethods.h"
#include "Tools_Granny.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( AIGeometry, CAIGeometryExporter )


const char *CAIGeometryExporter::GetAddPath() const
{
	return "bin\\aigeometries\\";
}

bool CAIGeometryExporter::FormScript( string *pScriptText,
																			const string &szTypeName,
																			const string &szObjName,
																			const string &szDstPath,
																			const string &szSrcPath,
																 	    IManipulator *pManipulator )
{
	string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() ) 
	{
		szSettingsFileName = GetOption( &SUserData::SMayaExportData::szAIGeomSettingsFileName );
		NStr::ReplaceAllChars( &szSettingsFileName, '\\', '/' );
	}
	//
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
	const string szScriptTemplate = GetScriptTemplate( "ExportAIGeometry" );
	if ( CManipulatorManager::GetValue( &szRootMesh, pManipulator, "RootMesh" ) == false )
		szRootMesh.clear();
	if ( CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" ) == false )
		szRootJoint.clear();
	if ( szRootMesh == szRootJoint ) 
		szRootJoint.clear();

	*pScriptText = StrFmt( szScriptTemplate.c_str(),
		szObjName.c_str(), szDstPath.c_str(), szSrcPath.c_str(),
		szRootMesh.c_str(), szRootJoint.c_str(),
		szSettingsFileName.c_str() );
	*pScriptText += ";\n";
	//
	return true;
}

bool CAIGeometryExporter::ImportInfoToDBBeforeRefs( const string &szGeomObjName, 
																									  const string &szSrcScenePath,
																										const string &szDstFileName,
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
		pLogger->Log( LT_ERROR, StrFmt("\tObject name: %s\n", szGeomObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tFile name: %s\n", szDstFileName.c_str()) );
	}
	return false;
}

EXPORT_RESULT CAIGeometryExporter::CustomCheck( const string &szTypeName,
																								const string &szObjName, 
																								const string &szSrcScenePath,
																								const string &szDestinationPath, 
																								IManipulator *pManipulator )
{
	CGrannyFileInfoGuard fileInfo( szDestinationPath );
	for ( int nMeshIndex = 0; nMeshIndex < fileInfo->MeshCount; ++nMeshIndex )
	{
		vector<CVec3> vertexList;
		vector<STriangle> triangleList;
		granny_mesh *pMesh = fileInfo->Meshes[nMeshIndex];
		GetVerticesFromGrannyMesh( pMesh, &vertexList );
		GetTrianglesFromGrannyMesh( pMesh, &triangleList );
		NAI::CEdgesInfo edgesInfo;
		edgesInfo.GenerateEdgeList( triangleList, vertexList );
		if ( !edgesInfo.IsClosed() )
		{
			ILogger *pLogger = NLog::GetLogger();
			string szSrcScenePath;
			pLogger->Log( LT_ERROR, "AI Geometry is not closed\n" );
			pLogger->Log( LT_ERROR, StrFmt("\tObject name: %s\n", szObjName.c_str()) );
			pLogger->Log( LT_ERROR, StrFmt("\tSource file: %s\n", szSrcScenePath.c_str()) );
			return ER_FAIL;
		}
	}
	return ER_SUCCESS;
}


