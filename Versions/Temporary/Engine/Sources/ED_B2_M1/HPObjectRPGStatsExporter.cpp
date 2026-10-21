#include "stdafx.h"

#include "ExporterMethods.h"
#include "HPObjectRPGStatsExporter.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "System/FileUtils.h"
#include "TraceModel.h"

#include <algorithm>

#include <fmt/format.h>

static bool SaveSurfacePointsToDB( IManipulator *pManipulator, const std::vector<SModelSurfacePoint> &rPoints )
{
	if ( !pManipulator )
	{
		return false;
	}
	//
	if ( !pManipulator->RemoveNode( "SurfacePoints" ) )
	{
		return false;
	}
	//
	for ( int i = 0; i < rPoints.size(); ++i )
	{
		if ( !pManipulator->InsertNode( "SurfacePoints" ) )
		{
			return false;
		}
		//
		std::string szDBAPos = fmt::format( "SurfacePoints.[{}].Pos", i );
		std::string szDBAOrient = fmt::format( "SurfacePoints.[{}].Orient", i );
		//
		CVec3 pos = rPoints[i].vPos;
		CVec3 n = rPoints[i].vNormal;

		Normalize( &n );
		CVec3 ax = V3_AXIS_Z ^ n;
		float fA = 0;
		float fa = fabs( ax );
		if ( fa > 1.0f )
		{
			fa = 1.0f;
		}
		if ( fa > 0 )
		{
			fA = asin( fa );
		}
		else
		{
			ax = V3_AXIS_Z;
		}

		CQuat q( fA, ax, true );

		if ( !CManipulatorManager::SetVec3( pos, pManipulator, szDBAPos ) )
		{
			return false;
		}
		if ( !CManipulatorManager::SetVec4( q.GetInternalVector(), pManipulator, szDBAOrient ) )
		{
			return false;
		}
		//
	}
	//
	return true;
}

static bool CreateSingleSurfacePoints( IManipulator *pMan )
{
	bool bNeedSurfacePoints = true;
	CManipulatorManager::GetValue( &bNeedSurfacePoints, pMan, "NeedSurfacePoints" );
	if ( bNeedSurfacePoints == false )
	{
		return true;
	}
	// Получаем манипулятор на VisObject
	CPtr<IManipulator> pVisObjMan = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
	if ( pVisObjMan )
	{
		CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
		if ( pModelMan != 0 )
		{
			std::string szGeometryName;
			CPtr<IManipulator> pGeomMan =
					CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, &szGeometryName, 0 );
			std::vector<SModelSurfacePoint> surfacePoints;
			bool bRes = TraceModel( &surfacePoints, szGeometryName );
			if ( bRes )
			{
				bool bRes = SaveSurfacePointsToDB( pMan, surfacePoints );
				NI_ASSERT( bRes, "CreateSurfacePoints(): SaveSurfacePointsToDB() failed" );
			}
		}
	}
	return true;
}

struct STempLightInfo
{
	std::string szLocatorName;
	CVec3 vEffectPos;
	CVec4 vEffectRot;
	CVec3 vFlarePos;
	CVec3 vPointLightPos;
	float fFlareSize;
	float fPointLightSize;
	float fConeLength;
	float fConeSize;
	bool bAtDay;
	bool bAtNight;
	CVec3 vPointLightColour;
	bool bPointLightColourDataPresent;
};

