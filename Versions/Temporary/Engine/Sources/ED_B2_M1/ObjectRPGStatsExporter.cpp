#include "stdafx.h"

#include "Misc/StrProc.h"
#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "ObjectRPGStatsExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"

#include <zconf.h>

REGISTER_EXPORTER_IN_DLL( ObjectRPGStats, CObjectRPGStatsExporter )


EXPORT_RESULT CObjectRPGStatsExporter::CheckObject( IManipulator* pManipulator,
																										const string &rszObjectTypeName,
																										const string &rszObjectName,
																										bool bExport,
																										EXPORT_TYPE exportType )
{
	CObjectBaseRPGStatsExporter::CheckObject( pManipulator, rszObjectTypeName, rszObjectName, bExport, exportType );
	//
	if ( exportType == ET_BEFORE_REF )
		return ER_SUCCESS;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	ILogger *pLogger = NLog::GetLogger();
	//
	int nNumSpecificJoints = 0;
	if ( CManipulatorManager::GetValue( &nNumSpecificJoints, pManipulator, "SpecificJoints" ) && nNumSpecificJoints != 0 ) 
	{
		if ( CPtr<IManipulator> pVisObjMan = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pManipulator, 0, 0, 0 ) ) 
		{
			if ( CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 ) ) 
			{
				string szSkeletonName;
				if ( CManipulatorManager::GetParamsFromReference( "Skeleton", pModelMan, 0, &szSkeletonName, 0 ) && !szSkeletonName.empty() )
				{
					//const string szSkeletonFileName = pUserData->szExportDestinationFolder + StrFmt( "bin\\skeletons\\%d", nSkeletonID );
					const string szSkeletonFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\skeletons\\";
					CDBPtr<NDb::SSkeleton> pDBSkeleton = NDb::Get<NDb::SSkeleton>( CDBID( szSkeletonName ) );
					string szSkeletonFileName = NBinResources::GetBinaryFileName( szSkeletonFolder, pDBSkeleton->GetRecordID(), pDBSkeleton->uid ); // uid
					bool bFileExist = WaitForFile( szSkeletonFileName, 10000 );
					if ( !bFileExist )
					{
						szSkeletonFileName = szSkeletonFolder + std::to_string(  pDBSkeleton->GetRecordID() );
						bFileExist =  WaitForFile( szSkeletonFileName, 10000 );
					}
					if ( bFileExist )
					{
						std::unordered_map<string, int> bonesMap;
						CGrannyFileInfoGuard pInfo( szSkeletonFileName );
						for ( int i = 0; i < pInfo->Skeletons[0]->BoneCount; ++i ) 
							bonesMap[pInfo->Skeletons[0]->Bones[i].Name] = 1;
						//
						for ( int i = 0; i < nNumSpecificJoints; ++i ) 
						{
							string szJointName;
							if ( CManipulatorManager::GetValue( &szJointName, pManipulator, StrFmt("SpecificJoints.[%d]", i) ) && !szJointName.empty() )
							{
								if ( bonesMap.find( szJointName ) == bonesMap.end() )
								{
									pLogger->Log( LT_ERROR, "Specific joint doesn't exist in object's skeleton\n" );
									pLogger->Log( LT_ERROR, StrFmt("\tObject: %s\n", rszObjectName.c_str()) );
									pLogger->Log( LT_ERROR, StrFmt("\tSpecific joint name: %s\n", szJointName.c_str()) );
									pLogger->Log( LT_ERROR, StrFmt("\tSpecific joint index: %d\n", i) );
								}
							}
						}
					}
				}
			}
		}
	}
	//
	return ER_SUCCESS;
}

// basement storage  


