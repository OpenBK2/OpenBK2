#include "StdAfx.h"

#include "TargetMesh.h"
#include "EditorScene.h"

#include "../3DLib/GGeometry.h"
#include "../3DLib/Transform.h"

#include "../3DMotor/GView.h"
#include "../3DMotor/Gfx.h"

#include "../SceneB2/Camera.h"

namespace NTargetMesh
{
static void FillVertexData( NGScene::SVertex &vertex )
{
	vertex.tex.Set( 0, 0 );
	CalcCompactVector( &(vertex.normal), CVec3(0, 0, 1) );
	CalcCompactVector( &(vertex.texU), CVec3(0, 0, 0) );
	CalcCompactVector( &(vertex.texV), CVec3(0, 0, 0) );
}

class CTargetMesh : public CPtrFuncBase<NGScene::CObjectInfo>
{
	CVec4 vOriginalVerticesPos[4];
public:
	CTargetMesh( float fSizeX, float fSizeY );
	bool NeedUpdate() { return true; }
	void Recalc();
};

class CTargetMeshTransform : public CFuncBase<SFBTransform>
{
public:
	CTargetMeshTransform() { Identity( &value.forward ); Identity( &value.backward ); }
	bool NeedUpdate() { return true; }
	void Recalc() {};
};

class CTargetMeshBound : public CFuncBase<SBound>
{
public:
	CTargetMeshBound() { value = NGScene::MakeLargeHintBound(); }
	bool NeedUpdate() { return true; }
	void Recalc() {};
};


CTargetMesh::CTargetMesh( float fSizeX, float fSizeY )
{
	vOriginalVerticesPos[0] = CVec4( fSizeX, fSizeY, 0.1f, 1.0f );
	vOriginalVerticesPos[1] = CVec4( -fSizeX, fSizeY, 0.1f, 1.0f );
	vOriginalVerticesPos[2] = CVec4( -fSizeX, -fSizeY, 0.1f, 1.0f );
	vOriginalVerticesPos[3] = CVec4( fSizeX, -fSizeY, 0.1f, 1.0f );

}

void CTargetMesh::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;

	const CTransformStack &cameraTransform = Camera()->GetTransform();

	CVec2 vScreenSize = NGfx::GetScreenRect();
	float fAspect = vScreenSize.x/vScreenSize.y;

	NGScene::CObjectInfo::SData objData;	
	objData.verts.resize( 4 );
	for ( int iVertex = 0; iVertex < 4; ++iVertex )
	{
		CVec4 vProj = vOriginalVerticesPos[iVertex];
		vProj.y *= fAspect;

		CVec4 vResult;
		cameraTransform.Get().backward.RotateHVector( &vResult, vProj );
		objData.verts[iVertex].pos.x = vResult.x/vResult.w;
		objData.verts[iVertex].pos.y = vResult.y/vResult.w;
		objData.verts[iVertex].pos.z = vResult.z/vResult.w;

		FillVertexData( objData.verts[iVertex] );
	}
	
	objData.geometry.reserve( 2 );
	objData.geometry.push_back( STriangle( 0, 1, 2) );
	objData.geometry.push_back( STriangle( 2, 3, 0) );

	pValue->AssignFast( &objData );
}

static CObj<CObjectBase> pTargetMesh = 0;

void CreateTargetMesh( float fSizeX, float fSizeY )
{
	CPtr<NGScene::IGameView> pGameView = EditorScene()->GetGView();
	if ( !pGameView )
		return;

	CPtr< CPtrFuncBase<NGScene::CObjectInfo> > pGeom = new CTargetMesh( fSizeX, fSizeY );

	NGScene::IGameView::SMeshInfo meshInfo;
	meshInfo.parts.resize( 1 );
	meshInfo.parts[0].pGeometry = pGeom;
	meshInfo.parts[0].pMaterial = pGameView->CreateMaterial( CVec4( 0.75, 0.75, 0.75, 0.5f ), false );

	pTargetMesh = pGameView->CreateMesh( meshInfo, new CTargetMeshTransform(), new CTargetMeshBound(), 0 );	

	return;
}

void DeleteTargetMesh()
{
	pTargetMesh = 0;
}
}
