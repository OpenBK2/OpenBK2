#include "stdafx.h"
#include <fmt/format.h>

#include "ParticleExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "libdb/ResourceManager.h"

#include "MapEditorLib/CommonExporterMethods.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "Misc/StrProc.h"

#include "3Dmotor/GParticleFormat.h"

#include <thread>

REGISTER_EXPORTER_IN_DLL( Particle, CParticleExporter )
//
#define ADD_PATH "bin\\effects\\"

CParticleExporter::CParticleExporter() : bNeedExport( false )
{
}


bool CParticleExporter::StartExport( const std::string &rszObjectTypeName, bool bForce )
{
	CStoreObjectExporter::StartExport( rszObjectTypeName, bForce );
	bNeedExport = false;
	MEStartScript( &szScriptText, true );
	//
	szScriptText += fmt::format( "loadPlugin \"{}\";\r\n", GetOption(&SUserData::SMayaExportData::szOldPluginFileName).c_str() );
	return true;
}


EXPORT_RESULT CParticleExporter::ExportObject( IManipulator* pManipulator,
																							 const std::string &rszObjectTypeName,
																							 const std::string &rszObjectName,
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
	std::string szSorceValue;
	CManipulatorManager::GetValue( &szSorceValue, pManipulator, "SrcName" );
	std::string szSource = pUserData->constUserData.szExportSourceFolder + szSorceValue;
	NFile::NormalizePath( &szSource );
	const std::string szParticleFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + ADD_PATH;
	//const std::string szDestination = szParticleFolder + std::to_string(  nObjectID );
	std::string szDestination = BuildDestFilePath( pManipulator, szParticleFolder ); // uid
	NFile::NormalizePath( &szDestination );
	// check for source and destination times if not forced mode
	if ( CheckFilesUpdated( szSource, szDestination, bForce ) )
	{
		return ER_SUCCESS;
	}
	//
	NFile::CreatePath( szParticleFolder.c_str() );
	//
	std::string szTempDstName = NFile::GetTempFileName() + ".ptc";
	NFile::NormalizePath( &szTempDstName );
	//
	std::string szObjectName;
	NFile::NormalizePath( &szObjectName, rszObjectName );
	//
	std::string szExportPrefix;
	CManipulatorManager::GetValue( &szExportPrefix, pManipulator, "ExportPrefix" );
	//
	szScriptText += fmt::format( "print \"{}\";\r\n", szObjectName.c_str() );
//	szScriptText += fmt::format( "sysFile -del \"{}\";\r\n", szDestination.c_str() );
	szScriptText += fmt::format( "file -o -f \"{}\";\r\n", szSource.c_str() );
	szScriptText += fmt::format( "string $list[] = `ls -type objectSet \"{}*\"`;\r\n", szExportPrefix.c_str() );
	szScriptText += fmt::format( "select -cl;\r\n" );
	szScriptText += fmt::format( "for ($item in $list)\r\n" );
	szScriptText += fmt::format( "  select -add -ne $item;\r\n" );
	szScriptText += fmt::format( "file -es -f -typ \"{}\" -op \"particles=1;{};\" \"{}\";\r\n",
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

void CParticleExporter::FinishExport( const std::string &rszObjectTypeName, bool bForce )
{
	MEFinishScript( &szScriptText, true );
	MERunScript( szScriptText, rszObjectTypeName, bNeedExport, true );
	//
	if ( GetObjectSet().objectNameSet.empty() ) 
		return;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string szParticleFolder	= Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + ADD_PATH;
	//
	for ( CObjectNameSet::const_iterator it = GetObjectSet().objectNameSet.begin(); it != GetObjectSet().objectNameSet.end(); ++it )
	{
		if ( CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( GetObjectSet().szObjectTypeName, it->first ) )
		{
			//std::string szDstFileName = fmt::format( "{}{}", szParticleFolder.c_str(), nObjectID );
			std::string szDstFileName = BuildDestFilePath( pManipulator, szParticleFolder );  // uid
			NFile::NormalizePath( &szDstFileName );
			const std::string szTempFileName = exportedFilesMap[szDstFileName];
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


bool CParticleExporter::ImportInfoToDBAfterRefs( const std::string &szObjName, 
																								 const std::string &szDstFileName,
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
		std::this_thread::yield();
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


