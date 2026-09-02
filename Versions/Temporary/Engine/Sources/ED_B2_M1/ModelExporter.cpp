#include "stdafx.h"

#include "ModelExporter.h"
#include "ExporterMethods.h"
#include "MapEditorLib/ExporterFactory.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ED_Common/TempAttributesTool.h"
#include "libdb/ResourceManager.h"
#include "Misc/StrProc.h"
#include "System/FilePath.h"

namespace NModelExporter
{

EXPORT_RESULT CModelExporter::ExportObject( IManipulator* pManipulator,
																						const string &rszObjectTypeName,
																						const string &rszObjectName,
																						bool bForce,
																						EXPORT_TYPE exportType )
{
	if ( exportType == ET_BEFORE_REF ) 
		return ER_SUCCESS;
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	try
	{
		// first, check number of meshes and number of materials
		string szGeometryName;
		CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pManipulator, 0, &szGeometryName, 0 );
		if ( pGeomMan == 0 || szGeometryName.empty() ) 
		{
			Log( LT_ERROR, "Can't extract geometry from model\n" );
			Log( LT_ERROR, StrFmt("\tModel name: %s\n", rszObjectName.c_str()) );
			return ER_FAIL;
		}
		int nNumGeometryMeshes = 0;
		CManipulatorManager::GetValue( &nNumGeometryMeshes, pGeomMan, "NumMeshes" );
		if ( nNumGeometryMeshes == 0 ) 
		{
			Log( LT_ERROR, "Empty 'NumMeshes' in geometry\n" );
			Log( LT_ERROR, StrFmt("\tModel name: %s\n", rszObjectName.c_str()) );
			return ER_FAIL;
		}
		int nNumGeomMaterials = 0;
		CManipulatorManager::GetValue( &nNumGeomMaterials, pManipulator, "Materials" );

		string szModelName = "";
		CManipulatorManager::GetValue( &szModelName, pGeomMan, "RootMesh" );

		if ( nNumGeomMaterials == 0 && PatMat( szModelName.c_str(), "*section??" ) == 0 ) 
		{
			Log( LT_ERROR, "Empty material list in model\n" );
			Log( LT_ERROR, StrFmt("\tModel name: %s\n", rszObjectName.c_str()) );
			return ER_FAIL;
		}
		// load geometry
		CDBPtr<NDb::SGeometry> pGeometry = NDb::Get<NDb::SGeometry>( CDBID( szGeometryName ) );
		CGrannyFileInfoGuard pInfo( NBinResources::GetExistentBinaryFileName( Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\geometries", pGeometry->GetRecordID(), pGeometry->uid ) ); // uid
		//CGrannyFileInfoGuard pInfo( pUserData->szExportDestinationFolder + StrFmt( "bin\\geometries\\%d", nGeomID ) );
		int nNumGrannyMeshes = pInfo->MeshCount;
		//
		if ( nNumGrannyMeshes != nNumGeometryMeshes ) 
		{
		}
		//
		if ( nNumGeomMaterials > 0 )
			MakeMaterialsList( pManipulator, pGeomMan, pInfo );
	}
	catch ( ... ) 
	{
		Log( LT_ERROR, "General fail during model check\n" );
		Log( LT_ERROR, StrFmt("\tModel name: %s\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}

bool MakeMaterialCopy( const string &szSrcName, const string &szDstName )
{
	return Singleton<IFolderCallback>()->CopyObject( "Material", szDstName, szSrcName );
}

struct SMaterialInfo
{
	string szName;
	string szType;
	//
	SMaterialInfo() {}
	SMaterialInfo( const string &_szName, const string &_szType )
		: szName( _szName ), szType( _szType ) {}
};

bool CModelExporter::MakeMaterialsList( IManipulator* pModelMan, IManipulator* pGeomMan, CGrannyFileInfoGuard &pInfo )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	CGrannyBoneAttributesList attribs;
	ReadAttributes( &attribs, NMEGeomAttribs::GetAttribsByModel(pModelMan), "", true );
	// find opacity attribute for each mesh
	bool bHasTransparency = false;
	std::unordered_map<string, bool> meshTranspInfoMap;
	for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
	{
		SGrannyBoneAttributes::CAttributeMap::const_iterator posAttribute = it->attributeMap.find( "transparent" );
		bool bTransparent = posAttribute != it->attributeMap.end() ? posAttribute->second : false;
		meshTranspInfoMap[it->szBoneName] = bTransparent;
		bHasTransparency = bHasTransparency || bTransparent;
	}
	// check, do we need transparent material and leave if needn't
	if ( !bHasTransparency ) 
		return true;
	// collect all materials from model
	string szTransparentMaterialName, szOpaqueMaterialName;
	int nNumModelMaterials = 0;
	CManipulatorManager::GetValue( &nNumModelMaterials, pModelMan, "Materials" );
	list<SMaterialInfo> materials;
	for ( int i = 0; i < nNumModelMaterials; ++i ) 
	{
		const string szMaterialRefName = StrFmt( "Materials.[%d]", i );
		string szMaterialName;
		CManipulatorManager::GetValue( &szMaterialName, pModelMan, szMaterialRefName );
		if ( szMaterialName.empty() || szMaterialName == " " )
		{
			Log( LT_ERROR, "Empty material found\n" );
			return false;
		}
		//
		CPtr<IManipulator> pMaterialMan = CManipulatorManager::CreateManipulatorFromReference( szMaterialRefName, pModelMan, 0, 0, 0 );
		string szMaterialType;
		CManipulatorManager::GetValue( &szMaterialType, pMaterialMan, "AlphaMode" );
		//
		if ( szMaterialType == "AM_TRANSPARENT" ) 
			szTransparentMaterialName = szMaterialName;
		else
			szOpaqueMaterialName = szMaterialName;
		materials.push_back( SMaterialInfo(szMaterialName, szMaterialType) );
	}
	// make transparent material and change texture params
	if ( szTransparentMaterialName.empty() && !materials.empty() ) 
	{
		szTransparentMaterialName = NFile::GetFilePath( materials.front().szName ) + NFile::GetFileTitle( materials.front().szName ) + "_transp.xdb";
		CPtr<IManipulator> pTranspMaterialMan = Singleton<IResourceManager>()->CreateObjectManipulator( "Material", szTransparentMaterialName );
		if ( pTranspMaterialMan == 0 ) 
		{
			if ( MakeMaterialCopy( materials.front().szName, szTransparentMaterialName ) == false )
			{
				Log( LT_ERROR, "Can't make material copy to create transparent material\n" );
				Log( LT_ERROR, StrFmt("\tMaterial name: %s\n", szTransparentMaterialName.c_str()) );
				return ER_FAIL;
			}
			pTranspMaterialMan = Singleton<IResourceManager>()->CreateObjectManipulator( "Material", szTransparentMaterialName );
		}
		//
		if ( pTranspMaterialMan != 0 ) 
			CManipulatorManager::SetValue( string("AM_TRANSPARENT"), pTranspMaterialMan, "AlphaMode" );
		// change texture export params to achive transparency
		CPtr<IManipulator> pMaterialMan = CManipulatorManager::CreateManipulatorFromReference( "Materials.[0]", pModelMan, 0, 0, 0 );
		CPtr<IManipulator> pTextureMan = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterialMan, 0, 0, 0 );
		bool bNeedConvertTexture = false;
		string szConversionType;
		CManipulatorManager::GetValue( &szConversionType, pTextureMan, "ConversionType" );
		if ( szConversionType != "CONVERT_TRANSPARENT" ) 
		{
			CManipulatorManager::SetValue( string("CONVERT_TRANSPARENT"), pTextureMan, "ConversionType" );
			bNeedConvertTexture = true;
		}
		string szTextureFormat;
		CManipulatorManager::GetValue( &szTextureFormat, pTextureMan, "Format" );
		if ( szTextureFormat != "TF_DXT3" ) 
		{
			CManipulatorManager::SetValue( string("TF_DXT3"), pTextureMan, "Format" );
			bNeedConvertTexture = true;
		}
		//
		if ( bNeedConvertTexture ) 
		{
			string szTextureName;
			CManipulatorManager::GetValue( &szTextureName, pMaterialMan, "Texture" );
			Singleton<IExporterContainer>()->ExportObject( pTextureMan, "Texture", szTextureName, true, NOT_EXPORT_REFERENCES );
		}
	}
	// 
	if ( pInfo->ModelCount != 1 ) 
	{
		Log( LT_ERROR, "Granny file contains incorrect number of models\n" );
		return ER_FAIL;
	}
	
	for ( int i = 0; i < pInfo->Models[0]->MeshBindingCount; ++i )
	{
		string szBoneName = pInfo->Models[0]->MeshBindings[i].Mesh->BoneBindings[0].BoneName;
		NStr::ToLower( &szBoneName );
		bool bTransparent = meshTranspInfoMap[szBoneName];
		//
		string szMaterialName;
		const string szMaterialRefName = StrFmt( "Materials.[%d]", i );
		if ( i >= nNumModelMaterials ) 
		{
			if ( pModelMan->InsertNode("Materials") == false )
			{
				Log( LT_ERROR, "Can't insert material in model\n" );
				return ER_FAIL;
			}
		}
		else
			CManipulatorManager::GetValue( &szMaterialName, pModelMan, szMaterialRefName );
		//
		if ( szMaterialName.empty() || szMaterialName == " " ) 
		{
			if ( bTransparent ) 
				CManipulatorManager::SetValue( szTransparentMaterialName, pModelMan, szMaterialRefName, true );
			else
				CManipulatorManager::SetValue( szOpaqueMaterialName, pModelMan, szMaterialRefName, true );
		}
		else
		{
			if ( bTransparent && szMaterialName != szTransparentMaterialName ) 
				CManipulatorManager::SetValue( szTransparentMaterialName, pModelMan, szMaterialRefName, true );
			else if ( !bTransparent && szMaterialName == szTransparentMaterialName ) 
				CManipulatorManager::SetValue( szOpaqueMaterialName, pModelMan, szMaterialRefName, true );
		}
	}

	return true;
}

}

using namespace NModelExporter;
REGISTER_EXPORTER_IN_DLL( Model, CModelExporter )


