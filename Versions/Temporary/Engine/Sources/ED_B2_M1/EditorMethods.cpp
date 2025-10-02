#include "StdAfx.h"

#include "misc/strproc.h"
#include "EditorMethods.h"

#include "EditorOptions.h"
#include "libdb/ResourceManager.h"
#include "EditorScene.h"
#include "SceneB2/Camera.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/Interface_EditorAI.h"
#include "SceneB2/TerraGen.h"
#include "System/FileUtils.h"
#include "System/BinaryResources.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "B2_M1_Terrain/DBTerrain.h"
#include "System/VFS.h"
#include "System/VFSOperations.h"
#include "MapEditorLib/Interface_MOD.h"
//#include "../Stats_B2_M1/TerraAIObserver.h"
//#include "../SceneB2/TerraGen.h"

namespace NEditor
{
	const string SZ_TERRA_BIN_FILE_NAME = "map.b2m";
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	string MakeMapPath( const CDBID &dbid )
	{
		return NDb::GetFolderName( dbid );// + "/" + NScene::SZ_TERRA_BIN_FILE_NAME;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	string GetTerrainBinFileName( const NDb::STerrain *pDesc )
	{
		const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		// new map location
		string szMapFileName = MakeMapPath( pDesc->GetDBID() );
		if ( NVFS::GetMainVFS()->DoesFileExist(szMapFileName) )
			return szMapFileName;
		// unsuccessfull :(
		return "";
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void CreateTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc )
	{
		const string szMapFilePath = NDb::GetFolderName( pDesc->GetDBID() );
		//
		return NScene::CreateTerrain( pTerraManager, pDesc, szMapFilePath );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool LoadTerrain( ITerraManager *pTerraManager, const NDb::STerrain *pDesc )
	{
		pTerraManager->SetAIObserver( Singleton<IEditorAI>()->CreateTerraAIObserver(pDesc->nNumPatchesX * AI_TILES_IN_PATCH, pDesc->nNumPatchesY * AI_TILES_IN_PATCH) );
		const string szMapFilePath = MakeMapPath( pDesc->GetDBID() );
		//
		return NScene::LoadTerrain( pTerraManager, pDesc, szMapFilePath );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SaveTerrain( struct ITerraManager *pTerraManager )
	{
		if ( const NDb::STerrain *pDesc = pTerraManager->GetDesc() )
		{
			const string szMapFilePath = MakeMapPath( pDesc->GetDBID() );
			//
			return NScene::SaveTerrain( pTerraManager, szMapFilePath );
		}
		return false;
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void LoadBgMap( const string &szDesiredSeason, const string &rszMapInfoNameLoaded, CVec2 *pBgMapSize )
	{
		string szMapInfoNameLoaded = rszMapInfoNameLoaded;
		NStr::ToLower( &szMapInfoNameLoaded );
		IEditorScene *pScene = EditorScene();
		if ( pScene == 0 ) 
			return;
		// get map and camera anchor from options
		string szMapName = NEditorOptions::GetBgMap( szDesiredSeason );
		NStr::ToLower( &szMapName );
		/**
		if ( szMapName == szMapInfoNameLoaded ) 
			return;
		/**/
		CVec3 vCameraAnchor = NEditorOptions::GetBgMapAnchor( szDesiredSeason );
		if ( vCameraAnchor == VNULL3 ) 
		{
			vCameraAnchor.x = 16.0f * AI_TILE_SIZE;
			vCameraAnchor.y = 16.0f * AI_TILE_SIZE;
			AI2Vis( &vCameraAnchor );
		}
		// load map
		if ( const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( CDBID( szMapName ) ) )
		{
			pScene->SetLight( pMapInfo->pLight );
			if ( ITerraManager *pTerraManager = pScene->GetTerraManager() )
			{
				pTerraManager->SetAIObserver( Singleton<IEditorAI>()->CreateTerraAIObserver(pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH) );
				SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
				//pTerrain->SetStreamPathes( pUserData->szExportDestinationFolder, pUserData->szExportSourceFolder );
				const string szMapFilePath = MakeMapPath( pMapInfo->GetDBID() );
				//
				NScene::LoadTerrain( pTerraManager, pMapInfo, szMapFilePath );
				if ( pBgMapSize ) 
				{
					pBgMapSize->x = pMapInfo->nNumPatchesX;
					pBgMapSize->y = pMapInfo->nNumPatchesY;
				}
			}
		}
		else
		{
			if ( pBgMapSize ) 
			{
				pBgMapSize->x = 0;
				pBgMapSize->y = 0;
			}
		}
		Camera()->SetAnchor( vCameraAnchor );
	}
}


//
//
//		UNIT COMMAND TYPE INFO
//
//

bool LoadUnitCommandTypesFromXML( vector<SUnitCommandTypeInfo> *pCmdTypes )
{
	if ( !pCmdTypes )
		return false;

	const string szFileName = Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + 
		"editor\\BitFields\\AIActions.xml";

	pCmdTypes->clear();

	CFileStream stream( szFileName.c_str(), CFileStream::WIN_READ_ONLY );
	CPtr<IXmlSaver> pXS = CreateXmlSaver( &stream, SAVER_MODE_READ );
	if ( pXS == 0 ) 
	{
		NI_ASSERT( pXS != 0, StrFmt("Can't open stream \"%s\" to read", szFileName.c_str()) );
		return false;
	}
	pXS->Add( "Fields", pCmdTypes );
	return true;
} 


bool GetPolyBoundingRect( float *pXmin, float *pYmin, float *pXmax, float *pYmax, const vector<CVec3> &rPoly )
{
	if ( rPoly.empty() )
		return false;

	CVec3 v0 = rPoly.front();

	float xMin = v0.x;
	float yMin = v0.y;
	float xMax = v0.x;
	float yMax = v0.y;

	for ( vector<CVec3>::const_iterator it = rPoly.begin(); it != rPoly.end(); ++it )
	{
		CVec3 v = *it;

		if ( v.x >= xMax )
			xMax = v.x;
		
		if ( v.x <= xMin )
			xMin = v.x;

		if ( v.y >= yMax )
			yMax = v.y;
		
		if ( v.y <= yMin )
			yMin = v.y;
	}

	*pXmin = xMin;
	*pYmin = yMin;
	*pXmax = xMax;
	*pYmax = yMax;

	return true;
}


CVec3 GetNearestTileCenter( const CVec3 &p )
{
	const float F_AI_TILE_SIZE = AI_TILE_SIZE;
	const float F_AI_TILE_SIZE_2 = F_AI_TILE_SIZE / 2.0f;

	CVec3 r = p;

	r /= F_AI_TILE_SIZE;
	r.x = int( r.x + 0.5f );
	r.y = int( r.y + 0.5f );
	r *= F_AI_TILE_SIZE;
	r.x += F_AI_TILE_SIZE_2;
	r.y += F_AI_TILE_SIZE_2;

	return r;
}


CVec3 GetNearestVisTileCorner( const CVec3 &p )
{
	const float F_VIS_TILE_SIZE = AI_TILE_SIZE * AI_TILES_IN_VIS_TILE;

	CVec3 r = p;

	r /= F_VIS_TILE_SIZE;
	r.x = int( r.x + 0.5f );
	r.y = int( r.y + 0.5f );
	r *= F_VIS_TILE_SIZE;

	return r;
}


int SUnitCommandTypeInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "Value", &nValue );
	saver.Add( "NeedTargetUnit", &nNeedTargetUnit );
	return 0;
}



