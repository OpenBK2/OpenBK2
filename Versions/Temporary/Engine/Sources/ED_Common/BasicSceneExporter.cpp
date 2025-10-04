#include "stdafx.h"

#include "BasicSceneExporter.h"
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

	string szSrcScenePath;
	CManipulatorManager::GetValue( &szSrcScenePath, pManipulator, "SrcName" );
	if ( szSrcScenePath.empty() )
	{
		Log( LT_ERROR, "Geometry contain empty path to source scene\n" );
		bStructureValid = false;
	}

	return bStructureValid;
}


bool CBasicSceneExporter::ExportFromMaya( const string &szTypeName, 
																				  const string &szObjName, 
																				  const string &szDstPath, 
																				  const string &szSrcPath,
																				  IManipulator *pManipulator )
{
	string szScript;
	try
	{
		if ( FormScript( &szScript, szTypeName, szObjName, szDstPath, szSrcPath, pManipulator ) == false )
			return false;
	}
	catch ( ... ) 
	{
		const string szError( "Script formation failed - wrong script template - check it\n" );
		Log( LT_ERROR, szError );
		return false;
	}

	return ExecuteMayaScript( szScript );
}


EXPORT_RESULT CBasicSceneExporter::ExportObject( IManipulator* pManipulator,
                                                 const string &rszObjectTypeName,
                                                 const string &rszObjectName,
                                                 bool bForce,
                                                 EXPORT_TYPE exportType )
{
	NI_ASSERT( pManipulator != 0, "CBasicSceneExporter::ExportObject() pManipulator == 0 )" );
	ILogger *pLogger = NLog::GetLogger();
	//
	if ( Validate( pManipulator ) == false )
		return ER_FAIL;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// Export this object
	string szSrcScenePath;
	BuildSrcFilePath( &szSrcScenePath, pManipulator, "SrcName" );
	if ( NFile::DoesFileExist( szSrcScenePath ) == false )
	{
		NLog::Log( LT_ERROR, "Source file does not exists!\n" );
		NLog::Log( LT_ERROR, "\tFile name: %s\n", szSrcScenePath.c_str() );
		NLog::Log( LT_ERROR, "\tObject name: %s\n", rszObjectName.c_str() );
		NLog::Log( LT_ERROR, "\tObject type: %s\n", rszObjectTypeName.c_str() );
		return ER_BREAK;
	}
	string szDestinationFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + GetAddPath();
	string szDestinationPath = BuildDestFilePath( pManipulator, szDestinationFolder );
	NFile::NormalizePath( &szDestinationPath );
	string szObjName( rszObjectName );
	NFile::NormalizePath( &szObjName );

	string szTempDstPath = NFile::GetTempFileName() + ".gr2";
	NFile::NormalizePath( &szTempDstPath );

	if ( exportType == ET_BEFORE_REF || exportType == ET_NO_REF )
	{
		// check for source and destination times if not forced mode
		if ( CheckFilesUpdated( szSrcScenePath, szDestinationPath, bForce ) )
			return ER_SUCCESS;
		// export geometry
		bool bResult = true;
		try
		{
			if ( ExportFromMaya( rszObjectTypeName, szObjName, szTempDstPath, szSrcScenePath, pManipulator ) &&
				   WaitForFile( szTempDstPath, 10000, true ) && 
				   CustomCheck( rszObjectTypeName, szObjName, szSrcScenePath, szTempDstPath, pManipulator ) == ER_SUCCESS )
			{
				bResult = bResult && ImportInfoToDBBeforeRefs( szObjName, szSrcScenePath, szTempDstPath, pManipulator );
				MoveTempFileToDestination( szTempDstPath, szDestinationPath );
			}
			else
			{
				DeleteFile( szTempDstPath.c_str() );
				bResult = false;
			}
		}
		catch ( ... )
		{
			DeleteFile( szTempDstPath.c_str() );
			bResult = false;
			NLog::Log( LT_ERROR, "Export failed\n" );
			NLog::Log( LT_ERROR, "\tObject type: \"%s\"\n", rszObjectTypeName.c_str() );
			NLog::Log( LT_ERROR, "\tObject name: \"%s\"\n", rszObjectName.c_str() );
			NLog::Log( LT_ERROR, "\tSource file name: \"%s\"\n", szSrcScenePath.c_str() );
			NLog::Log( LT_ERROR, "\tDestination file name: \"%s\"\n", szDestinationPath.c_str() );
			return ER_FAIL;
		}
		return (bResult ? ER_SUCCESS : ER_FAIL);
	}
	if ( exportType == ET_AFTER_REF || exportType == ET_NO_REF )
	{
		bool bResult = ImportInfoToDBAfterRefs( szObjName,
																						szSrcScenePath,
																						szDestinationPath,
																						pManipulator );
		return (bResult ? ER_SUCCESS : ER_FAIL);
	}

	return ER_BREAK;
}

EXPORT_RESULT CBasicSceneExporter::CheckObject( IManipulator* pManipulator,
																								const string &rszObjectTypeName,
																								const string &rszObjectName,
																								bool bExport,
																								EXPORT_TYPE exportType )
{
	if ( exportType == ET_BEFORE_REF ) 
		return ER_SUCCESS;
	//
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// Export this object
	string szSrcScenePath;
	BuildSrcFilePath( &szSrcScenePath, pManipulator, "SrcName" );
	string szDestinationFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + GetAddPath();
	//string szDestinationPath = szDestinationFolder + StrFmt( "%d", nObjectID );
	string szDestinationPath = BuildDestFilePath( pManipulator, szDestinationFolder );
	NStr::ReplaceAllChars( &szDestinationPath, '\\', '/' );
	string szObjName( rszObjectName );
	NStr::ReplaceAllChars( &szObjName, '\\', '/' );
	// check source sife existance
	if ( !NFile::DoesFileExist( szSrcScenePath ) ) 
	{
		const string szError = StrFmt( "Object \"%s\" (of type \"%s\") source file \"%s\" doesn't exist!\n", rszObjectName.c_str(), rszObjectTypeName.c_str(), szSrcScenePath.c_str() );
		Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	// check for destination file
	if ( WaitForFile( szDestinationPath, 10000, false ) == false ) 
	{
		// old file name style ?
		string _szDestinationPath = szDestinationFolder + StrFmt( "%d", pManipulator->GetID( "" ) );
		if ( !NFile::DoesFileExist( _szDestinationPath ) ) 
		{
			const string szError = StrFmt( "Object \"%s\" (of type \"%s\") destination file \"%s\" doesn't exist (not exported yet?)\n", rszObjectName.c_str(), rszObjectTypeName.c_str(), szDestinationPath.c_str() );
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
		const string szError = StrFmt( "General fail during check object \"%s\" of type \"%s\"\n", szObjName.c_str(), rszObjectTypeName.c_str() );
		Log( LT_ERROR, szError );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}

// basement storage  


