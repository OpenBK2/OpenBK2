#include "StdAfx.h"

#include "LineMesh.h"

void CLineMesh::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;
	NGScene::CObjectInfo::SData objData;
	vector<NGScene::SVertex> verts;
	vector<STriangle> tris;
	BuildLine( objData.verts, tris );
	objData.geometry = tris;
	pValue->Assign( &objData, true );
}

CLineMesh::CLineMesh( const CVec2 &_vStart, const CVec2 &_vEnd, CTerrainManager *_pTerraManager ) :
	vStart( _vStart ), vEnd( _vEnd ), pTerraManager( _pTerraManager )
{
}

void CLineMesh::BuildLine( vector<NGScene::SVertex> &verts, vector<STriangle> &tris )
{
	const float fLength = fabs( vEnd - vStart );
	const int nSegments = int( fLength / 4.0f );
	const float fStep = fLength / nSegments;
	verts.resize( ( nSegments + 1 ) * 2 );
	tris.resize( nSegments * 2 );
	CVec2 vDir = vEnd - vStart;
	Normalize( &vDir );
	const CVec2 vOrtDir( vDir.y * 0.5f, -vDir.x * 0.5f );
	for ( int i = 0; i <= nSegments; ++i )
	{
		const float fCurrLength = fStep * i;
		NGScene::SVertex &vert1 = verts[i * 2];
		NGScene::SVertex &vert2 = verts[i * 2 + 1];
		vert1.pos.Set( vDir.x * fCurrLength - vOrtDir.x + vStart.x, vDir.y * fCurrLength - vOrtDir.y + vStart.y, 0 );
		vert2.pos.Set( vDir.x * fCurrLength + vOrtDir.x + vStart.x, vDir.y * fCurrLength + vOrtDir.y + vStart.y, 0 );
		vert1.pos.z = pTerraManager->GetRealTerraHeight( vert1.pos.x, vert1.pos.y ) + 1.0f;
		vert2.pos.z = pTerraManager->GetRealTerraHeight( vert2.pos.x, vert2.pos.y ) + 1.0f;
		FillVertexData( vert1 );
		FillVertexData( vert2 );
		if ( i != nSegments )
		{
			STriangle &tri1 = tris[i * 2];
			STriangle &tri2 = tris[i * 2 + 1];
			tri1.i1 = i * 2;
			tri1.i2 = i * 2 + 3;
			tri1.i3 = i * 2 + 2;
			tri2.i1 = i * 2;
			tri2.i2 = i * 2 + 1;
			tri2.i3 = i * 2 + 3;
		}
	}
}

void CLineMesh::FillVertexData( NGScene::SVertex &vertex )
{
	vertex.tex.Set( 0, 0 );
	CalcCompactVector( &(vertex.normal), CVec3(0, 0, 1) );
	CalcCompactVector( &(vertex.texU), CVec3(0, 0, 0) );
	CalcCompactVector( &(vertex.texV), CVec3(0, 0, 0) );
}

REGISTER_SAVELOAD_CLASS( 0x101554C1, CLineMesh )


