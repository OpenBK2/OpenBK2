#include "StdAfx.h"

#include "3DMotor/DBScene.h"
#include "GenTerrain.h"

#define DEF_FOOT_WIDTH_BASE 1.5f
#define DEF_FOOT_WIDTH 0.3f
#define DEF_FOOT_PARAGRAPH 2
#define DEF_FOOT_PARAGRAPH_INV ( 1.0f / ( DEF_FOOT_PARAGRAPH - 1 ) )
#define DEF_FOOT_PATCH_SIZE 64

void CTerraGen::AddFoot( const STerrainInfo::SFoot &foot )
{
	if ( !foot.pFootMaterial )
		return;

	SFootGFXInfo gfxNewFoot;
	gfxNewFoot.nID = foot.nID;
	gfxNewFoot.pMaterial = foot.pFootMaterial;
	gfxNewFoot.patches.reserve( 16 );
	gfxNewFoot.patches.resize( 0 );

	NMeshData::SMeshData data;

	data.vertices.reserve( 256 );
	data.triangles.reserve( 256 );

	const float fTexScaleX = foot.fTexGeomScale / foot.pFootMaterial->pTexture->nWidth;
	const float fTexScaleY = foot.fTexGeomScale / foot.pFootMaterial->pTexture->nHeight;

	float fFootTexX;
	float fAlpha1, fAlpha2;

	int nPatchesCount;

	for ( vector< vector<STerrainInfo::SVSOPoint> >::const_iterator itPointsArr = foot.points.begin(); itPointsArr != foot.points.end(); ++itPointsArr )
	{
		fFootTexX = 0.0f;

		data.vertices.resize( 0 );
		data.triangles.resize( 0 );

		nPatchesCount = 0;

		const int nPreLastNum = itPointsArr->size() - 1;
		for ( int i = 0; i < nPreLastNum; ++i )
		{
			const STerrainInfo::SVSOPoint &point1 = (*itPointsArr)[i];
			const STerrainInfo::SVSOPoint &point2 = (*itPointsArr)[i + 1];

			const float fFootWidth1 = DEF_FOOT_WIDTH_BASE + ( DEF_FOOT_WIDTH * point1.fRadius );
			const float fFootWidth2 = DEF_FOOT_WIDTH_BASE + ( DEF_FOOT_WIDTH * point2.fRadius );

			const CVec3 vNextPoint1 = point1.vPos + point1.vNorm * fFootWidth1;
			const CVec3 vNextPoint2 = point2.vPos + point2.vNorm * fFootWidth2;

			const float fNextFootTexX = fFootTexX + sqrt( fabs2( point1.vPos.x - point2.vPos.x ) + fabs2( point1.vPos.y - point2.vPos.y ) ) * fTexScaleX;

			const float fTexY1 = point1.vPos.z * fTexScaleY;
			const float fTexY2 = point2.vPos.z * fTexScaleY;
			const float fNextTexY1 = fTexY1 - fFootWidth1 * fTexScaleY;
			const float fNextTexY2 = fTexY2 - fFootWidth2 * fTexScaleY;

			if ( i < DEF_FOOT_PARAGRAPH )
				fAlpha1 = (float)i * DEF_FOOT_PARAGRAPH_INV;
			else
				if ( i > (itPointsArr->size() - DEF_FOOT_PARAGRAPH - 1) )
					fAlpha1 = (float)( itPointsArr->size() - i - 1 ) * DEF_FOOT_PARAGRAPH_INV;
				else
					fAlpha1 = 1.0f;
			if ( ( i + 1 ) < DEF_FOOT_PARAGRAPH )
				fAlpha2 = (float)( i + 1 ) * DEF_FOOT_PARAGRAPH_INV;
			else
				if ( ( i + 1 ) > (itPointsArr->size() - DEF_FOOT_PARAGRAPH - 1) )
					fAlpha2 = (float)( itPointsArr->size() - (i + 1) - 1 ) * DEF_FOOT_PARAGRAPH_INV;
				else
					fAlpha2 = 1.0f;

			ProjectQuadOnTerrain( point1.vPos, vNextPoint1, vNextPoint2, point2.vPos,
														CVec2(fFootTexX, fTexY1), CVec2(fFootTexX, fNextTexY1),
														CVec2(fNextFootTexX, fNextTexY2), CVec2(fNextFootTexX, fTexY2),
														fAlpha1, 0.0f, 0.0f, fAlpha2, &data );

			fFootTexX = fNextFootTexX;

			if ( ++nPatchesCount >= DEF_FOOT_PATCH_SIZE )
			{
				gfxNewFoot.patches.push_back( data );
				nPatchesCount = 0;
				data.vertices.resize( 0 );
				data.triangles.resize( 0 );
			}
		}

		if ( nPatchesCount > 0 )
			gfxNewFoot.patches.push_back( data );
	}

	terrainGfxInfo.foots.push_back( gfxNewFoot );

	if ( pGfxObserver )
		pGfxObserver->AddFoot( &(terrainGfxInfo.foots.back()) );
}

//void CTerraGen::AddAllFoots()
//{
//	for ( list<STerrainInfo::SFoot>::const_iterator it = terrainInfo.foots.begin(); it != terrainInfo.foots.end(); ++it )
//		AddFoot( *it );
//}

void CTerraGen::AddAllNeededFoots()
{
	for ( vector<int>::const_iterator it = needAddFoots.begin(); it != needAddFoots.end(); ++it )
	{
		for ( list<STerrainInfo::SFoot>::const_iterator itFoot = terrainInfo.foots.begin(); itFoot !=	terrainInfo.foots.end(); ++itFoot )
		{
			if ( *it == itFoot->nID )
				AddFoot( *itFoot );
		}
	}
	needAddFoots.resize( 0 );
}

void CTerraGen::RemoveFoot( const int nVSOID )
{
	//RemovePeakInfo( nVSOID );
	//RemovePeakGfxInfo( nVSOID );
	RemoveFootInfo( nVSOID );
	RemoveFootGfxInfo( nVSOID );
	if ( pGfxObserver )
	{
		//pGfxObserver->RemovePeak( nVSOID );
		pGfxObserver->RemoveFoot( nVSOID );
	}
}

void CTerraGen::RemoveFootInfo( const int nVSOID )
{
	for ( list<STerrainInfo::SFoot>::iterator it = terrainInfo.foots.begin(); it != terrainInfo.foots.end(); )
	{
		if ( it->nID == nVSOID )
			it = terrainInfo.foots.erase( it );
		else
			++it;
	}
}

void CTerraGen::RemoveFootGfxInfo( const int nVSOID )
{
	for ( list<SFootGFXInfo>::iterator it = terrainGfxInfo.foots.begin(); it != terrainGfxInfo.foots.end(); )
	{
		if ( it->nID == nVSOID )
			it = terrainGfxInfo.foots.erase( it );
		else
			++it;
	}
}


