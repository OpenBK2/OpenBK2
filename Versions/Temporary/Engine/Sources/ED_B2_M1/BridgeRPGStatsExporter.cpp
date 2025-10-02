#include "StdAfx.h"

#include "../misc/strproc.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../libdb/ResourceManager.h"
#include "BridgeRPGStatsExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"
#include "../System/FilePath.h"
#include "../System/FileUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const char *AI_GEOMETRY_PREFIX[] = { "mAI", "center", "border" };
//static const char GEOMETRY_FOLDER[] = "bin\\Geometries\\";
static const char BRIDGE_FOLDER[] = "bin\\Bridges\\";

REGISTER_EXPORTER_IN_DLL( BridgeRPGStats, CBridgeRPGStatsExporter )


void CBridgeRPGStatsExporter::GetTempAIGeometryName( string *pszAIGeometryName, const string &rszVisObjectName, const CDBID &rDBID, EAIGeometry eAIGeometry )
{
	NI_ASSERT( pszAIGeometryName != 0, "CBridgeRPGStatsExporter::GetTempAIGeometryName() pszAIGeometryName == 0" );
	const string szKey = StrFmt( "%s%s\\%s", rszVisObjectName.c_str(), AI_GEOMETRY_PREFIX[eAIGeometry], rDBID.ToString().c_str() );
	CTempNamesMap::const_iterator pos = tempNamesMap.find( szKey );
	if ( pos != tempNamesMap.end() )
		*pszAIGeometryName = pos->second;
	else
	{
		const string szTempFileName = NFile::GetTempFileName() + ".gr2";
		tempNamesMap[szKey] = szTempFileName;
		*pszAIGeometryName = szTempFileName;
	}
}


void CBridgeRPGStatsExporter::GetVisObjectNameList( list<string> *pVisOblectNameList, IManipulator *pManipulator )
{
	NI_ASSERT( pVisOblectNameList != 0, "CBridgeRPGStatsExporter::GetVisObjectNameList() pVisOblectNameList == 0" );
	NI_ASSERT( pManipulator != 0, "CBridgeRPGStatsExporter::GetVisObjectNameList() pManipulator == 0" );
	//
	int nEndCount = 0;
	int nCenterCount = 0;
	CManipulatorManager::GetValue( &nEndCount, pManipulator, "End.VisualObjects" );
	CManipulatorManager::GetValue( &nCenterCount, pManipulator, "Center.VisualObjects" );
	pVisOblectNameList->clear();
	if ( nEndCount > 0 )
	{
		pVisOblectNameList->push_back( "End" );
	}
	if ( nCenterCount > 0 )
	{
		pVisOblectNameList->push_back( "Center" );
	}
}


void CBridgeRPGStatsExporter::EnlargeArray( CArray2D<BYTE> *pDestination, const CVec2 &rvDestination, const CVec2 &rvSource )
{
	NI_ASSERT( pDestination != 0, "CBridgeRPGStatsExporter::EnlargeArray() pDestination == 0" );
	if ( rvDestination == rvSource )
	{
		return;
	}
	CVec2 vDifference = ( rvSource - rvDestination ); 
	NI_ASSERT( ( vDifference.x > 0.0f ) || ( vDifference.y > 0.0f ), StrFmt( "CBridgeRPGStatsExporter:EnlargeArray() wrong sign: (%g,%g)", vDifference.x, vDifference.y ) );
	CTPoint<int> difference( 0.5f + ( vDifference.x / AI_TILE_SIZE ), 0.5f + ( vDifference.y / AI_TILE_SIZE ) );
	if ( ( difference.x == 0 ) && ( difference.y == 0 ) )
	{
		return;
	}
	CTPoint<int> size( pDestination->GetSizeX(), pDestination->GetSizeY() );
	CArray2D<BYTE> backup = ( *pDestination );
	pDestination->SetSizes( difference.x + size.x, difference.y + size.y );
	pDestination->FillZero();
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			( *pDestination )[difference.y + nYIndex][difference.x + nXIndex] = backup[nYIndex][nXIndex];
		}
	}
}