static void WriteLightEffect( IManipulator *pMan, const STempLightInfo &light, int nIndex, bool bNew )
{
	const std::string prefix = fmt::format( "LightEffects.[{}].", nIndex );
	pMan->SetValue( prefix + "LocatorName", light.szLocatorName );
	CManipulatorManager::SetVec3( light.vEffectPos, pMan, prefix + "Pos" );
	CManipulatorManager::SetVec4( light.vEffectRot, pMan, prefix + "Rot" );
	CManipulatorManager::SetVec3( light.vFlarePos, pMan, prefix + "FlarePos" );
	pMan->SetValue( prefix + "FlareSize", light.fFlareSize );
	CManipulatorManager::SetVec3( light.vPointLightPos, pMan, prefix + "PointLightPos" );
	pMan->SetValue( prefix + "PointLightSize", light.fPointLightSize );
	pMan->SetValue( prefix + "ConeSize", light.fConeSize );
	pMan->SetValue( prefix + "ConeLength", light.fConeLength );
	pMan->SetValue( prefix + "OnAtDay", light.bAtDay );
	pMan->SetValue( prefix + "OnAtNight", light.bAtNight );

	// Keep an existing colour when RGB was omitted; new lights default to white.
	// Texture and AdditionalEffect references are authored in the editor and stay untouched.
	if ( bNew || light.bPointLightColourDataPresent )
	{
		// Colour is a VecColor in the database, not a packed pixel value.
		CManipulatorManager::SetVec3( light.vPointLightColour, pMan, prefix + "Colour" );
	}
}

void CHPObjectRPGStatsExporter::ExportSingleLightFX( IManipulator *pMan )
{
	CVec3 vEffectPos;
	CQuat qEffectRot;

	std::list<STempLightInfo> lights;
	std::list<int> toDelete;

	int nEffectIndex = 0;
	//
	//
	CPtr<IManipulator> pVisObjMan = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
	if ( pVisObjMan == 0 )
	{
		return;
	}
	CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
	if ( pModelMan == 0 )
	{
		return;
	}
	CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, 0, 0 );
	if ( pGeomMan == 0 )
	{
		return;
	}
	// Get all attributes from model
	CGrannyBoneAttributesList attribs;
	if ( GetGeometryAttributes( pGeomMan, &attribs ) )
	{
		// Zero is the answer when the record has no LightEffects node at all,
		// which is the case the "add all" branch below exists for:
		// CManipulatorManager::GetValue leaves its output untouched and returns
		// false when the lookup fails, and this call ignores the result.
		int nExistingLights = 0;
		CManipulatorManager::GetValue( &nExistingLights, pMan, "LightEffects" );

		// 1. Collect all information from granny file
		for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it )
		{
			if ( !PatMat( it->szBoneName.c_str(), "leffect??" ) )
			{
				continue;
			}

			STempLightInfo tmpLight = {}; // Optional GLTF extras must have predictable defaults.
			// Match the database default: a light works at night without an extra flag.
			tmpLight.bAtNight = true;

			tmpLight.szLocatorName = it->szRealName;

			it->GetAttribute( "translatex", &tmpLight.vEffectPos.x );
			it->GetAttribute( "translatey", &tmpLight.vEffectPos.y );
			it->GetAttribute( "translatez", &tmpLight.vEffectPos.z );

			vEffectPos = VNULL3;
			it->GetAttribute( "rotatex", &vEffectPos.x );
			it->GetAttribute( "rotatey", &vEffectPos.y );
			it->GetAttribute( "rotatez", &vEffectPos.z );

			qEffectRot.FromEulerAngles( vEffectPos.z, vEffectPos.y, vEffectPos.x );
			tmpLight.vEffectRot = qEffectRot.GetInternalVector();

			it->GetAttribute( "flarex", &tmpLight.vFlarePos.x );
			it->GetAttribute( "flarey", &tmpLight.vFlarePos.y );
			it->GetAttribute( "flarez", &tmpLight.vFlarePos.z );
			it->GetAttribute( "flaresize", &tmpLight.fFlareSize );
			it->GetAttribute( "pointlightx", &tmpLight.vPointLightPos.x );
			it->GetAttribute( "pointlighty", &tmpLight.vPointLightPos.y );
			it->GetAttribute( "pointlightz", &tmpLight.vPointLightPos.z );
			it->GetAttribute( "pointlightintensity", &tmpLight.fPointLightSize );
			it->GetAttribute( "pointlightsize", &tmpLight.fPointLightSize );
			it->GetAttribute( "conebasisscale", &tmpLight.fConeSize );
			it->GetAttribute( "conesize", &tmpLight.fConeSize );
			it->GetAttribute( "conelengthscale", &tmpLight.fConeLength );
			it->GetAttribute( "atday", &tmpLight.bAtDay );
			it->GetAttribute( "atnight", &tmpLight.bAtNight );
			it->GetAttribute( "onatnight", &tmpLight.bAtNight );
			// Direct property names take precedence over legacy aliases, including zero/false.
			// Sizes alone enable components; needflare and needcone are no longer used.

			// Missing channels preserve an existing colour; negative channels are clamped below.
			tmpLight.bPointLightColourDataPresent = it->GetAttribute( "pointlightr", &tmpLight.vPointLightColour.x ) &&
			                                        it->GetAttribute( "pointlightg", &tmpLight.vPointLightColour.y ) &&
			                                        it->GetAttribute( "pointlightb", &tmpLight.vPointLightColour.z );
			if ( !tmpLight.bPointLightColourDataPresent )
			{
				tmpLight.vPointLightColour = CVec3( 1.0f, 1.0f, 1.0f );
			}

			// Normalize colour
			float fMaxValue = 0.0f;
			tmpLight.vPointLightColour.x = (std::max)( tmpLight.vPointLightColour.x, 0.0f );
			tmpLight.vPointLightColour.y = (std::max)( tmpLight.vPointLightColour.y, 0.0f );
			tmpLight.vPointLightColour.z = (std::max)( tmpLight.vPointLightColour.z, 0.0f );
			fMaxValue = (std::max)( fMaxValue, tmpLight.vPointLightColour.x );
			fMaxValue = (std::max)( fMaxValue, tmpLight.vPointLightColour.y );
			fMaxValue = (std::max)( fMaxValue, tmpLight.vPointLightColour.z );
			// VecColor uses 0..1 channels. Black stays black without dividing by zero.
			if ( fMaxValue > 0.0f )
			{
				tmpLight.vPointLightColour.x /= fMaxValue;
				tmpLight.vPointLightColour.y /= fMaxValue;
				tmpLight.vPointLightColour.z /= fMaxValue;
			}

			lights.push_back( tmpLight );
		}

		// Update surviving records in place so editor-only fields and ordering survive.
		// Consume matched locators; the remainder must be appended even to a populated list.
		for ( int i = 0; i < nExistingLights; ++i )
		{
			std::string locator;
			CManipulatorManager::GetValue( &locator, pMan, fmt::format( "LightEffects.[{}].LocatorName", i ) );
			const auto light =
					std::find_if( lights.begin(),
			                  lights.end(),
			                  [&locator]( const STempLightInfo &entry ) { return entry.szLocatorName == locator; } );
			if ( light == lights.end() )
			{
				toDelete.push_front( i );
				continue;
			}
			WriteLightEffect( pMan, *light, i, false );
			lights.erase( light );
		}

		// Remove in reverse order before choosing the indices for new lights.
		for ( int index : toDelete )
		{
			pMan->RemoveNode( "LightEffects", index );
		}
		CManipulatorManager::GetValue( &nEffectIndex, pMan, "LightEffects" );
		for ( const STempLightInfo &light : lights )
		{
			if ( pMan->InsertNode( "LightEffects" ) )
			{
				WriteLightEffect( pMan, light, nEffectIndex, true );
				++nEffectIndex;
			}
		}
	}
}

