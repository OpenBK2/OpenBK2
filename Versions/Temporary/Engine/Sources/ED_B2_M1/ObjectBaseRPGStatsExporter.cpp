#include "StdAfx.h"

#include "../misc/strproc.h"
#include "../misc/2darray.h"
#include "../3dmotor/dbscene.h"
#include "ObjectBaseRPGStatsExporter.h"
#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/StringManager.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../System/FilePath.h"
#include "../MapEditorLib/Interface_MOD.h"

#include "ExporterMethods.h"
#include "SeasonMnemonics.h"

#include <zconf.h>

namespace NDb
{
	// CRAP{ legacy - remove it ASAP
	const CDBID *GetDBID( const int nClassTypeID, const int nRecordID );
	// CRAP}
}

//REGISTER_EXPORTER_IN_DLL( BuildingRPGStats, CObjectBaseRPGStatsExporter )


static bool SetObjectHeight( IManipulator *pObjectBaseRPGStatsManipulator, granny_file_info *pInfo )
{
	CVec3 vMin = VNULL3;
	CVec3 vMax = VNULL3;

	if ( !GetGrannyMeshBoundingBox( &vMin, &vMax, pInfo ) )
		return false;

	int nHeightAI = Vis2AI( vMax.y - vMin.y );
	CVariant v = nHeightAI;
	
	return pObjectBaseRPGStatsManipulator->SetValue( "ObjectHeight", v );
}