void CBridgeRPGStatsExporter::EnlargeArray( CArray2D<BYTE> *pDestination, const CTPoint<int>  &rSourceSize )
{
	NI_ASSERT( pDestination != 0, "CBridgeRPGStatsExporter::EnlargeArray() pDestination == 0" );
	CTPoint<int> size( pDestination->GetSizeX(), pDestination->GetSizeY() );
	if ( size == rSourceSize )
	{
		return;
	}
	if ( ( rSourceSize.x < size.x ) || ( rSourceSize.y < size.y ) )
	{
		return;
	}
	CArray2D<BYTE> backup = ( *pDestination );
	pDestination->SetSizes( rSourceSize.x, rSourceSize.y );
	pDestination->FillZero();
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			( *pDestination )[nYIndex][nXIndex] = backup[nYIndex][nXIndex];
		}
	}
}


void CBridgeRPGStatsExporter::EnlargeXSide( CArray2D<BYTE> *pDestination, CVec2 *pOrigin, int nAITileCount )
{
	NI_ASSERT( pDestination != 0, "CBridgeRPGStatsExporter::EnlargeArray() pDestination == 0" );
	NI_ASSERT( pOrigin != 0, "CBridgeRPGStatsExporter::EnlargeArray() pOrigin == 0" );
	CTPoint<int> size( pDestination->GetSizeX(), pDestination->GetSizeY() );
	CArray2D<BYTE> backup = ( *pDestination );
	pDestination->SetSizes( size.x + ( 2 * nAITileCount ), size.y );
	pDestination->FillZero();
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			( *pDestination )[nYIndex][nXIndex + nAITileCount] = backup[nYIndex][nXIndex];
		}
	}
	//
	if ( nAITileCount > 0 )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			for ( int nAITileIndex = 0; nAITileIndex < nAITileCount; ++nAITileIndex )
			{
				( *pDestination )[nYIndex][nAITileIndex] = backup[nYIndex][0];
				( *pDestination )[nYIndex][size.x + nAITileCount + nAITileIndex] = backup[nYIndex][size.x - 1];
			}
		}
	}
	pOrigin->x += ( AI_TILE_SIZE * nAITileCount );
}


void CBridgeRPGStatsExporter::EnlargeYSide( CArray2D<BYTE> *pDestination, CVec2 *pOrigin, bool bMakeStep, int nAITileCount )
{
	NI_ASSERT( pDestination != 0, "CBridgeRPGStatsExporter::EnlargeArray() pDestination == 0" );
	NI_ASSERT( pOrigin != 0, "CBridgeRPGStatsExporter::EnlargeArray() pOrigin == 0" );
	CTPoint<int> size( pDestination->GetSizeX(), pDestination->GetSizeY() );
	CArray2D<BYTE> backup = ( *pDestination );
	pDestination->SetSizes( size.x, size.y + ( 2 * nAITileCount ) + ( bMakeStep ? 1 : 0 ) );
	pDestination->FillZero();
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			( *pDestination )[nYIndex + nAITileCount][nXIndex] = backup[nYIndex][nXIndex];
		}
	}
	//
	if ( nAITileCount > 0 )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			for ( int nAITileIndex = 0; nAITileIndex < nAITileCount; ++nAITileIndex )
			{
				( *pDestination )[nAITileIndex][nXIndex] = 0x00;
				( *pDestination )[size.y + nAITileCount + nAITileIndex + ( bMakeStep ? 1 : 0 )][nXIndex] = 0x00;
			}
		}
	}
	if ( bMakeStep )
	{
		for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
		{
			( *pDestination )[size.y + nAITileCount][nXIndex] = 0x00;
		}
	}
	pOrigin->y = pDestination->GetSizeY() * AI_TILE_SIZE / 2.0f;
}


