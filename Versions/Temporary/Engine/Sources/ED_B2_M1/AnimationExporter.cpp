#include "StdAfx.h"

#include "AnimationExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"

#include "ExporterMethods.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( AnimB2, CAnimationExporter )


const char *CAnimationExporter::GetAddPath() const
{
	return "bin\\animations\\";
}
static const char PARAMS_AABBA[] = ".aabba";
static const char PARAMS_AABBD[] = ".aabbd";

bool CAnimationExporter::FormScript( string *pScriptText,
																		const string &szTypeName,
																		const string &szObjName,
                                    const string &szDstPath,
																		const string &szSrcPath,
                                    IManipulator *pManipulator )
{
	ILogger *pLogger = NLog::GetLogger();
	const string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() )
	{
		pLogger->Log( LT_ERROR, StrFmt("Granny exporter settings file is not specified\n") );
		pLogger->Log( LT_ERROR, StrFmt("Check ConstUserData.xml in \"MayaExport\" section\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tExport type: %s\n", szTypeName.c_str()) );
		return false;
	}
	//
	string szRootJoint;
	CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" );
	int nFirstFrame = 0, nLastFrame = 0;
	CManipulatorManager::GetValue( &nFirstFrame, pManipulator , "FirstFrame" );
	CManipulatorManager::GetValue( &nLastFrame, pManipulator , "LastFrame" );
	// main script - export animation
	string szScriptTemplate = GetScriptTemplate( "ExportAnimation" );
	*pScriptText = StrFmt( szScriptTemplate.c_str(),
		szDstPath.c_str(), szSrcPath.c_str(),
		szRootJoint.c_str(), nFirstFrame, nLastFrame,
		szSettingsFileName.c_str() );
	*pScriptText += ";\n";
	// additional script - export AABB_A & AABB_D params
	const string szAttribSettingsFileName = GetGrannyExportSettingsFileName( "Attribs" );
	if ( szAttribSettingsFileName.empty() )
	{
		pLogger->Log( LT_ERROR, StrFmt("Granny exporter settings file is not specified\n") );
		pLogger->Log( LT_ERROR, StrFmt("Check ConstUserData.xml in \"MayaExport\" section\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tExport type: %s\n", "Attribs") );
		return false;
	}
	//
	szScriptTemplate = GetScriptTemplate( "ExportAttribs" );
	string szAABBAName;
	if ( CManipulatorManager::GetValue( &szAABBAName, pManipulator , "AABBAName" ) && !szAABBAName.empty() )
	{
		const string szScriptTemplate = GetScriptTemplate( "ExportAttribs" );
		*pScriptText += StrFmt( szScriptTemplate.c_str(),
			(szDstPath + PARAMS_AABBA).c_str(), szSrcPath.c_str(),
			szAABBAName.c_str(), "", 
			szAttribSettingsFileName.c_str() );
		*pScriptText += ";\n";
	}
	string szAABBDName;
	if ( CManipulatorManager::GetValue( &szAABBDName, pManipulator , "AABBDName" ) && !szAABBDName.empty() )
	{
		*pScriptText += StrFmt( szScriptTemplate.c_str(),
			(szDstPath + PARAMS_AABBD).c_str(), szSrcPath.c_str(),
			szAABBDName.c_str(), "", 
			szAttribSettingsFileName.c_str() );
		*pScriptText += ";\n";
	}
	return true;
}

