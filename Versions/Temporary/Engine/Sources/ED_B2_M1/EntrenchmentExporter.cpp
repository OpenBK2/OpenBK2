#include "stdafx.h"

#include "MapEditorLib/ExporterFactory.h"
#include "EntrenchmentExporter.h"
#include "libdb/ResourceManager.h"
#include "ExporterMethods.h"
#include "MapEditorLib/ManipulatorManager.h"

REGISTER_EXPORTER_IN_DLL( EntrenchmentRPGStats, CEntrenchmentExporter )


EXPORT_RESULT CEntrenchmentExporter::ExportObject( IManipulator* pManipulator,
																									 const std::string &rszObjectTypeName,
																									 const std::string &rszObjectName,
 																									 bool bForce,
																									 EXPORT_TYPE exportType )
{
	EXPORT_RESULT eResult = ER_SUCCESS;
	if ( ( exportType == ET_AFTER_REF ) || ( exportType == ET_NO_REF ) )
	{
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		if ( pResourceManager == 0 )
		{
			return ER_FAIL;
		}
		if ( CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( rszObjectTypeName, CDBID( rszObjectName ) ) )
		{
			int nSegments = 0;
			CManipulatorManager::GetValue( &nSegments, pManipulator, "segments" );
			for ( int nSegmentIndex = 0; nSegmentIndex < nSegments; ++nSegmentIndex )
			{
				const std::string szSegmentName = StrFmt( "segments.[%d].", nSegmentIndex );
				std::string szObjName;
				if ( CManipulatorManager::GetValue( &szObjName, pManipulator, szSegmentName + "VisObj" ) && ( !szObjName.empty() ) )
				{
					CPtr<IManipulator> pVisObjMan = pResourceManager->CreateObjectManipulator( "VisObj", szObjName );
					CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
					CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, 0, 0 );
					CPtr<IManipulator> pAIGeomMan = CManipulatorManager::CreateManipulatorFromReference( "AIGeometry", pGeomMan, 0, 0, 0 );
					if ( pAIGeomMan != 0 ) 
					{
						CVec3 vAABBCenter;
						CVec3 vAABBHalfSize;
						CManipulatorManager::GetVec3<CVec3, float>( &vAABBCenter, pAIGeomMan, "AABBCenter" );
						CManipulatorManager::GetVec3<CVec3, float>( &vAABBHalfSize, pAIGeomMan, "AABBHalfSize" );
						//
						Vis2AI( &vAABBCenter );
						Vis2AI( &vAABBHalfSize );
						//
						CManipulatorManager::SetVec3( vAABBHalfSize, pManipulator, szSegmentName + "AABBHalfSize" );
						CManipulatorManager::SetVec2( CVec2( vAABBCenter.x, vAABBCenter.y ), pManipulator, szSegmentName + "AABBCenter" );
					}

					// Export fireplaces
					pManipulator->RemoveNode( "fireplaces" );

					CGrannyBoneAttributesList attribs;
					int nFireplaceIndex = 0;
					if ( GetGeometryAttributes( pGeomMan, &attribs ) )
					{
						for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
						{
							if ( !PatMat( it->szBoneName.c_str(), "lfireplace??" ) )			
								continue;

							if ( !pManipulator->InsertNode( "fireplaces" ) ) 
								continue;

							std::string szNodeName = szSegmentName + StrFmt( "fireplaces.[%d]", nFireplaceIndex );
							CVec2 vPos( 0, 0 );

							it->GetAttribute( "translatex", &vPos.x );
							it->GetAttribute( "translatey", &vPos.y );

							Vis2AI( &vPos );

							CManipulatorManager::SetVec2( vPos, pManipulator, szNodeName ); 

							++nFireplaceIndex;
						}
					}
				}
			}
		}
	}
	return eResult;
}

// basement storage  