void CBridgeRPGStatsExporter::SetArrayInfo( CArray2D<BYTE> *pDestination, const CArray2D<BYTE> &rSource, LOCK_TYPE lockType )
{
	NI_ASSERT( pDestination != 0, "CBridgeRPGStatsExporter::SetArrayInfo() pDestination == 0" );
	CTPoint<int> size( pDestination->GetSizeX(), pDestination->GetSizeY() );
	CTPoint<int> sourceSize( rSource.GetSizeX(), rSource.GetSizeY() );
	NI_ASSERT( size == sourceSize, StrFmt( "CBridgeRPGStatsExporter::SetArrayInfo() wrong size: dest:(%d,%d), source(%d,%d) ", size.x, size.y, sourceSize.x, sourceSize.y ) );
	for ( int nXIndex = 0; nXIndex < size.x; ++nXIndex )
	{
		for ( int nYIndex = 0; nYIndex < size.y; ++nYIndex )
		{
			if ( rSource[nYIndex][nXIndex] > 0 )
			{
				switch( lockType )
				{
					case LOCK_TILE:
						( *pDestination )[nYIndex][nXIndex] &= 0xF0;
						( *pDestination )[nYIndex][nXIndex] |= 0x01;
						break;
					case SHIP_LOCK_TILE:
						( *pDestination )[nYIndex][nXIndex] |= 0x10;
						break;
					case UNLOCK_TILE:
						( *pDestination )[nYIndex][nXIndex] &= 0xF0;
						( *pDestination )[nYIndex][nXIndex] |= 0x02;
						break;
					case SHIP_UNLOCK_TILE:
					default:
						break;
				}
			}
		}
	}
}


void CBridgeRPGStatsExporter::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	CStaticObjectRPGStatsExporter::FinishExport( rszObjectTypeName, bForce );
	//
}


void CBridgeRPGStatsExporter::ExportAdditionalInfo( IManipulator *pManipulator, const string &rszObjectName, const CDBID &rDBID )
{
	list<string> visualObjectNameList;
	GetVisObjectNameList( &visualObjectNameList, pManipulator );
	for ( list<string>::const_iterator itVisualObject = visualObjectNameList.begin(); itVisualObject != visualObjectNameList.end(); ++itVisualObject )
	{
		// Получаем манипулятор на VisObject
		CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( ( *itVisualObject ) + ".VisualObjects.[0]", pManipulator, 0, 0, 0 );
		if ( !pVisObjectManipulator )
		{
			return;
		}
		// Получаем манипулятор модель сезона по умолчанию ( летнюю )
		CPtr<IManipulator> pModelManipulator = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 );
		if ( pModelManipulator == 0 )
		{
			return;
		}
		// Получаем Геометрию, чтобы определить имя файла со сценой
		string szSorceValue;
		{
			if ( CPtr<IManipulator> pGeometryManipulator = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelManipulator, 0, 0, 0 ) )
			{
				CManipulatorManager::GetValue( &szSorceValue, pGeometryManipulator, "SrcName" );
			}
		}
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		//
		const string szScriptTemplate = GetTextTemplate( "ExportAIGeometry" );
		const string szSettingsFileName = GetGrannyExportSettingsFileName( "AIGeometry" );
		// Формируем тело скрипта
		string szTempMAIGeometryName;
		string szTempCenterAIGeometryName;
		string szTempBorderAIGeometryName;
		GetTempAIGeometryName( &szTempMAIGeometryName, *itVisualObject, rDBID, AIG_MAI );
		GetTempAIGeometryName( &szTempCenterAIGeometryName, *itVisualObject, rDBID, AIG_CENTER );
		GetTempAIGeometryName( &szTempBorderAIGeometryName, *itVisualObject, rDBID, AIG_BORDER );
		//
		string szObjectName = rszObjectName;
		NStr::ReplaceAllChars( &szObjectName, '\\', '/' );
		string szSource = pUserData->constUserData.szExportSourceFolder + szSorceValue;
		NStr::ReplaceAllChars( &szSource, '\\', '/' );
		string szMAIDestination = szTempMAIGeometryName;
		NStr::ReplaceAllChars( &szMAIDestination, '\\', '/' );
		string szCenterDestination = szTempCenterAIGeometryName;
		NStr::ReplaceAllChars( &szCenterDestination, '\\', '/' );
		string szBorderDestination = szTempBorderAIGeometryName;
		NStr::ReplaceAllChars( &szBorderDestination, '\\', '/' );
		string szBridgeFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + BRIDGE_FOLDER;
		NFile::CreatePath( szBridgeFolder.c_str() );

		// main AI geometry
		string szScriptText = StrFmt( szScriptTemplate.c_str(),
			szObjectName.c_str(), szMAIDestination.c_str(), szSource.c_str(),
			AI_GEOMETRY_PREFIX[AIG_MAI], "",
			szSettingsFileName.c_str() );
		szScriptText += ";\n";
		ExecuteMayaScript( szScriptText );
		// center part
		szScriptText = StrFmt( szScriptTemplate.c_str(),
			szObjectName.c_str(), szCenterDestination.c_str(), szSource.c_str(),
			AI_GEOMETRY_PREFIX[AIG_CENTER], "",
			szSettingsFileName.c_str() );
		szScriptText += ";\n";
		ExecuteMayaScript( szScriptText );
		// borders
		szScriptText = StrFmt( szScriptTemplate.c_str(),
			szObjectName.c_str(), szBorderDestination.c_str(), szSource.c_str(),
			AI_GEOMETRY_PREFIX[AIG_BORDER], "",
			szSettingsFileName.c_str() );
		szScriptText += ";\n";
		ExecuteMayaScript( szScriptText );

	}
}

