#include "StdAfx.h"

#include "../Misc/Win32Random.h"
#include "Scene.h"
#include "TerrainInfo.h"
#include "SurfController.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEF_SURF_BASE_LENGTH ( DEF_TILE_SIZE )
#define DEF_SURF_LENGTH ( DEF_TILE_SIZE * 4.0f )
#define DEF_SURF_HEIGHT 0.2f
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEF_NEW_TIME ( 1.0f / 50.0f )
#define DEF_FOAM_DY ( 0.08f * DEF_NEW_TIME * 12.0f )
#define DEF_FOAM_SPEED_QUAD 0.1f
#define DEF_FOAM_SPEED_QUAD_X 0.31622776f
#define DEF_FOAM_SPEED_QUAD_SCALE (1.0f / DEF_FOAM_SPEED_QUAD)
#define DEF_FOAM_BACK_DIST 0.5f
#define DEF_FOAM_BACK_DIST2 ( 1.0f - ( 1.0f - DEF_FOAM_BACK_DIST ) * ( 1.0f - DEF_FOAM_BACK_DIST ))
#define DEF_INV_ONE_MINUS_FOAM_BACK_DIST2 ( 1.0f / ( 1.0f - DEF_FOAM_BACK_DIST2 ) )
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetOuterOpacity( const CVec2 &vMax, NGScene::SVertex *pVertex )
{
	if ( (pVertex->pos.x < 0) || (pVertex->pos.y < 0) ||
		(pVertex->pos.x >= vMax.x) || (pVertex->pos.y >= vMax.y) )
	{
		pVertex->normal.w = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVisSurfPatch::UpdateGeomData()
{
	float fTexCoeff = 1, fTexOffs = 0;
	int nAlpha;

	for ( int i = 0; i < texYOffsets.size(); ++i )
	{
		const float fDy = texYOffsets[i] + (float)pTimer->GetValue() * DEF_FOAM_DY * DEF_NEW_TIME;
		const int nDy = (int)fDy;
		const int nTale = nDy % 3;
		if ( nTale < 2 )
		{
			const float fFoamDy = ( fDy - nDy + nTale ) * 0.5f;
			const float t = DEF_FOAM_SPEED_QUAD_X - fFoamDy * DEF_FOAM_SPEED_QUAD_X;
			const float fFoamY = 1.0f - t * t * DEF_FOAM_SPEED_QUAD_SCALE;
			fTexCoeff = Clamp( 1.0f / (0.01f + fFoamY * 0.99f ), 0.0f, 10.0f );
			fTexOffs = 0.0f;
			nAlpha = Clamp( Float2Int( fFoamY * 255.0f), 0, 255 );
		}
		else
		{
			const float fFoamDy = 1.0f - ( fDy - (float)nDy ) * 0.5f;
			const float t = DEF_FOAM_SPEED_QUAD_X - fFoamDy * DEF_FOAM_SPEED_QUAD_X;
			const float fFoamY = 1.0f - t * t * DEF_FOAM_SPEED_QUAD_SCALE;
			fTexCoeff = 1.0f;
			fTexOffs = Clamp( 1.0f - fFoamY, 0.0f, 1.0f );
			nAlpha = Clamp( Float2Int((fFoamY - DEF_FOAM_BACK_DIST2) * DEF_INV_ONE_MINUS_FOAM_BACK_DIST2 * 255.0f), 0, 255 );
		}

		const int nInd = i * 4;
		data.vertices[nInd].tex.y = fTexOffs + fTexCoeff;
		data.vertices[nInd].normal.w = nAlpha;
		data.vertices[nInd + 1].tex.y = fTexOffs + fTexCoeff;
		data.vertices[nInd + 1].normal.w = nAlpha;
		data.vertices[nInd + 2].tex.y = fTexOffs;
		data.vertices[nInd + 2].normal.w = nAlpha;
		data.vertices[nInd + 3].tex.y = fTexOffs;
		data.vertices[nInd + 3].normal.w = nAlpha;

		for ( int i = 0; i < 4; ++i )
		{
			SetOuterOpacity( vMaxMapCoords, &(data.vertices[nInd + i]) );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CVisSurfPatch::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	UpdateGeomData();

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;

	//pValue->AssignFast( &objData );
	pValue->Assign( &objData, false );
	bUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddPoints( const CVec2 &vWind, const vector<NDb::SVSOPoint> &points, CVisSurfPatch *pCurPatch )
{
	NGScene::SVertex vert;
	vert.texU.dw = 0xffffffff;
	vert.texV.dw = 0;
	CalcCompactVector( &(vert.normal), V3_AXIS_Z );

	const int nVertsOffs = pCurPatch->data.vertices.size();

	// fill vertices
	// TODO: old waterbanks
	for ( int i = 0; i < (points.size() - 1); ++i )
	{
		CVec3 vNorm = points[i + 1].vPos - points[i].vPos;
		vNorm.Set( vNorm.y, -vNorm.x, 0.0f );
		Normalize( &vNorm );
		//if ( ( vNorm * points[i].vNorm ) < 0.0f )
		//	vNorm = -vNorm;

		const float fAng = Clamp( vWind.x * vNorm.x + vWind.y * vNorm.y, 0.0f, 1.0f );
		const CVec3 vOffset = vNorm * DEF_WATER_BLENDING_DIST * 0.5f;
		const CVec3 vDist = vNorm * ( DEF_SURF_BASE_LENGTH + fAng * DEF_SURF_LENGTH );

		//const CVec3 &vNorm1 = points[i].vNorm;
		//const CVec3 &vNorm2 = points[i + 1].vNorm;
		//const CVec3 vOffset1 = vNorm1 * DEF_WATER_BLENDING_DIST;
		//const CVec3 vOffset2 = vNorm2 * DEF_WATER_BLENDING_DIST;

		vert.pos = points[i].vPos + vOffset/* + vOffset1*/;
		vert.pos.z = DEF_SURF_HEIGHT;
		vert.tex.Set( 0, 1 );
		pCurPatch->data.vertices.push_back( vert );

		vert.pos = points[i + 1].vPos + vOffset/* + vOffset2*/;
		vert.pos.z = DEF_SURF_HEIGHT;
		vert.tex.Set( 1, 1 );
		pCurPatch->data.vertices.push_back( vert );

		//const float fAng1 = Clamp( vWind.x * vNorm1.x + vWind.y * vNorm1.y, 0.0f, 1.0f );
		//vert.pos = points[i].vPos + vNorm1 * ( DEF_SURF_BASE_LENGTH + fAng1 * DEF_SURF_LENGTH ) + vOffset1;
		vert.pos = points[i].vPos + vOffset + vDist;
		vert.pos.z = DEF_SURF_HEIGHT;
		vert.tex.Set( 0, 0 );
		pCurPatch->data.vertices.push_back( vert );

		//const float fAng2 = Clamp( vWind.x * vNorm2.x + vWind.y * vNorm2.y, 0.0f, 1.0f );
		//vert.pos = points[i + 1].vPos + vNorm2 * ( DEF_SURF_BASE_LENGTH + fAng2 * DEF_SURF_LENGTH ) + vOffset2;
		vert.pos = points[i + 1].vPos + vOffset + vDist;
		vert.pos.z = DEF_SURF_HEIGHT;
		vert.tex.Set( 1, 0 );
		pCurPatch->data.vertices.push_back( vert );
	}

	// fill vertices
	// TODO: new waterbanks
	//for ( int i = 0; i < (points.size() - 1); ++i )
	//{
	//	const CVec3 &vNorm( points[i].vNorm );

	//	const float fAngle = Clamp( vWind.x * vNorm.x + vWind.y * vNorm.y, 0.0f, 1.0f );
	//	const CVec3 vDist = AI2Vis( points[i].fWidth ) * vNorm;

	//	vert.pos = points[i].vPos;
	//	vert.pos.z = DEF_SURF_HEIGHT;
	//	vert.tex.Set( 0, 1 );
	//	pCurPatch->data.vertices.push_back( vert );

	//	vert.pos = points[i + 1].vPos;
	//	vert.pos.z = DEF_SURF_HEIGHT;
	//	vert.tex.Set( 1, 1 );
	//	pCurPatch->data.vertices.push_back( vert );

	//	vert.pos = points[i].vPos  + vDist;
	//	vert.pos.z = DEF_SURF_HEIGHT;
	//	vert.tex.Set( 0, 0 );
	//	pCurPatch->data.vertices.push_back( vert );

	//	vert.pos = points[i + 1].vPos  + vDist;
	//	vert.pos.z = DEF_SURF_HEIGHT;
	//	vert.tex.Set( 1, 0 );
	//	pCurPatch->data.vertices.push_back( vert );
	//}

	// fill triangles
	for ( int i = 0; i < (points.size() - 1); ++i )
	{
		const int nInd = i * 4;
		if ( IsCCW(pCurPatch->data.vertices[nVertsOffs + nInd].pos, pCurPatch->data.vertices[nVertsOffs + nInd + 1].pos,
							 pCurPatch->data.vertices[nVertsOffs + nInd + 3].pos) )
			pCurPatch->data.triangles.push_back( STriangle(nVertsOffs + nInd, nVertsOffs + nInd + 1, nVertsOffs + nInd + 3) );
		else
			pCurPatch->data.triangles.push_back( STriangle(nVertsOffs + nInd, nVertsOffs + nInd + 3, nVertsOffs + nInd + 1) );
		if ( IsCCW(pCurPatch->data.vertices[nVertsOffs + nInd + 3].pos, pCurPatch->data.vertices[nVertsOffs + nInd + 2].pos,
							 pCurPatch->data.vertices[nVertsOffs + nInd].pos) )
			pCurPatch->data.triangles.push_back( STriangle(nVertsOffs + nInd + 3, nVertsOffs + nInd + 2, nVertsOffs + nInd) );
		else
			pCurPatch->data.triangles.push_back( STriangle(nVertsOffs + nInd + 2, nVertsOffs + nInd + 3, nVertsOffs + nInd) );
	}

	// set offsets
	for ( int i = 0; i < (points.size() - 1); ++i )
		pCurPatch->texYOffsets.push_back( NWin32Random::Random(0.0f, 3.0f) );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSurfController::Init( const float fAngle, const CVec2i &vMapSize, const vector<NWaterStuff::SSurfBorder> &borders, NGScene::IGameView *_pGScene )
{
	pGScene = _pGScene;

	static vector<NDb::SVSOPoint> points( 1024 );
	CVisSurfPatch *pCurPatch;

	CVec2 vBBMin, vBBMax;

	const CVec2 vWind( cos(fAngle + FP_PI / 2), sin(fAngle + FP_PI / 2) );

	surfPatches.resize( 0 );
	surfPatchesBounds.resize( 0 );

	for ( vector<NWaterStuff::SSurfBorder>::const_iterator it = borders.begin(); it != borders.end(); ++it )
	{
		points = it->points;

		vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
		vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );

		for ( vector<NDb::SVSOPoint>::iterator it = points.begin(); it != points.end(); ++it )
		{
			AI2Vis( &(it->vPos) );
			//it->vPos += it->vNorm * DEF_WATER_BLENDING_DIST * 0.5f;
			vBBMin.Minimize( CVec2(it->vPos.x, it->vPos.y) );
			vBBMax.Maximize( CVec2(it->vPos.x, it->vPos.y) );
		}

		//// map boundaries check
		//if ( (vBBMin.x >= vMapSize.x) || (vBBMin.y >= vMapSize.y) || 
		//		 (vBBMax.x <= 0) || (vBBMax.y <= 0) )
		//	continue;

		const float fExpand = DEF_WATER_BLENDING_DIST + DEF_SURF_BASE_LENGTH + DEF_SURF_LENGTH;
		vBBMin.x -= fExpand;
		vBBMin.y -= fExpand;
		vBBMax.x += fExpand;
		vBBMax.y += fExpand;

		surfPatches.push_back( CVisSurfPatchHolder() );
		surfPatches.back().pPatch = new CVisSurfPatch( pTimer, CVec2(vMapSize.x * VIS_TILE_SIZE, vMapSize.y * VIS_TILE_SIZE) );
		pCurPatch = surfPatches.back().pPatch;

		pCurPatch->data.vertices.reserve( (points.size() - 1) * 4 * 2 );
		pCurPatch->data.vertices.resize( 0 );
		pCurPatch->data.triangles.reserve( (points.size() - 1) * 2 * 2 );
		pCurPatch->data.triangles.resize( 0 );
		pCurPatch->texYOffsets.reserve( (points.size() - 1) * 2 );
		pCurPatch->texYOffsets.resize( 0 );

		AddPoints( vWind, points, pCurPatch );

		const bool bNeedCycle = ( fabs(points[0].vPos.x - points[points.size() - 1].vPos.x) < DEF_EPS ) &&
														( fabs(points[0].vPos.y - points[points.size() - 1].vPos.y) < DEF_EPS );

		for ( int i = 0; i < points.size(); ++i )
		{
			const NDb::SVSOPoint &nextPoint = ( i < (points.size() - 1) ) ? points[i + 1] : points[0];
			points[i].vPos = ( points[i].vPos + nextPoint.vPos ) * 0.5f;
			points[i].vNorm = ( points[i].vNorm + nextPoint.vNorm ) * 0.5f;
		}

		if ( !bNeedCycle )
			points.resize( points.size() - 1 );

		AddPoints( vWind, points, pCurPatch );

		SBound bound;
		bound.BoxInit( CVec3(vBBMin.x, vBBMin.y, 0.0f), CVec3(vBBMax.x, vBBMax.y, 1.0f) );
		//bound.BoxInit( CVec3(0.0f, 0.0f, 0.0f), CVec3(vMapSize.x, vMapSize.y, 1.0f) );
		surfPatchesBounds.push_back( new CCSBound() );
		surfPatchesBounds.back()->Set( bound );

		//NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
		NGScene::SFullRoomInfo room( NGScene::SRoomInfo(NGScene::LF_NEVER_STATIC, -100), 0, 0 );
		if ( pGScene )
		{
			surfPatches.back().pHolder = pGScene->CreateDynamicMesh( pGScene->MakeMeshInfo(pCurPatch, it->pSurfMaterial), 0,
																															 surfPatchesBounds.back(), NGScene::MakeLargeHintBound(), room );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
