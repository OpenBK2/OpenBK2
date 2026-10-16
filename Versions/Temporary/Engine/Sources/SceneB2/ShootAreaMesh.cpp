#include "stdafx.h"

#include "Scene.h"
#include "ShootAreaMesh.h"
#include "System/FastMath.h"

void AppendToVector( std::vector<NGScene::SVertex> &dst, const std::vector<NGScene::SVertex> &src )
{
	int nStart = dst.size();
	dst.resize( nStart + src.size() );
	for ( int i = 0; i < src.size(); ++i )
	{
		dst[nStart + i] = src[i];
	}
}

void AppendToVector( std::vector<STriangle> &dst, const std::vector<STriangle> &src, int nOffset )
{
	int nStart = dst.size();
	dst.resize( nStart + src.size() );
	for ( int i = 0; i < src.size(); ++i )
	{
		dst[nStart + i] = src[i];
		dst[nStart + i].i1 += nOffset;
		dst[nStart + i].i2 += nOffset;
		dst[nStart + i].i3 += nOffset;
	}
}

void CShootAreaMesh::Recalc()
{
	if ( pValue == 0 )
	{
		pValue = new NGScene::CObjectInfo;
	}
	NGScene::CObjectInfo::SData objData;
	std::vector<NGScene::SVertex> verts;
	std::vector<STriangle> allTris;
	std::vector<STriangle> tris;
	BuildCircle( verts, tris, fMaxRadius );
	AppendToVector( allTris, tris, objData.verts.size() );
	AppendToVector( objData.verts, verts );
	if ( fMinRadius > 1.0f )
	{
		BuildCircle( verts, tris, fMinRadius );
		AppendToVector( allTris, tris, objData.verts.size() );
		AppendToVector( objData.verts, verts );
	}
	if ( fStartAngle != fEndAngle )
	{
		BuildLine( verts, tris, fStartAngle );
		AppendToVector( allTris, tris, objData.verts.size() );
		AppendToVector( objData.verts, verts );
		BuildLine( verts, tris, fEndAngle );
		AppendToVector( allTris, tris, objData.verts.size() );
		AppendToVector( objData.verts, verts );
	}

	objData.geometry.swap( allTris );
	pValue->AssignFast( &objData );
}

CVec2 CShootAreaMesh::GetCenter()
{
	pTransform.Refresh();
	CVec3 vCenter = pTransform->GetValue().forward.GetTranslation();
	return CVec2( vCenter.x, vCenter.y );
}

CShootAreaMesh::CShootAreaMesh( CFuncBase<SFBTransform> *_pTransform,
                                float _fStartAngle,
                                float _fEndAngle,
                                float _fMinRadius,
                                float _fMaxRadius,
                                CTerrainManager *_pTerraManager,
                                float _fWidth )
	: pTransform( _pTransform )
	, fStartAngle( _fStartAngle )
	, fEndAngle( _fEndAngle )
	, fMinRadius( _fMinRadius )
	, fMaxRadius( _fMaxRadius )
	, pTerraManager( _pTerraManager )
	, fWidth( _fWidth )
{
}

CShootAreaMesh::CShootAreaMesh( const CVec2 &vCenter,
                                float _fStartAngle,
                                float _fEndAngle,
                                float _fMinRadius,
                                float _fMaxRadius,
                                CTerrainManager *_pTerraManager,
                                float _fWidth )
	: fStartAngle( _fStartAngle )
	, fEndAngle( _fEndAngle )
	, fMinRadius( _fMinRadius )
	, fMaxRadius( _fMaxRadius )
	, pTerraManager( _pTerraManager )
	, fWidth( _fWidth )
{
	SFBTransform transform;
	transform.forward.Set( CVec3( vCenter.x, vCenter.y, 0 ), QNULL );
	transform.backward.Set( -CVec3( vCenter.x, vCenter.y, 0 ), QNULL );
	pTransform = new CCSFBTransform( transform );
}

