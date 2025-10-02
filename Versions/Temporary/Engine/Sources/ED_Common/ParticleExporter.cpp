#include "StdAfx.h"

#include "ParticleExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../libdb/ResourceManager.h"

#include "../MapEditorLib/CommonExporterMethods.h"
#include "../System/FilePath.h"
#include "../System/FileUtils.h"
#include "../Misc/StrProc.h"

#include "../3Dmotor/GParticleFormat.h"

REGISTER_EXPORTER_IN_DLL( Particle, CParticleExporter )
//
#define ADD_PATH "bin\\effects\\"

CParticleExporter::CParticleExporter() : bNeedExport( false )
{
}


bool CParticleExporter::StartExport( const string &rszObjectTypeName, bool bForce )
{
	CStoreObjectExporter::StartExport( rszObjectTypeName, bForce );
	bNeedExport = false;
	MEStartScript( &szScriptText, true );
	//
	szScriptText += StrFmt( "loadPlugin \"%s\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginFileName).c_str() );
	return true;
}


EXPORT_RESULT CParticleExporter::ExportObject( IManipulator* pManipulator,
																							 const string &rszObjectTypeName,
																							 const string &rszObjectName,
																							 bool bForce,
																							 EXPORT_TYPE exportType )
{
	NI_ASSERT( pManipulator != 0, "CParticleExporter::ExportObject() pManipulator == 0 )" );
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	// Формируем тело скрипта
	string szSorceValue;
	CManipulatorManager::GetValue( &szSorceValue, pManipulator, "SrcName" );
	string szSource = pUserData->constUserData.szExportSourceFolder + szSorceValue;
	NFile::NormalizePath( &szSource );
	const string szParticleFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + ADD_PATH;
	//const string szDestination = szParticleFolder + StrFmt( "%d", nObjectID );
	string szDestination = BuildDestFilePath( pManipulator, szParticleFolder ); // uid
	NFile::NormalizePath( &szDestination );
	// check for source and destination times if not forced mode
	if ( CheckFilesUpdated( szSource, szDestination, bForce ) )
	{
		return ER_SUCCESS;
	}
	//
	NFile::CreatePath( szParticleFolder.c_str() );
	//
	string szTempDstName = NFile::GetTempFileName() + ".ptc";
	NFile::NormalizePath( &szTempDstName );
	//
	string szObjectName;
	NFile::NormalizePath( &szObjectName, rszObjectName );
	//
	string szExportPrefix;
	CManipulatorManager::GetValue( &szExportPrefix, pManipulator, "ExportPrefix" );
	//
	szScriptText += StrFmt( "print \"%s\";\r\n", szObjectName.c_str() );
//	szScriptText += StrFmt( "sysFile -del \"%s\";\r\n", szDestination.c_str() );
	szScriptText += StrFmt( "file -o -f \"%s\";\r\n", szSource.c_str() );
	szScriptText += StrFmt( "string $list[] = `ls -type objectSet \"%s*\"`;\r\n", szExportPrefix.c_str() );
	szScriptText += StrFmt( "select -cl;\r\n" );
	szScriptText += StrFmt( "for ($item in $list)\r\n" );
	szScriptText += StrFmt( "  select -add -ne $item;\r\n" );
	szScriptText += StrFmt( "file -es -f -typ \"%s\" -op \"particles=1;%s;\" \"%s\";\r\n",
			GetOption( &SUserData::SMayaExportData::szOldPluginName ).c_str(), 
			GetOption( &SUserData::SMayaExportData::szOldPluginParticleFixYZOption ).c_str(),
			szTempDstName.c_str()
		);
	exportedFilesMap[szDestination] = szTempDstName;
	//
	bNeedExport = true;
	CStoreObjectExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType  );
	return ER_SUCCESS;
}

void CParticleExporter::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	MEFinishScript( &szScriptText, true );
	MERunScript( szScriptText, rszObjectTypeName, bNeedExport, true );
	//
	if ( GetObjectSet().objectNameSet.empty() ) 
		return;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const string szParticleFolder	= Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + ADD_PATH;
	//
	for ( CObjectNameSet::const_iterator it = GetObjectSet().objectNameSet.begin(); it != GetObjectSet().objectNameSet.end(); ++it )
	{
		if ( CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, it->first ) )
		{
			//string szDstFileName = StrFmt( "%s%d", szParticleFolder.c_str(), nObjectID );
			string szDstFileName = BuildDestFilePath( pManipulator, szParticleFolder );  // uid
			NFile::NormalizePath( &szDstFileName );
			const string szTempFileName = exportedFilesMap[szDstFileName];
			if ( WaitForFile(szTempFileName, 10000, true) != false )
			{
				MoveTempFileToDestination( szTempFileName, szDstFileName );
				ImportInfoToDBAfterRefs( "Unknown name", szDstFileName, pManipulator );
			}
			else
				DeleteFile( szTempFileName.c_str() );
		}
	}
	exportedFilesMap.clear();
}


bool CParticleExporter::ImportInfoToDBAfterRefs( const string &szObjName, 
																								 const string &szDstFileName,
																								 IManipulator *pManipulator )
{
	CPtr<NGScene::CParticlesLoader> pParticle = new NGScene::CParticlesLoader;

	CVariant varUID;
	if ( CManipulatorManager::GetValue( &varUID, pManipulator, "uid" ) )
	{
		GUID uid;
		memcpy( &uid, varUID.GetPtr(), sizeof( uid ) );
		pParticle->SetKey( SIntResKey( uid, pManipulator->GetID( "" ) ) );
	}
	else
		pParticle->SetKey( pManipulator->GetID( "" ) );

	CDGPtr<CPtrFuncBase<NGScene::CParticlesInfo> > pFunc = pParticle;

	pFunc.Refresh();
	NGScene::CParticlesInfo *pInfo = pFunc->GetValue();
	while ( pInfo == 0 )
	{
		Sleep( 0 );
		pInfo = pFunc->GetValue();
	}
	if ( pInfo == 0 )
		return false;
	SBound bound;
	pInfo->CalcBound( &bound );

	pManipulator->SetValue( "Bound", CVariant(&bound, sizeof(bound)) );

	return true;
}

// basement storage  