bool CAnimationExporter::ImportInfoToDBBeforeRefs( const string &szObjName, 
																									 const string &szSrcScenePath,
																									 const string &szDstFileName,
															                     IManipulator *pManipulator )
{
	ILogger *pLogger = NLog::GetLogger();
	bool bResult = true;
	try
	{
		{
			// read anim length
			WaitForFile( szDstFileName, 10000 );
			CGrannyFileInfoGuard fileInfo( szDstFileName );
			const int nAnimLength = GetGrannyAnimationLength( fileInfo );
			pManipulator->SetValue( "Length", nAnimLength );
			// set action moment (msec)
			int nFirstFrame = 0, nLastFrame = 0, nActionFrame = 0;
			CManipulatorManager::GetValue( &nFirstFrame, pManipulator , "FirstFrame" );
			CManipulatorManager::GetValue( &nLastFrame, pManipulator , "LastFrame" );
			CManipulatorManager::GetValue( &nActionFrame, pManipulator , "ActionFrame" );
			const int nActionTime = int( float( nActionFrame * nAnimLength ) / float( nLastFrame - nFirstFrame ) );
			CManipulatorManager::SetValue( nActionTime, pManipulator , "Action" );
		}
		//
		string szAABBAName;
		if ( CManipulatorManager::GetValue( &szAABBAName, pManipulator, "AABBAName" ) && !szAABBAName.empty() )
		{
			const string szAttribsFileName = szDstFileName + PARAMS_AABBA;
			WaitForFile( szAttribsFileName, 10000 );
			CGrannyFileInfoGuard fileInfo( szAttribsFileName );
			//
			CVec3 vMin = VNULL3;
			CVec3 vMax = VNULL3;
			GetGrannyMeshBoundingBox( &vMin, &vMax, fileInfo );
			CVec3 vCenter = ( vMax + vMin ) / 2.0f;
			CVec3 vHalfSize = ( vMax - vMin ) / 2.0f;
			Vis2AI( &vCenter ); 
			Vis2AI( &vHalfSize ); 
			//
			pManipulator->SetValue( "aabb_a.Center.x", vCenter.x );
			pManipulator->SetValue( "aabb_a.Center.y", vCenter.y );
			pManipulator->SetValue( "aabb_a.Center.z", vCenter.z );
			pManipulator->SetValue( "aabb_a.HalfSize.x", vHalfSize.x );
			pManipulator->SetValue( "aabb_a.HalfSize.y", vHalfSize.y );
			pManipulator->SetValue( "aabb_a.HalfSize.z", vHalfSize.z );
		}
		string szAABBDName;
		if ( CManipulatorManager::GetValue( &szAABBDName, pManipulator, "AABBDName" ) && ( !szAABBDName.empty() ) )
		{
			const string szAttribsFileName = szDstFileName + PARAMS_AABBD;
			WaitForFile( szAttribsFileName, 10000 );
			CGrannyFileInfoGuard fileInfo( szAttribsFileName );
			//
			CVec3 vMin = VNULL3;
			CVec3 vMax = VNULL3;
			GetGrannyMeshBoundingBox( &vMin, &vMax, fileInfo );
			CVec3 vCenter = ( vMax + vMin ) / 2.0f;
			CVec3 vHalfSize = ( vMax - vMin ) / 2.0f;
			Vis2AI( &vCenter ); 
			Vis2AI( &vHalfSize ); 
			//
			pManipulator->SetValue( "aabb_d.Center.x", vCenter.x );
			pManipulator->SetValue( "aabb_d.Center.y", vCenter.y );
			pManipulator->SetValue( "aabb_d.Center.z", vCenter.z );
			pManipulator->SetValue( "aabb_d.HalfSize.x", vHalfSize.x );
			pManipulator->SetValue( "aabb_d.HalfSize.y", vHalfSize.y );
			pManipulator->SetValue( "aabb_d.HalfSize.z", vHalfSize.z );
		}
	}
	catch ( ... ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("Can't open attribs file to update animation or can't open base animation file\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tBase animation file: %s\n", szDstFileName.c_str()) );
		bResult = false;
	}
	::DeleteFile( (szDstFileName + PARAMS_AABBA).c_str() );
	::DeleteFile( (szDstFileName + PARAMS_AABBD).c_str() );
	return bResult;
}

EXPORT_RESULT CAnimationExporter::CustomCheck( const string &szTypeName,
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
		pLogger->Log( LT_ERROR, StrFmt("Incorrect number of skeletons in file - check RootJoint uniquness\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tAnimation: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource file: %s\n", szSrcScenePath.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSkeletons count: %d\n", fileInfo->SkeletonCount) );
		if ( fileInfo->SkeletonCount > 1 ) 
		{
			for ( int i = 0; i < fileInfo->SkeletonCount; ++i ) 
				pLogger->Log( LT_ERROR, StrFmt("\t\t%s\n", fileInfo->Skeletons[i]->Name) );
		}
		return ER_FAIL;
	}
	// check for number of bones
	if ( fileInfo->Skeletons[0]->BoneCount == 0 ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("Empty bones list in skeleton - check \"RootJoint\"\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tAnimation: %s\n", szObjName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tSource file: %s\n", szSrcScenePath.c_str()) );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}

// basement storage  