void CShootAreaMesh::BuildCircle( std::vector<NGScene::SVertex> &verts, std::vector<STriangle> &tris, float fRadius )
{
	const float fSRadius = fRadius - fWidth;
	const int nSegments = (std::max)( int( fRadius * FP_2PI / 4.0f ), 16 );
	const float fStep = FP_2PI / nSegments;
	verts.resize( nSegments * 2 );
	tris.resize( nSegments * 2 );

	CVec2 vCenter = GetCenter();
	for ( int i = 0; i < nSegments; ++i )
	{
		const float fRadiant = fStep * i;
		const float fRadiantX = NMath::Cos( fRadiant );
		const float fRadiantY = NMath::Sin( fRadiant );
		NGScene::SVertex &vert1 = verts[i * 2];
		NGScene::SVertex &vert2 = verts[i * 2 + 1];
		vert1.pos.Set( fRadiantX * fSRadius + vCenter.x, fRadiantY * fSRadius + vCenter.y, 0 );
		vert2.pos.Set( fRadiantX * fRadius + vCenter.x, fRadiantY * fRadius + vCenter.y, 0 );
		vert1.pos.z = pTerraManager->GetRealTerraHeight( vert1.pos.x, vert1.pos.y ) + 1.0f;
		vert2.pos.z = pTerraManager->GetRealTerraHeight( vert2.pos.x, vert2.pos.y ) + 1.0f;

		pTransform->GetValue().backward.RotateHVector( &vert1.pos, vert1.pos );
		pTransform->GetValue().backward.RotateHVector( &vert2.pos, vert2.pos );

		FillVertexData( vert1 );
		FillVertexData( vert2 );
		STriangle &tri1 = tris[i * 2];
		STriangle &tri2 = tris[i * 2 + 1];
		tri1.i1 = i * 2 + 1;
		tri1.i2 = ( i * 2 + 3 ) % ( nSegments * 2 );
		tri1.i3 = ( i * 2 + 2 ) % ( nSegments * 2 );
		tri2.i1 = i * 2 + 1;
		tri2.i2 = ( i * 2 + 2 ) % ( nSegments * 2 );
		tri2.i3 = i * 2;
	}
}

void CShootAreaMesh::BuildLine( std::vector<NGScene::SVertex> &verts, std::vector<STriangle> &tris, float fRadiant )
{
	const int nSegments = int( ( fMaxRadius - fMinRadius ) / 4.0f );
	const float fStep = ( fMaxRadius - fMinRadius ) / nSegments;
	verts.resize( ( nSegments + 1 ) * 2 );
	tris.resize( nSegments * 2 );
	const float fRadiantX = NMath::Cos( fRadiant );
	const float fRadiantY = NMath::Sin( fRadiant );
	const CVec2 vOrtDir( NMath::Cos( fRadiant - FP_PI * 0.5f ) * 0.5f, NMath::Sin( fRadiant - FP_PI * 0.5f ) * 0.5f );
	CVec2 vCenter = GetCenter();
	for ( int i = 0; i <= nSegments; ++i )
	{
		const float fCurrLength = fMinRadius + fStep * i;
		NGScene::SVertex &vert1 = verts[i * 2];
		NGScene::SVertex &vert2 = verts[i * 2 + 1];
		vert1.pos.Set(
				fRadiantX * fCurrLength - vOrtDir.x + vCenter.x, fRadiantY * fCurrLength - vOrtDir.y + vCenter.y, 0 );
		vert2.pos.Set(
				fRadiantX * fCurrLength + vOrtDir.x + vCenter.x, fRadiantY * fCurrLength + vOrtDir.y + vCenter.y, 0 );
		vert1.pos.z = pTerraManager->GetRealTerraHeight( vert1.pos.x, vert1.pos.y ) + 1.0f;
		vert2.pos.z = pTerraManager->GetRealTerraHeight( vert2.pos.x, vert2.pos.y ) + 1.0f;

		pTransform->GetValue().backward.RotateHVector( &vert1.pos, vert1.pos );
		pTransform->GetValue().backward.RotateHVector( &vert2.pos, vert2.pos );

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

void CShootAreaMesh::FillVertexData( NGScene::SVertex &vertex )
{
	vertex.tex.Set( 0, 0 );
	CalcCompactVector( &( vertex.normal ), CVec3( 0, 0, 1 ) );
	CalcCompactVector( &( vertex.texU ), CVec3( 0, 0, 0 ) );
	CalcCompactVector( &( vertex.texV ), CVec3( 0, 0, 0 ) );
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x101554C0, CShootAreaMesh )
