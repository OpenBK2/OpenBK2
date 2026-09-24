#include "stdafx.h"
#include <fmt/format.h>

#include "Misc/StrProc.h"
#include "Misc/2Darray.h"
#include "3Dmotor/DBScene.h"
#include "ObjectBaseRPGStatsExporter.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "MapEditorLib/Interface_MOD.h"

#include "ExporterMethods.h"
#include "ED_Common/GltfExporter.h"
#include "MapEditorLib/MessageBoxes.h"
#include <map>
#include "SeasonMnemonics.h"

#include <cstdint>

#include <zconf.h>

namespace NDb
{
	// CRAP{ legacy - remove it ASAP
	const CDBID *GetDBID( const int nClassTypeID, const int nRecordID );
	// CRAP}
}

//REGISTER_EXPORTER_IN_DLL( BuildingRPGStats, CObjectBaseRPGStatsExporter )


static EXPORT_RESULT ObjectExportError( const std::string &objectName, const std::string &detail )
{
	const std::string message = "Cannot export " + objectName + ":\n" + detail;
	NLog::Log(LT_ERROR, "%s\n", message.c_str());
	NMessage::Error(message, "Object export");
	return ER_BREAK;
}

bool CObjectBaseRPGStatsExporter::ExportDynamicDebris( IManipulator *pManipulator, const std::string &szObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	// Keep generated debris private to this object, even when several stats share a folder.
	const std::string szObjectPath = NFile::CutFileExt(szObjectName, ".xdb") + "_";

	std::string szGeometryName;
	{
		std::string szVisObj = "";
		CManipulatorManager::GetValue( &szVisObj, pManipulator, "visualObject" );
		int nDamageLevels = 0;
		CManipulatorManager::GetValue( &nDamageLevels, pManipulator, "DamageLevels" );
		for ( int i = 0; i < nDamageLevels; ++i )
		{
			float fHP = 0;
			CManipulatorManager::GetValue( &fHP, pManipulator, fmt::format( "DamageLevels.[{}].DamageHP", i ) );
			if ( fHP <= 0 )
			{
				CManipulatorManager::GetValue( &szVisObj, pManipulator, fmt::format( "DamageLevels.[{}].VisObj", i ) );
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
		CPtr<IManipulator> geometry = pResourceManager->CreateObjectManipulator("Geometry", szGeometryName);
		NEditorGltf::SMeshData mesh;
		if ( !geometry || !NEditorGltf::LoadGeometry(geometry, &mesh) ) return false;
		CPtr<IManipulator> pDebrisSet = CManipulatorManager::CreateManipulatorFromReference( "DynamicDebris.Debris", pManipulator, 0, 0, 0 );
		if ( !pDebrisSet ) return false;
		auto *folders = Singleton<IFolderCallback>();
		{
			// Reuse generated XDB resources. Deleting and recreating a loaded resource
			// leaves its cached references invalid on subsequent exports after restart.
			pManipulator->RemoveNode("DynamicDebris.Masks", NODE_REMOVEALL_INDEX);
			int nNewCount = 0;
			CManipulatorManager::GetValue( &nNewCount, pDebrisSet, "Debris" );
			// Stable ordering keeps generated seasonal mask names consistent on every platform.
			typedef std::map< std::string, std::pair<std::string, float> > CSeasonDebrisMap;
			CSeasonDebrisMap debrisTextures;
			for ( int i = 0; i < nNewCount; ++i )
			{
				CVariant var;
				pDebrisSet->GetValue( fmt::format( "Debris.[{}].Texture", i ), &var );
				if ( !IsDBIDEmpty(var) )
				{
					std::string szSeason;
					CManipulatorManager::GetValue( &szSeason, pDebrisSet, fmt::format( "Debris.[{}].Season", i ) );
					debrisTextures[szSeason].first = std::string( var.GetStr() );
					float fWidth = 0;
					CManipulatorManager::GetValue( &fWidth, pDebrisSet, fmt::format( "Debris.[{}].Width", i ) );
					debrisTextures[szSeason].second = fWidth;
				}
			}
			int nEntryCounter = 0;
			for ( CSeasonDebrisMap::const_iterator it = debrisTextures.begin(); it != debrisTextures.end(); ++it, ++nEntryCounter )
			{
				CVec2 vDynamicDebrisOrigin( 0, 0 );
				std::string szDynamicDebrisTextureFileName = fmt::format( "{}DynDebris_{}.tga", szObjectPath.c_str(), nEntryCounter );
				if ( !CreateObjectDynamicDebris(mesh, NFile::JoinPath(pUserData->constUserData.szExportSourceFolder,
					szDynamicDebrisTextureFileName), &vDynamicDebrisOrigin, it->second.second) ) return false;
				const std::string szMaterialName = fmt::format( "{}DynDebris{}_Material.xdb", szObjectPath.c_str(), nEntryCounter );
				const std::string szTextureName = fmt::format( "{}DynDebris{}_Texture.xdb", szObjectPath.c_str(), nEntryCounter );
				if ( folders->IsUniqueName("Texture", szTextureName) && !folders->InsertObject("Texture", szTextureName) ) return false;
				CPtr<IManipulator> pTextureMan = pResourceManager->CreateObjectManipulator( "Texture", szTextureName );
				if ( !pTextureMan ) return false;
				CManipulatorManager::SetValue( szDynamicDebrisTextureFileName, pTextureMan, "SrcName", false );
				CManipulatorManager::SetValue( "TF_DXT3", pTextureMan, "Format", false );
				CManipulatorManager::SetValue( "WRAP", pTextureMan, "AddrType", false );
				CManipulatorManager::SetValue( 4, pTextureMan, "NMips" );
				if ( folders->IsUniqueName("Material", szMaterialName) && !folders->InsertObject("Material", szMaterialName) ) return false;
				CPtr<IManipulator> pMaterialMan = pResourceManager->CreateObjectManipulator( "Material", szMaterialName );
				if ( !pMaterialMan ) return false;
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
					if ( Singleton<IExporterContainer>()->ExportObject( pTextureManipulator, "Texture", it->second.first, true, false ) != ER_SUCCESS ) return false;
				}
				if ( pTextureMan )
				{
					if ( Singleton<IExporterContainer>()->ExportObject( pTextureMan, "Texture", szTextureName, true, false ) != ER_SUCCESS ) return false;
				}
			}
		}
	}
	return true;
}

EXPORT_RESULT CObjectBaseRPGStatsExporter::ExportObject( IManipulator* pManipulator,
																												const std::string &rszObjectTypeName,
																												const std::string &rszObjectName,
																												bool bForce,
																												EXPORT_TYPE exportType )
{
	// Reject legacy sources before any reference exporter can mutate this object.
	if ( exportType != ET_AFTER_REF && !NEditorGltf::ValidateForExport(pManipulator, rszObjectTypeName) ) return ER_BREAK;
	if ( exportType == ET_NO_REF )
	{
		// Direct exports also publish ModelFileRef before surface-point generation reads the model.
		CPtr<IManipulator> visual = CManipulatorManager::CreateManipulatorFromReference("visualObject", pManipulator, 0, 0, 0);
		CPtr<IManipulator> model = visual ? CreateModelManipulatorFromVisObj(visual, 0) : nullptr;
		CPtr<IManipulator> geometry = model ? CManipulatorManager::CreateManipulatorFromReference("Geometry", model, 0, 0, 0) : nullptr;
		if ( geometry && !NEditorGltf::Export(geometry, "Geometry", true) )
			return ObjectExportError(rszObjectName, "Cannot export the GLTF model. Check the source and writable data/mod folder.");
	}
	const auto result = CStaticObjectRPGStatsExporter::ExportObject(pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType);
	if ( result != ER_SUCCESS ) return result;
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
	CPtr<IManipulator> geometry = CManipulatorManager::CreateManipulatorFromReference("Geometry", pModelManipulator, 0, 0, 0);
	NEditorGltf::SMeshData mesh;
	if ( !geometry || !NEditorGltf::LoadGeometry(geometry, &mesh) )
		return ObjectExportError(rszObjectName, "Cannot read GLTF geometry. Check ModelFileRef and RootMesh.");
	// GLTF vertices have already been converted to the engine's Z-up coordinates.
	if ( !pManipulator->SetValue("ObjectHeight", int(Vis2AI(mesh.maximum.z - mesh.minimum.z))) )
		return ObjectExportError(rszObjectName, "Cannot update ObjectHeight.");
	// Получаем каталоги материалов, текстур и файлов текстур
	std::string szMaterialFolder;
	std::string szTextureFolder;
	std::string szTextureFileFolder;
	int nMaterialCount = 0;
	CManipulatorManager::GetValue( &nMaterialCount, pModelManipulator, "Materials" );	
	if ( nMaterialCount > 0 )
	{
		std::string szMaterialName;
		if ( CPtr<IManipulator> pMaterialManipulator = CManipulatorManager::CreateManipulatorFromReference( "Materials.[0]", pModelManipulator, 0, &szMaterialName, 0 ) )
		{
			std::string szTextureName;
			if ( CPtr<IManipulator> pTextureManipulator = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialManipulator, 0, &szTextureName, 0 ) )
			{
				CManipulatorManager::GetValue( &szTextureFileFolder, pTextureManipulator, "SrcName" );
				if ( !szTextureFileFolder.empty() )
				{
					CStringManager::CutFileExtention( &szTextureFileFolder );
					szTextureFileFolder += std::string( "_" );
				}
				//
				szTextureFolder = szTextureName;
				szTextureFolder += std::string( "_" );
			}
			szMaterialFolder = szMaterialName;
			szMaterialFolder += std::string( "_" );
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
			if ( !pDebrisManipulator ) return ObjectExportError(rszObjectName, "Static debris is enabled but its Debris resource is missing.");
			{
				int nDebrisCount = 0;
				bResult = bResult && CManipulatorManager::GetValue( &nDebrisCount, pDebrisManipulator, "Debris" );
				bResult = bResult && CManipulatorManager::EnsureArraySize( nDebrisCount, pManipulator, "StaticDebris.Masks" );

				// пробегаем по списку установленных 
				for ( int nDebrisIndex = 0; nDebrisIndex < nDebrisCount; ++nDebrisIndex )
				{
					const std::string szDebrisPrefix = fmt::format( "Debris.[{}]", nDebrisIndex );
					const std::string szMaskPrefix = fmt::format( "StaticDebris.Masks.[{}]", nDebrisIndex );

					std::string szSeasonName;
					float fWidth = 20;
					bResult = bResult && CManipulatorManager::GetValue( &szSeasonName, pDebrisManipulator, szDebrisPrefix + LEVEL_SEPARATOR_CHAR + "Season" );
					bResult = bResult && CManipulatorManager::GetValue( &fWidth, pDebrisManipulator, szDebrisPrefix + LEVEL_SEPARATOR_CHAR + "Width" );
					//
					CVec2 vMaskOrigin = VNULL2;
					const std::string szSeasonFilePostfix = typeSeasonFilePostfixMnemonics.GetMnemonic( typeSeasonMnemonics.GetValue( szSeasonName ) );
					const std::string szMaskFileName = szSeasonFilePostfix.empty() ? fmt::format( "{}StaticDebris.tga", szTextureFileFolder.c_str() ) : fmt::format( "{}StaticDebris_{}.tga", szTextureFileFolder.c_str(), szSeasonFilePostfix.c_str() );
					bResult = bResult && CreateObjectStaticDebris( mesh, NFile::JoinPath(pUserData->constUserData.szExportSourceFolder, szMaskFileName), &vMaskOrigin, (int)fWidth );
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
				const std::string szMaskPrefix = fmt::format( "StaticDebris.Masks.[{}]", nMasksIndex );
				//
				std::string szMaskFileName;
				if ( CManipulatorManager::GetValue( &szMaskFileName, pManipulator, szMaskPrefix + LEVEL_SEPARATOR_CHAR + "SrcName" ) )
				{
					if ( !szMaskFileName.empty() && ( szMaskFileName[0] != ' ' ) )
					{
						NFile::RemoveFile( ( pUserData->constUserData.szExportSourceFolder + szMaskFileName ).c_str() );
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
		NDb::SPassProfile passProfile;
		if ( bHasPassability )
		{
			CArray2D<uint8_t> passabilityArray;
			bResult = bResult && CreateObjectPassability( mesh, &passabilityArray, &vPassabilityOrigin );
			// Добавляем новую информацию
			bResult = bResult && CManipulatorManager::Set2DArray( passabilityArray, pManipulator, "passability" );

			bResult = bResult && CreateObjectPassabilityProfile( mesh, 1.0f, &passProfile );
		}
		if ( bResult ) SavePassProfile(passProfile, "", "PassProfile", pManipulator);
		bResult = bResult && pManipulator->SetValue( "Origin.x", vPassabilityOrigin.x );
		bResult = bResult && pManipulator->SetValue( "Origin.y", vPassabilityOrigin.y );
	}
	if ( !bResult )
		return ObjectExportError(rszObjectName, "Could not generate debris or passability. Check the log and ensure the export source folder is writable.");
	return ER_SUCCESS;
}


// basement storage  


