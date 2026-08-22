#include "stdafx.h"

#include "Scene.h"
#include "ShootAreaMesh.h"
#include "System/FastMath.h"

void AppendToVector( std::vector<NGScene::SVertex> &dst, const std::vector<NGScene::SVertex> &src )
{
	int nStart = dst.size();
	dst.resize( nStart + src.size() );
	for ( int i = 0; i < src.size(); ++i )
		dst[nStart + i] = src[i];
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

struct SRadianHeightFunctor : public SLinearHeightFunctor
{
	float GetHeight( float x )
	{
		return 4.0f * ( x + sqrt( 1.0f - x ) - 1.0f );
	}
	float GetGrid( float t )
	{
		if ( t < 0.5f )
			return t;
		return 1.0f - 2.0f * sqr( 1 - t );
	}
};

struct SCircleHeightFunctor : public SLinearHeightFunctor
{
	float GetHeight( float x )
	{
		return 1.0f - sqr( 1.0f - sqrt( 1.0f - 4.0f * sqr( x - 0.5f ) ) );
	}
	float GetGrid( float t )
	{
		if ( t < 0.5f )
			return 2.0f * t * t;
		return 1.0f - 2.0f * sqr( 1 - t );
	}	
};

void CShootAreaMesh::FillGrid( std::vector<float> &grid, int nNumPoints, SLinearHeightFunctor &func )
{
	grid.resize( nNumPoints );
	const float fStep = 1.0f / ( nNumPoints - 1 );
	for ( int i = 0; i < nNumPoints; ++i )
		grid[i] = func.GetGrid( i * fStep );
}

void CShootAreaMesh::LinearTransform( std::vector<float> &output, const std::vector<float> &input, float fMin, float fMax )
{
	output.resize( input.size() );
	for ( int i = 0; i < input.size(); ++i )
		output[i] = fMin + input[i] * ( fMax - fMin );
}

void CShootAreaMesh::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;
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

CShootAreaMesh::CShootAreaMesh( CFuncBase<SFBTransform> *_pTransform, float _fStartAngle, float _fEndAngle, float _fMinRadius, float _fMaxRadius, CTerrainManager *_pTerraManager, float _fWidth ) : 
	pTransform( _pTransform ), fStartAngle( _fStartAngle ), fEndAngle( _fEndAngle ), fMinRadius( _fMinRadius ), fMaxRadius( _fMaxRadius ), pTerraManager( _pTerraManager ), fWidth(_fWidth)
{
}

CShootAreaMesh::CShootAreaMesh( const CVec2 &vCenter, float _fStartAngle, float _fEndAngle, float _fMinRadius, float _fMaxRadius, CTerrainManager *_pTerraManager, float _fWidth ) : 
	fStartAngle( _fStartAngle ), fEndAngle( _fEndAngle ), fMinRadius( _fMinRadius ), fMaxRadius( _fMaxRadius ), pTerraManager( _pTerraManager ), fWidth(_fWidth)
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
		vert1.pos.Set( fRadiantX * fCurrLength - vOrtDir.x + vCenter.x, fRadiantY * fCurrLength - vOrtDir.y + vCenter.y, 0 );
		vert2.pos.Set( fRadiantX * fCurrLength + vOrtDir.x + vCenter.x, fRadiantY * fCurrLength + vOrtDir.y + vCenter.y, 0 );
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
	CalcCompactVector( &(vertex.normal), CVec3(0, 0, 1) );
	CalcCompactVector( &(vertex.texU), CVec3(0, 0, 0) );
	CalcCompactVector( &(vertex.texV), CVec3(0, 0, 0) );
}

void CShootAreaMesh::BuildSector( std::vector<NGScene::SVertex> &verts, std::vector<STriangle> &tris )
{
	const float fHeightCoeff = 10.0f;
	
	const int nNumRadians = 10;
	std::vector<float> radians;
	std::vector<float> transformedRadians;
	SRadianHeightFunctor radianFunctor;
	FillGrid( radians, nNumRadians, radianFunctor );
	LinearTransform( transformedRadians, radians, fStartAngle, fEndAngle );
	const int nNumCircles = 10;
	std::vector<float> circles;
	std::vector<float> transformedCircles;
	SCircleHeightFunctor circleFunctor;
	FillGrid( circles, nNumCircles, circleFunctor );
	LinearTransform( transformedCircles, circles, fMinRadius, fMaxRadius );

	const float nVertex = nNumRadians * nNumCircles;
	verts.resize( nVertex );
	for ( int i = 0; i < nNumRadians; ++i )
	{
		const float fRadianX = NMath::Cos( transformedRadians[i] );
		const float fRadianY = NMath::Sin( transformedRadians[i] );
		const float fRadianHeight = radianFunctor.GetHeight( radians[i] );
		for ( int j = 0; j < nNumCircles; ++i )
		{
			const float fCircle = transformedCircles[j];
			const float fCircleHeight = circleFunctor.GetHeight( circles[i] );
			NGScene::SVertex &vert = verts[i * nNumCircles + j];
			vert.pos.Set( fRadianX, fRadianY, fRadianHeight * fCircleHeight * fHeightCoeff );
			pTransform->GetValue().backward.RotateHVector( &vert.pos, vert.pos );

			vert.tex.Set( 0, 0 );
			CalcCompactVector( &(vert.normal), CVec3(0, 0, 1) );
			CalcCompactVector( &(vert.texU), CVec3(0, 0, 0) );
			CalcCompactVector( &(vert.texV), CVec3(0, 0, 0) );
		}
	}

	const int nTriangles = ( nNumRadians - 1 ) * ( nNumCircles - 1 ) * 2;
	tris.resize( nTriangles );
	for ( int i = 0; i < nNumRadians - 1; ++i )
	{
		for ( int j = 0; j < nNumCircles - 1; ++j )
		{
			int nIndex = ( i * nNumCircles + j ) * 2;
			STriangle &triangle1 = tris[nIndex];
			STriangle &triangle2 = tris[nIndex + 1];
			triangle1.i1 = i * nNumCircles + j;
			triangle1.i2 = ( i + 1 ) * nNumCircles + j + 1;
			triangle1.i3 = i * nNumCircles + j + 1;
			triangle2.i1 = i * nNumCircles + j;
			triangle2.i2 = ( i + 1 ) * nNumCircles + j;
			triangle2.i3 = ( i + 1 ) * nNumCircles + j + 1;
		}
	}
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x101554C0, CShootAreaMesh )