void CHPObjectRPGStatsExporter::CreateSingleIcons( IManipulator *pMan,
                                                   const std::string &szObjectTypeName,
                                                   const std::string &szObjectName )
{
	const std::string szIconTexturePrefix = "All\\UI\\Mission\\Icons\\";
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pTextureFolderMan = pResourceManager->CreateFolderManipulator( "Texture" );
	if ( !pTextureFolderMan )
	{
		return;
	}
	//
	std::string szIconTextureName;
	CManipulatorManager::GetValue( &szIconTextureName, pMan, "IconTexture" );
	if ( szIconTextureName.empty() )
	{
		std::string szTextureFolder;
		// получим каталог с тектурами
		CPtr<IManipulator> pVisObjMan =
				CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
		if ( pVisObjMan == 0 )
		{
			return;
		}
		CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
		if ( pModelMan == 0 )
		{
			return;
		}
		CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, 0, 0 );
		if ( pGeomMan == 0 )
		{
			return;
		}
		// GLTF ModelFileRef can live elsewhere; icon.tga stays beside the authored textures.
		CPtr<IManipulator> material =
				CManipulatorManager::CreateManipulatorFromReference( "Materials.[0]", pModelMan, 0, 0, 0 );
		CPtr<IManipulator> texture =
				material ? CManipulatorManager::CreateManipulatorFromReference( "Texture", material, 0, 0, 0 ) : nullptr;
		if ( texture )
		{
			CManipulatorManager::GetValue( &szTextureFolder, texture, "SrcName" );
		}
		if ( szTextureFolder.empty() )
		{
			CManipulatorManager::GetValue( &szTextureFolder, pGeomMan, "SrcName" );
		}
		if ( szTextureFolder.empty() )
		{
			return;
		}
		const std::string szIconTextureFileName = NFile::JoinPath( NFile::GetFilePath( szTextureFolder ), "icon.tga" );
		if ( NFile::DoesFileExist( NFile::JoinPath(
						 Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder, szIconTextureFileName ) ) )
		{
			szIconTextureName = szIconTexturePrefix + fmt::format( "{}\\{}", szObjectTypeName.c_str(), szObjectName.c_str() );
			pFolderCallback->InsertObject( "Texture", szIconTextureName );
			CPtr<IManipulator> pIconTextureMan = pResourceManager->CreateObjectManipulator( "Texture", szIconTextureName );
			if ( pIconTextureMan )
			{
				CManipulatorManager::SetValue( szIconTextureFileName, pIconTextureMan, "SrcName" );
				CManipulatorManager::SetValue( std::string( "TEXTURE_2D" ), pIconTextureMan, "Type" );
				CManipulatorManager::SetValue( std::string( "CONVERT_ORDINARY" ), pIconTextureMan, "ConversionType" );
				CManipulatorManager::SetValue( std::string( "CLAMP" ), pIconTextureMan, "AddrType" );
				CManipulatorManager::SetValue( std::string( "TF_8888" ), pIconTextureMan, "Format" );
				CManipulatorManager::SetValue( 1, pIconTextureMan, "NMips" );
			}
			//
			Singleton<IExporterContainer>()->ExportObject(
					pIconTextureMan, "Texture", szIconTextureName, FORCE_EXPORT, false );
			//
			CManipulatorManager::SetValue( szIconTextureName, pMan, "IconTexture" );
		}
		else
		{
			NLog::Log( LT_IMPORTANT, "Icon creation failed. Can't find icon.tga file.\n" );
			NLog::Log( LT_IMPORTANT, "\tObject type: %s\n", szObjectTypeName.c_str() );
			NLog::Log( LT_IMPORTANT, "\tObject name: %s\n", szObjectName.c_str() );
			NLog::Log( LT_IMPORTANT, "\tIcon path: %s\n", szIconTextureFileName.c_str() );
			return;
		}
	}
}

EXPORT_RESULT CHPObjectRPGStatsExporter::ExportObject( IManipulator *pManipulator,
                                                       const std::string &rszObjectTypeName,
                                                       const std::string &rszObjectName,
                                                       bool bForce,
                                                       EXPORT_TYPE exportType )
{
	EXPORT_RESULT result =
			CBasicExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
	if ( result != ER_SUCCESS )
	{
		return result;
	}
	if ( exportType == ET_BEFORE_REF )
	{
		return ER_SUCCESS;
	}
	if ( result != ER_FAIL )
	{
		//
		ExportSingleLightFX( pManipulator );
		if ( CreateSingleSurfacePoints( pManipulator ) == false )
		{
			NLog::Log( LT_IMPORTANT, "Create surface points failed\n" );
			NLog::Log( LT_IMPORTANT, "\tObject type: %s\n", rszObjectTypeName.c_str() );
			NLog::Log( LT_IMPORTANT, "\tObject name: %s\n", rszObjectName.c_str() );
		}
		CreateSingleIcons( pManipulator, rszObjectTypeName, rszObjectName );
	}
	return result;
}