bool CObjectBaseRPGStatsExporter::ExportDynamicDebris( IManipulator *pManipulator, const string &szObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	const string szGeometriesFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Geometries\\";
	const string szObjectPath = NFile::GetFilePath( szObjectName );

	string szGeometryName;
	{
		string szVisObj = "";
		CManipulatorManager::GetValue( &szVisObj, pManipulator, "visualObject" );
		int nDamageLevels = 0;
		CManipulatorManager::GetValue( &nDamageLevels, pManipulator, "DamageLevels" );
		for ( int i = 0; i < nDamageLevels; ++i )
		{
			float fHP = 0;
			CManipulatorManager::GetValue( &fHP, pManipulator, StrFmt( "DamageLevels.[%d].DamageHP", i ) );
			if ( fHP <= 0 )
			{
				CManipulatorManager::GetValue( &szVisObj, pManipulator, StrFmt( "DamageLevels.[%d].VisObj", i ) );
				break;
			}
		}
		if ( !szVisObj.empty() )
		{
			if ( CPtr<IManipulator> pVisObjectManipulator = pResourceManager->CreateObjectManipulator( "VisObj", szVisObj ) )
			{
				if ( CPtr<IManipulator> pModelManipulator = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 ) )
				{
					CManipulatorManager::GetParamsFromReference( "Geometry", pModelManipulator, 0, &szGeometryName, 0 );
				}
			}
		}
	}
	if ( szGeometryName.empty() )
		return true;
	bool bNeedDynamicDebris = false;
	CManipulatorManager::GetValue( &bNeedDynamicDebris, pManipulator, "DynamicDebris.NeedDebris" );
	if ( bNeedDynamicDebris )
	{
		CPtr<IManipulator> pDebrisSet = CManipulatorManager::CreateManipulatorFromReference( "DynamicDebris.Debris", pManipulator, 0, 0, 0 );
		CPtr<IManipulator> pTextureFolderManipulator = pResourceManager->CreateFolderManipulator( "Texture" );
		CPtr<IManipulator> pMaterialFolderManipulator = pResourceManager->CreateFolderManipulator( "Material" );
		if ( pDebrisSet != 0 )
		{
			int nOldCount = 0;
			CManipulatorManager::GetValue( &nOldCount, pManipulator, "DynamicDebris.Masks" );
			list<string> oldMaterials;
			list<string> oldTextures;
			for ( int i = 0; i < nOldCount; ++i )
			{
				string szOldMaterial = "";
				CPtr<IManipulator> pMaterialMan = CManipulatorManager::CreateManipulatorFromReference( StrFmt( "DynamicDebris.Masks.[%d].Material", i ), pManipulator, 0, &szOldMaterial, 0 );
				if ( pMaterialMan != 0 )
				{
					oldMaterials.push_back( szOldMaterial );
					CVariant var;
					pMaterialMan->GetValue( "Bump", &var );
					if ( !IsDBIDEmpty(var) )
						oldTextures.push_back( string( var.GetStr() ) );
				}
			}
			pManipulator->RemoveNode( "DynamicDebris.Masks" );
			for ( list<string>::const_iterator it = oldMaterials.begin(); it != oldMaterials.end(); ++it )					
				pMaterialFolderManipulator->RemoveNode( *it );
			for ( list<string>::const_iterator it = oldTextures.begin(); it != oldTextures.end(); ++it )					
				pTextureFolderManipulator->RemoveNode( *it );
			int nNewCount = 0;
			CManipulatorManager::GetValue( &nNewCount, pDebrisSet, "Debris" );
			typedef hash_map< string, pair<string, float> > CSeasonDebrisMap;
			CSeasonDebrisMap debrisTextures;
			for ( int i = 0; i < nNewCount; ++i )
			{
				CVariant var;
				pDebrisSet->GetValue( StrFmt( "Debris.[%d].Texture", i ), &var );
				if ( !IsDBIDEmpty(var) )
				{
					string szSeason;
					CManipulatorManager::GetValue( &szSeason, pDebrisSet, StrFmt( "Debris.[%d].Season", i ) );
					debrisTextures[szSeason].first = string( var.GetStr() );
					float fWidth;
					CManipulatorManager::GetValue( &fWidth, pDebrisSet, StrFmt( "Debris.[%d].Width", i ) );
					debrisTextures[szSeason].second = fWidth;
				}
			}
			int nEntryCounter = 0;
			for ( CSeasonDebrisMap::const_iterator it = debrisTextures.begin(); it != debrisTextures.end(); ++it, ++nEntryCounter )
			{
				CVec2 vDynamicDebrisOrigin( 0, 0 );
				string szDynamicDebrisTextureFileName = StrFmt( "%sDynDebris_%d.tga", szObjectPath.c_str(), nEntryCounter );
				CDBPtr<NDb::SGeometry> pGeometry = NDb::Get<NDb::SGeometry>( CDBID( szGeometryName ) );
				const string szGeometryFileName = NBinResources::GetExistentBinaryFileName( szGeometriesFolder, pGeometry->GetRecordID(), pGeometry->uid ); // uid
				CreateObjectDynamicDebris( szGeometryFileName, pUserData->constUserData.szExportSourceFolder + szDynamicDebrisTextureFileName, &vDynamicDebrisOrigin, it->second.second );
				const string szMaterialName = StrFmt( "%sDynDebris%d_Material.xdb", szObjectPath.c_str(), nEntryCounter );
				const string szTextureName = StrFmt( "%sDynDebris%d_Texture.xdb", szObjectPath.c_str(), nEntryCounter );
				pTextureFolderManipulator->InsertNode( szTextureName );
				CPtr<IManipulator> pTextureMan = pResourceManager->CreateObjectManipulator( "Texture", szTextureName );
				CManipulatorManager::SetValue( szDynamicDebrisTextureFileName, pTextureMan, "SrcName", false );
				CManipulatorManager::SetValue( "TF_DXT3", pTextureMan, "Format", false );
				CManipulatorManager::SetValue( "WRAP", pTextureMan, "AddrType", false );
				CManipulatorManager::SetValue( 4, pTextureMan, "NMips" );
				pMaterialFolderManipulator->InsertNode( szMaterialName );
				CPtr<IManipulator> pMaterialMan = pResourceManager->CreateObjectManipulator( "Material", szMaterialName );
				CManipulatorManager::SetValue( it->second.first, pMaterialMan, "Texture", true );
				CManipulatorManager::SetValue( szTextureName, pMaterialMan, "Bump", true );
				CManipulatorManager::SetValue( "AM_OVERLAY", pMaterialMan, "AlphaMode", false );
				CManipulatorManager::SetValue( false, pMaterialMan, "CastShadow" );
				CManipulatorManager::SetValue( 9, pMaterialMan, "Priority" );
				pManipulator->InsertNode( "DynamicDebris.Masks", 0 );
				CManipulatorManager::SetValue( it->first, pManipulator, "DynamicDebris.Masks.[0].Season" );
				CManipulatorManager::SetValue( szMaterialName, pManipulator, "DynamicDebris.Masks.[0].Material", true );
				CManipulatorManager::SetVec2( vDynamicDebrisOrigin, pManipulator, "DynamicDebris.Masks.[0].Origin" );
				CManipulatorManager::SetValue( it->second.second, pManipulator, "DynamicDebris.Masks.[0].Width" );
				if ( CPtr<IManipulator> pTextureManipulator = pResourceManager->CreateObjectManipulator( "Texture", it->second.first ) )
				{
					Singleton<IExporterContainer>()->ExportObject( pTextureManipulator, "Texture", it->second.first, true, false );
				}
				if ( pTextureMan )
				{
					Singleton<IExporterContainer>()->ExportObject( pTextureMan, "Texture", szTextureName, true, false  );
				}
			}
		}
	}
	return true;
}

