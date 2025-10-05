#include "stdafx.h"

#include "DBSceneConsts.h"
#include "Camera.h"
#include "SceneInternal.h"
//#include "TerrUtils.h"

void CSceneIconInfo::Visit( IAIVisitor *pVisitor )
{
	const float fMaxSize = max( vHalfSize.x * 2.0f, vHalfSize.y * 2.0f );
	const SHMatrix matr( fMaxSize, 0,				0,				vCenter.x,
											 0,				fMaxSize, 0,				vCenter.y,
											 0,				0,				fMaxSize, vCenter.z,
											 0,				0,				0,				1 );
 
	//pVisitor->AddHull( NDb::Get<NDb::SAIGeometry>( 1136 ), trans, 0, 0, 1 );
	pVisitor->AddHull( pAIGeometry, matr, 0, 0, 1 );
}

void CSceneIconInfo::OrientToViewer()
{
	const SHMatrix matrix = pCamera->GetViewMatrix();

	CVec3 s[4];
	NGfx::SCompactVector vNorm;
	CalcCompactVector( &vNorm, -pCamera->GetViewMatrix().GetZAxis3() );

	matrix.RotateVector( &s[0], CVec3( -vHalfSize.x, 0, -vHalfSize.y ) );
	matrix.RotateVector( &s[1], CVec3(	vHalfSize.x, 0, -vHalfSize.y ) ); 
	matrix.RotateVector( &s[2], CVec3(	vHalfSize.x, 0,	vHalfSize.y ) ); 
	matrix.RotateVector( &s[3], CVec3( -vHalfSize.x, 0,	vHalfSize.y ) );

	data.vertices[0].pos = vCenter + s[0];
	data.vertices[0].normal = vNorm;
	data.vertices[1].pos = vCenter + s[1];
	data.vertices[1].normal = vNorm;
	data.vertices[2].pos = vCenter + s[2];
	data.vertices[2].normal = vNorm;
	data.vertices[3].pos = vCenter + s[3];
	data.vertices[3].normal = vNorm;
}

void CSceneIconInfo::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	OrientToViewer();

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;

	pValue->AssignFast( &objData );
	bUpdate = false;
}

inline bool CSceneIconInfo::NeedUpdate()
{
	return bUpdate || pCamera->WasUpdated();
}

void CSceneIconInfo::CreateIcon( const int _nID, const CVec3 &_vCenter, const CVec2 &_vSize,
																 const CVec2 &_vTexMin, const CVec2 &_vTexMax, const NDb::SMaterial *_pMaterial,
																 const NDb::SAIGeometry *_pAIGeometry, CSyncSrc<CSceneIconInfo> *pSyncSrc )
{
	nID = _nID;
	pAIGeometry = _pAIGeometry;
	vCenter = _vCenter;
	vHalfSize = _vSize * 0.5f;

	data.vertices.resize( 4 );
	NGfx::SCompactVector texU;
	CalcCompactVector( &texU, CVec3( 1, 0, 0 ) );
	NGfx::SCompactVector texV;
	CalcCompactVector( &texV, CVec3( 0, 1, 0 ) );
	NGfx::SCompactVector norm;
	CalcCompactVector( &norm, CVec3( 0, 0, 1 ) );
	for ( std::vector<NGScene::SVertex>::iterator it = data.vertices.begin(); it != data.vertices.end(); ++it )
	{
		it->texU = texU;
		it->texV = texV;
		it->normal = norm;
	}
	data.vertices[0].tex.Set( _vTexMin.x, _vTexMin.y );
	data.vertices[1].tex.Set( _vTexMax.x, _vTexMin.y );
	data.vertices[2].tex.Set( _vTexMax.x, _vTexMax.y );
	data.vertices[3].tex.Set( _vTexMin.x, _vTexMax.y );

	data.triangles.resize( 2 );
	data.triangles[0].Set( 0, 1, 2 );
	data.triangles[1].Set( 2, 3, 0 );

	SBound bound;
	const float fMaxHalfSize = max( vHalfSize.x, vHalfSize.y );
	vBBHalfSize.Set( fMaxHalfSize, fMaxHalfSize, fMaxHalfSize );
	bound.BoxInit( vCenter - vBBHalfSize, vCenter + vBBHalfSize );

	pBound = new CCSBound();
	pBound->Set( bound );

	pMaterial = _pMaterial;

	AttachSyncSrc( pSyncSrc );
}

inline void CSceneIconInfo::MoveIcon( const CVec3 &_vCenter )
{
	vCenter = _vCenter;

	SBound bound;
	bound.BoxInit( vCenter - vBBHalfSize, vCenter + vBBHalfSize );

	if ( pBound == 0 )
		pBound = new CCSBound();
	pBound->Set( bound );
}

int CScene::AddSceneIcon( const int nID, const CVec3 &vCenter, const CVec2 &vSize, const CVec2 &vTexMin, const CVec2 &vTexMax,
													const NDb::SMaterial *pMaterial )
{
	NI_ASSERT( nID >= 0, StrFmt( "Wrong icon id (%d)", nID ) );

	CSceneIconHolder &iconsHolder = data[eScene]->iconsMap[nID];

	iconsHolder.pPatch = new CSceneIconInfo( Camera() );
	iconsHolder.pPatch->CreateIcon( nID, vCenter, vSize, vTexMin, vTexMax, pMaterial,
		data[eScene]->pSceneConsts->iconAIGeometry.pIcon, data[eScene]->pIconSyncSrc );

	NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
	iconsHolder.pHolder = data[eScene]->GetGScene()->CreateDynamicMesh( data[eScene]->GetGScene()->MakeMeshInfo( iconsHolder.pPatch,
		pMaterial ), 0, iconsHolder.pPatch->GetBound(), NGScene::MakeLargeHintBound(), room );

	iconsHolder.pPatch->ForceUpdate();

	return nID;
}

void CScene::RemoveSceneIcon( const int nID )
{
	data[eScene]->iconsMap.erase( nID );
}

void CScene::MoveSceneIcon( const int nID, const CVec3 &vCenter )
{
	SSceneData::CSceneIconsMap::iterator it = data[eScene]->iconsMap.find( nID );
	if ( ( it != data[eScene]->iconsMap.end() ) && ( it->second.pPatch ) )
	{
		it->second.pPatch->MoveIcon( vCenter );
		it->second.pPatch->ForceUpdate();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////data[eScene]->data[eScene]->data[eScene]->data[eScene]->data[eScene]->data[eScene]->data[eScene]->

