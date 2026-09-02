#include "stdafx.h"
#include <fmt/format.h>

#include "HPObjectRPGStatsExporter.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "libdb/ResourceManager.h"
#include "ExporterMethods.h"
#include "TraceModel.h"
#include "System/FileUtils.h"
#include "3Dmotor/GPixelFormat.h"

static bool SaveSurfacePointsToDB( IManipulator *pManipulator, const std::vector<SModelSurfacePoint> &rPoints )
{
	if ( !pManipulator )
		return false;
	//
		if ( !pManipulator->RemoveNode( "SurfacePoints" ) )
			return false;
	//
	for ( int i = 0; i < rPoints.size(); ++i )
	{
		if ( !pManipulator->InsertNode( "SurfacePoints" ) )
			return false;
		//
		std::string szDBAPos = fmt::format( "SurfacePoints.[{}].Pos", i );
		std::string szDBAOrient = fmt::format( "SurfacePoints.[{}].Orient", i );
		//
		CVec3 pos = rPoints[i].vPos;
		CVec3 n = rPoints[i].vNormal;

		Normalize(&n);
		CVec3 ax = V3_AXIS_Z ^ n;
		float fA = 0;
		float fa = fabs(ax);
		if ( fa > 1.0f )
			fa = 1.0f;
		if ( fa > 0 )
			fA = asin( fa );
		else
			ax = V3_AXIS_Z;			
		
		CQuat q( fA, ax, true );

		if ( !CManipulatorManager::SetVec3( pos, pManipulator, szDBAPos ) )
			return false;
		if ( !CManipulatorManager::SetVec4( q.GetInternalVector(), pManipulator, szDBAOrient ) )
			return false;
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
		return true;
	// Получаем манипулятор на VisObject
	CPtr<IManipulator> pVisObjMan = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
	if ( pVisObjMan )
	{
		CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
		if ( pModelMan != 0 ) 
		{
			std::string szGeometryName;
			CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, &szGeometryName, 0 );
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
	bool bNeedFlare;
	bool bNeedCone;
	bool bAtDay;
	bool bAtNight;
	CVec3 vPointLightColour;
	bool bPointLightColourDataPresent;
};

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
		return;
	CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
	if ( pModelMan == 0 )
		return;
	CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, 0, 0 );
	if ( pGeomMan == 0 )
		return;
	// Get all attributes from model
	CGrannyBoneAttributesList attribs;
	if ( GetGeometryAttributes( pGeomMan, &attribs ) )
	{
		int nExistingLights;
		CManipulatorManager::GetValue( &nExistingLights, pMan, "LightEffects" );

		// 1. Collect all information from granny file
		for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
		{
			if ( !PatMat( it->szBoneName.c_str(), "leffect??" ) )
				continue;

			STempLightInfo tmpLight;

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
			it->GetAttribute( "conebasisscale", &tmpLight.fConeSize );
			it->GetAttribute( "conelengthscale", &tmpLight.fConeLength );
			it->GetAttribute( "needcone", &tmpLight.bNeedCone );
			it->GetAttribute( "needflare", &tmpLight.bNeedFlare );
			it->GetAttribute( "atday", &tmpLight.bAtDay );
			it->GetAttribute( "atnight", &tmpLight.bAtNight );

			if ( !tmpLight.bNeedCone ) 
			{
				tmpLight.fConeSize = 0.0f;
				tmpLight.fConeLength = 0.0f;
			}
			if ( !tmpLight.bNeedFlare ) 
			{
				tmpLight.fFlareSize = 0.0f;
			}

			// Get data
			tmpLight.bPointLightColourDataPresent = true;
			tmpLight.vPointLightColour = CVec3( -100.0f, -100.0f, -100.0f );
			it->GetAttribute( "pointlightr", &tmpLight.vPointLightColour.x );
			it->GetAttribute( "pointlightg", &tmpLight.vPointLightColour.y );
			it->GetAttribute( "pointlightb", &tmpLight.vPointLightColour.z );
			if ( tmpLight.vPointLightColour.x == -100.0f ||
				tmpLight.vPointLightColour.y == -100.0f ||
				tmpLight.vPointLightColour.z == -100.0f )
			{
				tmpLight.bPointLightColourDataPresent = false;
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
			fMaxValue /= 255.0f;
			tmpLight.vPointLightColour.x /= fMaxValue;
			tmpLight.vPointLightColour.y /= fMaxValue;
			tmpLight.vPointLightColour.z /= fMaxValue;

			lights.push_back( tmpLight );
		}

		// 2. If DB record has no LightEffects elements, add all
		if ( nExistingLights <= 0 )
		{
			nEffectIndex = 0;
			for ( std::list<STempLightInfo>::iterator it = lights.begin(); it != lights.end(); ++it )
			{
				if ( !pMan->InsertNode( "LightEffects" ) ) 
					continue;

				const STempLightInfo &tmpLight = *it;

				const std::string szNodePrefix = fmt::format( "LightEffects.[{}].", nEffectIndex );

				pMan->SetValue( szNodePrefix + "LocatorName", tmpLight.szLocatorName );
				CManipulatorManager::SetVec3( tmpLight.vEffectPos, pMan, szNodePrefix + "Pos" ); 
				CManipulatorManager::SetVec4( tmpLight.vEffectRot, pMan, szNodePrefix + "Rot" ); 
				CManipulatorManager::SetVec3( tmpLight.vFlarePos, pMan, szNodePrefix + "FlarePos" );
				pMan->SetValue( szNodePrefix + "FlareSize", tmpLight.fFlareSize );
				CManipulatorManager::SetVec3( tmpLight.vPointLightPos, pMan, szNodePrefix + "PointLightPos" ); 
				pMan->SetValue( szNodePrefix + "PointLightSize", tmpLight.fPointLightSize );
				pMan->SetValue( szNodePrefix + "ConeSize", tmpLight.fConeSize );
				pMan->SetValue( szNodePrefix + "ConeLength", tmpLight.fConeLength );
				pMan->SetValue( szNodePrefix + "OnAtDay", tmpLight.bAtDay );
				pMan->SetValue( szNodePrefix + "OnAtNight", tmpLight.bAtNight );

				NGfx::SPixel8888 colour;
				colour.r = tmpLight.vPointLightColour.r;
				colour.g = tmpLight.vPointLightColour.g;
				colour.b = tmpLight.vPointLightColour.b;
				colour.a = 255;
				int nColour = colour.dwColor;
				//CManipulatorManager::SetVec3( tmpLight.vPointLightColour, pMan, szNodePrefix + "Colour" ); 
				pMan->SetValue( szNodePrefix + "Colour", nColour );

				++nEffectIndex;
			}
		}
		else	// 3. If DB record has LightEffects, merge
		{
			// 3.1 Update existing records, mark for deletion other items
			for ( int i = 0; i < nExistingLights; ++i )
			{
				const std::string szNodePrefix = fmt::format( "LightEffects.[{}].", i );

				std::string szLocatorName;
				CManipulatorManager::GetValue( &szLocatorName, pMan, szNodePrefix + "LocatorName" );

				// Search the model for this locator
				bool bFound = false;
				std::list<STempLightInfo>::iterator itLight;
				for ( itLight = lights.begin(); itLight != lights.end(); ++itLight )
				{
					if ( (*itLight).szLocatorName == szLocatorName )
					{
						bFound = true;
						break;
					}
				}

				if ( !bFound )		// Not found, mark for deletion
				{
					toDelete.push_front( i );		// push_front to create back numeration
					continue;
				}

				const STempLightInfo &tmpLight = *itLight;

				// Locator found, update data
				CManipulatorManager::SetVec3( tmpLight.vEffectPos, pMan, szNodePrefix + "Pos" ); 
				CManipulatorManager::SetVec4( tmpLight.vEffectRot, pMan, szNodePrefix + "Rot" ); 
				CManipulatorManager::SetVec3( tmpLight.vFlarePos, pMan, szNodePrefix + "FlarePos" );
				pMan->SetValue( szNodePrefix + "FlareSize", tmpLight.fFlareSize );
				CManipulatorManager::SetVec3( tmpLight.vPointLightPos, pMan, szNodePrefix + "PointLightPos" ); 
				pMan->SetValue( szNodePrefix + "PointLightSize", tmpLight.fPointLightSize );
				pMan->SetValue( szNodePrefix + "ConeSize", tmpLight.fConeSize );
				pMan->SetValue( szNodePrefix + "ConeLength", tmpLight.fConeLength );
				pMan->SetValue( szNodePrefix + "OnAtDay", tmpLight.bAtDay );
				pMan->SetValue( szNodePrefix + "OnAtNight", tmpLight.bAtNight );

				// If there is colour information, override
				if ( tmpLight.bPointLightColourDataPresent )
				{
					NGfx::SPixel8888 colour;
					colour.r = tmpLight.vPointLightColour.r;
					colour.g = tmpLight.vPointLightColour.g;
					colour.b = tmpLight.vPointLightColour.b;
					colour.a = 255;
					int nColour = colour.dwColor;
					//CManipulatorManager::SetVec3( tmpLight.vPointLightColour, pMan, szNodePrefix + "Colour" ); 
					pMan->SetValue( szNodePrefix + "Colour", nColour );
				}
			}

			// 3.2 delete marked items
			for ( std::list<int>::iterator it = toDelete.begin(); it != toDelete.end(); ++it )
			{ // the list has the numbers back to front, so nodes are erased correctly
				pMan->RemoveNode( "LightEffects", *it );
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
		return;
	//
	std::string szIconTextureName;
	CManipulatorManager::GetValue( &szIconTextureName, pMan, "IconTexture" );
	if ( szIconTextureName.empty() )
	{
		std::string szTextureFolder;
		// получим каталог с тектурами
		CPtr<IManipulator> pVisObjMan = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
		if ( pVisObjMan == 0 )
			return;
		CPtr<IManipulator> pModelMan = CreateModelManipulatorFromVisObj( pVisObjMan, 0 );
		if ( pModelMan == 0 )
			return;
		CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pModelMan, 0, 0, 0 );
		if ( pGeomMan == 0 )
			return;
		CManipulatorManager::GetValue( &szTextureFolder, pGeomMan, "SrcName" );
		if ( szTextureFolder.empty() )
			return;
		szTextureFolder = szTextureFolder.substr( 0, szTextureFolder.rfind( '\\' ) + 1 );
		const std::string szIconTextureFileName = szTextureFolder + "icon.tga";
		if ( NFile::DoesFileExist( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szIconTextureFileName ) )
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
			Singleton<IExporterContainer>()->ExportObject( pIconTextureMan, "Texture", szIconTextureName, FORCE_EXPORT, false );
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
	EXPORT_RESULT result = CBasicExporter::ExportObject( pManipulator, rszObjectTypeName, rszObjectName, bForce, exportType );
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


