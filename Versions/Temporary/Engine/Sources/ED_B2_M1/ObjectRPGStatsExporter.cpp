#include "stdafx.h"
#include <fmt/format.h>

#include "Misc/StrProc.h"
#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "ObjectRPGStatsExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "ED_Common/GltfExporter.h"

#include <zconf.h>

REGISTER_EXPORTER_IN_DLL( ObjectRPGStats, CObjectRPGStatsExporter )


EXPORT_RESULT CObjectRPGStatsExporter::CheckObject( IManipulator* pManipulator,
																										const std::string &rszObjectTypeName,
																										const std::string &rszObjectName,
																										bool bExport,
																										EXPORT_TYPE exportType )
{
	const auto result = CObjectBaseRPGStatsExporter::CheckObject(pManipulator, rszObjectTypeName, rszObjectName, bExport, exportType);
	if ( result != ER_SUCCESS ) return result;
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
				std::string szSkeletonName;
				if ( CManipulatorManager::GetParamsFromReference( "Skeleton", pModelMan, 0, &szSkeletonName, 0 ) && !szSkeletonName.empty() )
				{
					CPtr<IManipulator> skeletonResource = CManipulatorManager::CreateManipulatorFromReference("Skeleton", pModelMan, 0, 0, 0);
					// Validate joints directly in the GLTF hierarchy; no generated GR2 file exists.
					if ( !skeletonResource || !NEditorGltf::IsGltf(skeletonResource) ) return ER_FAIL;
					std::string root;
					CManipulatorManager::GetValue(&root, skeletonResource, "RootJoint");
					NGltf::SSkeletonDefinition skeleton;
					if ( !NGltf::BuildSkeleton(NEditorGltf::Load(skeletonResource), root, 0, &skeleton) ) return ER_FAIL;
					{
						//
						for ( int i = 0; i < nNumSpecificJoints; ++i ) 
						{
							std::string szJointName;
							if ( CManipulatorManager::GetValue( &szJointName, pManipulator, fmt::format("SpecificJoints.[{}]", i) ) && !szJointName.empty() )
							{
								if ( skeleton.FindBone(szJointName) < 0 )
								{
									pLogger->Log( LT_ERROR, "Specific joint doesn't exist in object's skeleton\n" );
									pLogger->Log( LT_ERROR, fmt::format("\tObject: {}\n", rszObjectName.c_str()) );
									pLogger->Log( LT_ERROR, fmt::format("\tSpecific joint name: {}\n", szJointName.c_str()) );
									pLogger->Log( LT_ERROR, fmt::format("\tSpecific joint index: {}\n", i) );
									return ER_FAIL;
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