EXPORT_RESULT CObjectBaseRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																												const string &rszObjectTypeName,
																												const string &rszObjectName,
																												bool bForce,
																												EXPORT_TYPE exportType )
{
	CStaticObjectRPGStatsExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	//
	if ( exportType == ET_BEFORE_REF )
		return ER_SUCCESS;
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	//
	bool bResult = true;
	// Получаем манипулятор на VisObject
	CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pManipulator, 0, 0, 0 );
	if ( !pVisObjectManipulator )
	{
		return ER_SUCCESS;
	}
	// Получаем манипулятор модель сезона по умолчанию ( летнюю )
	CPtr<IManipulator> pModelManipulator = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 );
	if ( pModelManipulator == 0 )
	{
		return ER_SUCCESS;
	}
	// Получаем ID Геометрии, чтобы вычислить путь до файла c бинарными данными
	string szGeometryName;
	string szGeometryTypeName;
	CManipulatorManager::GetParamsFromReference( "Geometry", pModelManipulator, &szGeometryTypeName, &szGeometryName, 0 );
	//
	const string szGeometriesFolder =	Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Geometries\\";
	CDBPtr<NDb::SGeometry> pGeometry = NDb::Get<NDb::SGeometry>( CDBID( szGeometryName ) );
	string szDestination = NBinResources::GetBinaryFileName( szGeometriesFolder, pGeometry->GetRecordID(), pGeometry->uid ); // uid
	// Проверяем файл на открываемость
	try
	{
		if ( !WaitForFile( szDestination, 10000 ) )
			szDestination = szGeometriesFolder + StrFmt( "%d", pGeometry->GetRecordID() );

		CGrannyFileInfoGuard fileInfo( szDestination );
		// Записываем высоту объекта (из модели)
		SetObjectHeight( pManipulator, fileInfo ); 
	}
	catch ( ... ) 
	{
		return ER_SUCCESS;
	}
	// Получаем каталоги материалов, текстур и файлов текстур
	string szMaterialFolder;
	string szTextureFolder;
	string szTextureFileFolder;
	int nMaterialCount = 0;
	CManipulatorManager::GetValue( &nMaterialCount, pModelManipulator, "Materials" );	
	if ( nMaterialCount > 0 )
	{
		string szMaterialName;
		if ( CPtr<IManipulator> pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Materials.[0]", pModelManipulator, 0, &szMaterialName, 0 ) )
		{
			string szTextureName;
			if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, &szTextureName, 0 ) )
			{
				CManipulatorManager::GetValue( &szTextureFileFolder, pTextureManipulator, "SrcName" );
				if ( !szTextureFileFolder.empty() )
				{
					CStringManager::CutFileExtention( &szTextureFileFolder );
					szTextureFileFolder += string( "_" );
				}
				//
				szTextureFolder = szTextureName;
				szTextureFolder += string( "_" );
			}
			szMaterialFolder = szMaterialName;
			szMaterialFolder += string( "_" );
		}
	}
	// Записываем прервый параметр - статический модификатор земли под статическим объектом
	// Опрашиваем и, если надо, устанавливаем имя файла для статической текстуры:
	{
		bool bNeedStaticDebris = false;
		CManipulatorManager::GetValue( &bNeedStaticDebris, pManipulator, "StaticDebris.NeedDebris" );
		if ( bNeedStaticDebris )
		{
			// Получаем манипулятор на описатель
			CPtr<IManipulator> pDebrisManipulator = CManipulatorManager::CreateManipulatorFromReference( "StaticDebris.Debris", pManipulator, 0, 0, 0 ); 
			if ( pDebrisManipulator )
			{
				int nDebrisCount = 0;
				bResult = bResult && CManipulatorManager::GetValue( &nDebrisCount, pDebrisManipulator, "Debris" );
				bResult = bResult && CManipulatorManager::EnsureArraySize( nDebrisCount, pManipulator, "StaticDebris.Masks" );

				// пробегаем по списку установленных 
				for ( int nDebrisIndex = 0; nDebrisIndex < nDebrisCount; ++nDebrisIndex )
				{
					const string szDebrisPrefix = StrFmt( "Debris.[%d]", nDebrisIndex );
					const string szMaskPrefix = StrFmt( "StaticDebris.Masks.[%d]", nDebrisIndex );

					string szSeasonName;
					float fWidth = 20;
					bResult = bResult && CManipulatorManager::GetValue( &szSeasonName, pDebrisManipulator, szDebrisPrefix + LEVEL_SEPARATOR_CHAR + "Season" );
					bResult = bResult && CManipulatorManager::GetValue( &fWidth, pDebrisManipulator, szDebrisPrefix + LEVEL_SEPARATOR_CHAR + "Width" );
					//
					CVec2 vMaskOrigin = VNULL2;
					const string szSeasonFilePostfix = typeSeasonFilePostfixMnemonics.GetMnemonic( typeSeasonMnemonics.GetValue( szSeasonName ) );
					const string szMaskFileName = szSeasonFilePostfix.empty() ? StrFmt( "%sStaticDebris.tga", szTextureFileFolder.c_str() ) : StrFmt( "%sStaticDebris_%s.tga", szTextureFileFolder.c_str(), szSeasonFilePostfix.c_str() );
					bResult = bResult && CreateObjectStaticDebris( szDestination, pUserData->constUserData.szExportSourceFolder + szMaskFileName, &vMaskOrigin, (int)fWidth );
					//
					bResult = bResult && CManipulatorManager::SetValue( szSeasonName, pManipulator, szMaskPrefix + LEVEL_SEPARATOR_CHAR + "Season" );
					bResult = bResult && CManipulatorManager::SetValue( szMaskFileName, pManipulator, szMaskPrefix + LEVEL_SEPARATOR_CHAR + "SrcName" );
					bResult = bResult && CManipulatorManager::SetVec2( vMaskOrigin, pManipulator, szMaskPrefix + LEVEL_SEPARATOR_CHAR + "Origin" );
					if ( !bResult )
					{
						break;
					}
				}
			}
		}
		else
		{
			// Удаляем все файлы с масками
			int nMasksCount = 0;
			bResult = bResult && CManipulatorManager::GetValue( &nMasksCount, pManipulator, "StaticDebris.Masks" );
			for ( int nMasksIndex = 0; nMasksIndex < nMasksCount; ++nMasksIndex )
			{
				const string szMaskPrefix = StrFmt( "StaticDebris.Masks.[%d]", nMasksIndex );
				//
				string szMaskFileName;
				if ( CManipulatorManager::GetValue( &szMaskFileName, pManipulator, szMaskPrefix + LEVEL_SEPARATOR_CHAR + "SrcName" ) )
				{
					if ( !szMaskFileName.empty() && ( szMaskFileName[0] != ' ' ) )
					{
						::DeleteFile( ( pUserData->constUserData.szExportSourceFolder + szMaskFileName ).c_str() );
					}					
				}
			}
			//
			pManipulator->RemoveNode( "StaticDebris.Masks", NODE_REMOVEALL_INDEX );
		}
	}
	// Записываем третий параметр - AI проходимость объекта
	bResult = bResult && ExportDynamicDebris( pManipulator, rszObjectName ); 
	if ( bResult && NeedCreatePassability() )
	{
		bool bHasPassability = false;
		CManipulatorManager::GetValue( &bHasPassability, pManipulator, "HasPassability" );
		// Удаляем старый массив
		bResult = bResult && CManipulatorManager::Remove2DArray( pManipulator, "passability" );
		//
		CVec2 vPassabilityOrigin = VNULL2;
		if ( bHasPassability )
		{
			CArray2D<BYTE> passabilityArray;
			bResult = bResult && CreateObjectPassability( szDestination, &passabilityArray, &vPassabilityOrigin );
			// Добавляем новую информацию
			bResult = bResult && CManipulatorManager::Set2DArray( passabilityArray, pManipulator, "passability" );

			NDb::SPassProfile passProfile;
			bResult = bResult && CreateObjectPassabilityProfile( szDestination, 1.0f, &passProfile );
			if ( bResult )
				SavePassProfile( passProfile, "", "PassProfile", pManipulator );
		}
		bResult = bResult && pManipulator->SetValue( "Origin.x", vPassabilityOrigin.x );
		bResult = bResult && pManipulator->SetValue( "Origin.y", vPassabilityOrigin.y );
	}
	return ER_SUCCESS;
}


// basement storage  