EXPORT_RESULT CBridgeRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																										 const string &rszObjectTypeName,
																										 const string &rszObjectName,
																										 bool bForce,
																										 EXPORT_TYPE exportType )
{
	CDBID objectDBID = CDBID( rszObjectName );
	CStaticObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	if ( exportType == ET_BEFORE_REF )
		return ER_SUCCESS;
	//
	ExportAdditionalInfo( pManipulator, rszObjectName, objectDBID );
	//

	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	// Формируем тело скрипта
	const string szBridgeFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + BRIDGE_FOLDER;
	//const string szGeometryFolder = pUserData->szExportDestinationFolder + GEOMETRY_FOLDER;
	bool bResult = true;
	//
	list<string> visualObjectNameList;
	GetVisObjectNameList( &visualObjectNameList, pManipulator );
	float fHeight = 0.0f;
	bool bHeightCalculated = false;
	//
	list<CArray2D<BYTE> > passabilityArrayList;
	list<CVec2> passabilityOriginList;
	list<CVec2> sizeList;
	//
	for ( list<string>::const_iterator itVisualObject = visualObjectNameList.begin(); itVisualObject != visualObjectNameList.end(); ++itVisualObject )
	{
		string szTempMAIGeometryName;
		string szTempCenterAIGeometryName;
		string szTempBorderAIGeometryName;
		GetTempAIGeometryName( &szTempMAIGeometryName, *itVisualObject, objectDBID, AIG_MAI );
		GetTempAIGeometryName( &szTempCenterAIGeometryName, *itVisualObject, objectDBID, AIG_CENTER );
		GetTempAIGeometryName( &szTempBorderAIGeometryName, *itVisualObject, objectDBID, AIG_BORDER );
		//
		//const string szDestination = szGeometryFolder + StrFmt( "%d", nGeometryID );
		const string szMAIDestination = szTempMAIGeometryName;
		const string szCenterDestination = szTempCenterAIGeometryName;
		const string szBorderDestination = szTempBorderAIGeometryName;
		//
		CVec2 vPassabilityOrigin = VNULL2;
		CVec2 vSize = VNULL2;
		CArray2D<BYTE> passabilityArray;
		// создаем массивы проходимостей
		CArray2D<BYTE> bridgePassabilityArray;
		CArray2D<BYTE> centerPassabilityArray;
		CArray2D<BYTE> borderPassabilityArray;
		CVec2 vBridgePassabilityOrigin = VNULL2;
		CVec2 vCenterPassabilityOrigin = VNULL2;
		CVec2 vBorderPassabilityOrigin = VNULL2;
		bResult = bResult && CreateObjectPassability( szMAIDestination, &bridgePassabilityArray, &vBridgePassabilityOrigin );
		bResult = bResult && CreateObjectPassability( szCenterDestination, &centerPassabilityArray, &vCenterPassabilityOrigin );
		bResult = bResult && CreateObjectPassability( szBorderDestination, &borderPassabilityArray, &vBorderPassabilityOrigin );
		//Trace2DByteArray( bridgePassabilityArray, StrFmt( " (%g,%g)", vBridgePassabilityOrigin.x, vBridgePassabilityOrigin.y ) );
		//Trace2DByteArray( centerPassabilityArray, StrFmt( " (%g,%g)", vCenterPassabilityOrigin.x, vCenterPassabilityOrigin.y ) );
		//Trace2DByteArray( borderPassabilityArray, StrFmt( " (%g,%g)", vBorderPassabilityOrigin.x, vBorderPassabilityOrigin.y ) );
		if ( bResult &&
			( ( bridgePassabilityArray.GetSizeX() > 0 ) && ( bridgePassabilityArray.GetSizeY() > 0 ) ) &&
			( ( centerPassabilityArray.GetSizeX() > 0 ) && ( centerPassabilityArray.GetSizeY() > 0 ) ) &&
			( ( borderPassabilityArray.GetSizeX() > 0 ) && ( borderPassabilityArray.GetSizeY() > 0 ) ) )
		{
			// Центрируем origin
			try
			{
				CVec3 vMin = VNULL3;
				CVec3 vMax = VNULL3;
				WaitForFile( szMAIDestination, 10000 );
				granny_file *pFile = GrannyReadEntireFile( szMAIDestination.c_str() );
				granny_file_info *pInfo = GrannyGetFileInfo( pFile );
				GetGrannyMeshBoundingBox( &vMin, &vMax, pInfo );
				GrannyFreeFile( pFile );
				Vis2AI( &vMin ); 
				Vis2AI( &vMax ); 
				NormalizePassabilityOrigin( &vBridgePassabilityOrigin,
					CTPoint<int>( bridgePassabilityArray.GetSizeX(),
					bridgePassabilityArray.GetSizeY() ),
					vMin,
					vMax );
			}
			catch ( ... ) 
			{
				return ER_SUCCESS;
			}
			try
			{
				CVec3 vMin = VNULL3;
				CVec3 vMax = VNULL3;
				WaitForFile( szCenterDestination, 10000 );
				CGrannyFileInfoGuard pInfo( szCenterDestination );
				GetGrannyMeshBoundingBox( &vMin, &vMax, pInfo );
				Vis2AI( &vMin ); 
				Vis2AI( &vMax ); 
				NormalizePassabilityOrigin( &vCenterPassabilityOrigin,
					CTPoint<int>( centerPassabilityArray.GetSizeX(),
					centerPassabilityArray.GetSizeY() ),
					vMin,
					vMax );
				if ( !bHeightCalculated )
				{
					fHeight = vMax.z;
					bHeightCalculated = true;
				}
			}
			catch ( ... ) 
			{
				return ER_SUCCESS;
			}
			try
			{
				CVec3 vMin = VNULL3;
				CVec3 vMax = VNULL3;
				WaitForFile( szBorderDestination, 10000 );
				CGrannyFileInfoGuard pInfo( szBorderDestination );
				GetGrannyMeshBoundingBox( &vMin, &vMax, pInfo );
				Vis2AI( &vMin ); 
				Vis2AI( &vMax ); 
				NormalizePassabilityOrigin( &vBorderPassabilityOrigin,
					CTPoint<int>( borderPassabilityArray.GetSizeX(),
					borderPassabilityArray.GetSizeY() ),
					vMin,
					vMax );
				vSize = CVec2( vMax.x - vMin.x, vMax.y - vMin.y );
			}
			catch ( ... ) 
			{
				return ER_SUCCESS;
			}
			//Trace2DByteArray( bridgePassabilityArray, StrFmt( " (%g,%g)", vBridgePassabilityOrigin.x, vBridgePassabilityOrigin.y ) );
			//Trace2DByteArray( centerPassabilityArray, StrFmt( " (%g,%g)", vCenterPassabilityOrigin.x, vCenterPassabilityOrigin.y ) );
			//Trace2DByteArray( borderPassabilityArray, StrFmt( " (%g,%g)", vBorderPassabilityOrigin.x, vBorderPassabilityOrigin.y ) );
			// находим максимальный origin
			vPassabilityOrigin = vBridgePassabilityOrigin;
			if ( vCenterPassabilityOrigin.x > vPassabilityOrigin.x )
			{
				vPassabilityOrigin.x = vCenterPassabilityOrigin.x;
			}
			if ( vBorderPassabilityOrigin.x > vPassabilityOrigin.x )
			{
				vPassabilityOrigin.x = vBorderPassabilityOrigin.x;
			}
			if ( vCenterPassabilityOrigin.y > vPassabilityOrigin.y )
			{
				vPassabilityOrigin.y = vCenterPassabilityOrigin.y;
			}
			if ( vBorderPassabilityOrigin.y > vPassabilityOrigin.y )
			{
				vPassabilityOrigin.y = vBorderPassabilityOrigin.y;
			}
			// увеличиваем размер массивов со стороны (0,0)
			EnlargeArray( &bridgePassabilityArray, vBridgePassabilityOrigin, vPassabilityOrigin );
			EnlargeArray( &centerPassabilityArray, vCenterPassabilityOrigin, vPassabilityOrigin );
			EnlargeArray( &borderPassabilityArray, vBorderPassabilityOrigin, vPassabilityOrigin );
			// находим максимальный размер массива
			CTPoint<int> size( bridgePassabilityArray.GetSizeX(), bridgePassabilityArray.GetSizeY() );
			if ( centerPassabilityArray.GetSizeX() > size.x )
			{
				size.x = centerPassabilityArray.GetSizeX();
			}
			if ( borderPassabilityArray.GetSizeX() > size.x )
			{
				size.x = borderPassabilityArray.GetSizeX();
			}
			if ( centerPassabilityArray.GetSizeY() > size.y )
			{
				size.y = centerPassabilityArray.GetSizeY();
			}
			if ( borderPassabilityArray.GetSizeY() > size.y )
			{
				size.y = borderPassabilityArray.GetSizeY();
			}
			// увеличиваем размер массивов с другой стороны
			EnlargeArray( &bridgePassabilityArray, size );
			EnlargeArray( &centerPassabilityArray, size );
			EnlargeArray( &borderPassabilityArray, size );
			//Trace2DByteArray( bridgePassabilityArray, StrFmt( " (%g,%g)", vBridgePassabilityOrigin.x, vBridgePassabilityOrigin.y ) );
			//Trace2DByteArray( centerPassabilityArray, StrFmt( " (%g,%g)", vCenterPassabilityOrigin.x, vCenterPassabilityOrigin.y ) );
			//Trace2DByteArray( borderPassabilityArray, StrFmt( " (%g,%g)", vBorderPassabilityOrigin.x, vBorderPassabilityOrigin.y ) );
			passabilityArray.SetSizes( size.x, size.y );
			passabilityArray.FillZero();
			SetArrayInfo( &passabilityArray, bridgePassabilityArray, SHIP_LOCK_TILE );
			SetArrayInfo( &passabilityArray, centerPassabilityArray, UNLOCK_TILE );
			SetArrayInfo( &passabilityArray, borderPassabilityArray, LOCK_TILE );
			//Trace2DByteArray( passabilityArray, StrFmt( " (%g,%g)", vPassabilityOrigin.x, vPassabilityOrigin.y ) );
			EnlargeXSide( &passabilityArray, &vPassabilityOrigin, 2 );
			//Trace2DByteArray( passabilityArray, StrFmt( " (%g,%g)", vPassabilityOrigin.x, vPassabilityOrigin.y ) );
			NormalizePassabilityArray( &passabilityArray, &vPassabilityOrigin );
			//vPassabilityOrigin.y = int( vPassabilityOrigin.y + 0.5f );
			passabilityArrayList.push_back( passabilityArray );
			passabilityOriginList.push_back( vPassabilityOrigin );
			sizeList.push_back( vSize );
		}
		if ( !bResult )
		{
			break;
		}
	}
	if ( bHeightCalculated )
	{
		bResult = bResult && CManipulatorManager::SetValue( fHeight, pManipulator, "Height" );
	}
	// Приводим массивы к одинаковой четности по стороне Y
	{
		int nMaxYSize = 0;
		{
			list<CArray2D<BYTE> >::const_iterator itPassabilityArray = passabilityArrayList.begin();
			for ( ; itPassabilityArray != passabilityArrayList.end(); )
			{
				if ( nMaxYSize < itPassabilityArray->GetSizeY() )
				{
					nMaxYSize = itPassabilityArray->GetSizeY();
				}
				++itPassabilityArray;
			}
		}
		{
			list<CArray2D<BYTE> >::iterator itPassabilityArray = passabilityArrayList.begin();
			list<CVec2>::iterator itPassabilityOrigin = passabilityOriginList.begin();
			for ( ; itPassabilityArray != passabilityArrayList.end(); )
			{
				if ( nMaxYSize > itPassabilityArray->GetSizeY() )
				{
					EnlargeYSide( &( *itPassabilityArray ),
						&( *itPassabilityOrigin ), 
						( itPassabilityArray->GetSizeY() & 0x01 ) != ( nMaxYSize & 0x01 ),
						( ( nMaxYSize - itPassabilityArray->GetSizeY() ) & ( 0xFFFFFFFF - 0x01 ) ) / 2 );
				}
				++itPassabilityArray;
				++itPassabilityOrigin;
			}
		}
	}
	// Прописываем данные
	{
		list<CArray2D<BYTE> >::const_iterator itPassabilityArray = passabilityArrayList.begin();
		list<CVec2>::const_iterator itPassabilityOrigin = passabilityOriginList.begin();
		list<CVec2>::const_iterator itSize = sizeList.begin();
		//
		for ( list<string>::const_iterator itVisualObject = visualObjectNameList.begin(); itVisualObject != visualObjectNameList.end(); ++itVisualObject )
		{
			// Удаляем старый массив
			bResult = bResult && CManipulatorManager::Remove2DArray( pManipulator, ( *itVisualObject ) + ".Passability" );
			// Добавляем новую информацию
			bResult = bResult && CManipulatorManager::Set2DArray( *itPassabilityArray,	pManipulator, ( *itVisualObject ) + ".Passability" );
			bResult = bResult && CManipulatorManager::SetVec2(		*itPassabilityOrigin,	pManipulator, ( *itVisualObject ) + ".Origin" );
			bResult = bResult && CManipulatorManager::SetVec2(		*itSize,							pManipulator, ( *itVisualObject ) + ".Size" );
			//
			Trace2DByteArray( *itPassabilityArray, StrFmt( " (%g,%g)", itPassabilityOrigin->x, itPassabilityOrigin->y ) );
			//
			++itPassabilityArray;
			++itPassabilityOrigin;
			++itSize;
			if ( !bResult )
			{
				break;
			}
		}
	}
	// Удаляем созданные временные файлы
	for ( list<string>::const_iterator itVisualObject = visualObjectNameList.begin(); itVisualObject != visualObjectNameList.end(); ++itVisualObject )
	{
		string szTempMAIGeometryName;
		string szTempCenterAIGeometryName;
		string szTempBorderAIGeometryName;
		GetTempAIGeometryName( &szTempMAIGeometryName, *itVisualObject, objectDBID, AIG_MAI );
		GetTempAIGeometryName( &szTempCenterAIGeometryName, *itVisualObject, objectDBID, AIG_CENTER );
		GetTempAIGeometryName( &szTempBorderAIGeometryName, *itVisualObject, objectDBID, AIG_BORDER );
		//
		const string szMAIDestination = szTempMAIGeometryName;
		const string szCenterDestination = szTempCenterAIGeometryName;
		const string szBorderDestination = szTempBorderAIGeometryName;
		//
		::DeleteFile( szMAIDestination.c_str() );
		::DeleteFile( szCenterDestination.c_str() );
		::DeleteFile( szBorderDestination.c_str() );
	}
	// Удаляем временный каталог ( только если он пустой )
	{
		list<string> szFileNameList;
		NFile::GetDirectoryFiles( szBridgeFolder.c_str(), "*", &szFileNameList, true );
		if ( szFileNameList.empty() )
		{
			NFile::DeleteDirectory( szBridgeFolder.c_str() );
		}
	}
	//
	return ER_SUCCESS;
}

// basement storage  


