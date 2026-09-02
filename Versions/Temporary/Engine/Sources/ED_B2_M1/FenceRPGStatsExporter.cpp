#include "stdafx.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "3Dmotor/DBScene.h"
#include "vendor/granny/include/granny.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"

#include "ExporterMethods.h"
#include "FenceRPGStatsExporter.h"

#include <cstdint>

#include <zconf.h>

REGISTER_EXPORTER_IN_DLL( FenceRPGStats, CFenceRPGStatsExporter )


EXPORT_RESULT CFenceRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																										const std::string &rszObjectTypeName,
																										const std::string &rszObjectName,
																										bool bForce,
																										EXPORT_TYPE exportType )
{
	CStaticObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	if ( exportType == ET_BEFORE_REF )
		return ER_SUCCESS;
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	CArray2D<uint8_t> passabilityArray( 1, 3 );
	passabilityArray[0][0] = 1;
	passabilityArray[1][0] = 1;
	passabilityArray[2][0] = 1;
	CVec3 vPassabilityOrigin( AI_TILE_SIZE / 2.0f, AI_TILE_SIZE / 2.0f, 0 );
	ExportVisobjs( pManipulator, "CenterSegments", passabilityArray, vPassabilityOrigin );

	ExportVisobjs( pManipulator, "DamagedSegments", passabilityArray, vPassabilityOrigin );

	passabilityArray[0][0] = 0;
	passabilityArray[1][0] = 0;
	passabilityArray[2][0] = 0;
	ExportVisobjs( pManipulator, "DestroyedSegments", passabilityArray, vPassabilityOrigin );

	CreatePassProfiles( pManipulator, "CenterSegments" );
	CreatePassProfiles( pManipulator, "DamagedSegments" );
	CreatePassProfiles( pManipulator, "DestroyedSegments" );

	return ER_SUCCESS;
}

bool CFenceRPGStatsExporter::GetGeom0FileName( IManipulator *pManipulator, const std::string &rszSegmentsSetName, std::string *pszGeomFileName )
{
	// Получаем манипулятор на VisObject-ы
	int nNumVisobjs = 0;
	if ( !CManipulatorManager::GetValue( &nNumVisobjs, pManipulator, rszSegmentsSetName + ".VisObjes" ) )
		return false;

	if ( nNumVisobjs <= 0 )
		return false;

	// рассматриваем только первую модель (если их несколько)
	// т.е. passability у них всех должна быть одинаковая
	std::string szObjName = rszSegmentsSetName + ".VisObjes.[0]";

	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	if ( !pResourceManager )
		return false;

	CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( szObjName, pManipulator, 0, 0, 0 );
	if ( !pVisObjectManipulator )
		return false;

	// Получаем манипулятор модель сезона по умолчанию ( летнюю )
	CPtr<IManipulator> pModelManipulator = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 );
	if ( pModelManipulator == 0 )
		return false;

	std::string szGeometryName;
	CManipulatorManager::GetParamsFromReference( "Geometry", pModelManipulator, 0, &szGeometryName, 0 );
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const std::string szGeometriesFolder =	Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Geometries\\";

	CDBPtr<NDb::SGeometry> pGeometry = NDb::Get<NDb::SGeometry>( CDBID( szGeometryName ) );
	*pszGeomFileName = NBinResources::GetExistentBinaryFileName( szGeometriesFolder, pGeometry->GetRecordID(), pGeometry->uid ); // uid
	return true;
}

void CFenceRPGStatsExporter::CreatePassProfiles( IManipulator *pManipulator, const std::string &rszSegmentsSetName )
{
	std::string szGrannyFileName;
	if ( GetGeom0FileName( pManipulator, rszSegmentsSetName, &szGrannyFileName ) )
	{
		NDb::SPassProfile passProfile;
		if ( CreateObjectPassabilityProfile( szGrannyFileName, 1.0f, &passProfile ) )
			SavePassProfile( passProfile, rszSegmentsSetName, "PassProfile", pManipulator ); 
	}
}

bool CFenceRPGStatsExporter::ExportVisobjs( IManipulator *pManipulator, 
																					  const std::string &rszSegmentsSetName, 
																						const CArray2D<uint8_t> &rPassabilityArray,
																						const CVec3 &rvPassabilityOrigin )
{
	// Записываем третий параметр - AI проходимость объекта

	// Удаляем старый массив
	bool bResult = CManipulatorManager::Remove2DArray( pManipulator, rszSegmentsSetName + ".passability" );
	
	if ( bResult )
	{
		bResult = bResult && CManipulatorManager::Set2DArray( rPassabilityArray, pManipulator, rszSegmentsSetName + ".passability" );
		bResult = bResult && pManipulator->SetValue( rszSegmentsSetName + ".Origin.x", rvPassabilityOrigin.x );
		bResult = bResult && pManipulator->SetValue( rszSegmentsSetName + ".Origin.y", rvPassabilityOrigin.y );
	}

	return bResult;
}



