#include "stdafx.h"

#include "3DLib/Bound.h"
#include "3DLib/GGeometry.h"
#include "ProceduralRock.h"
#include "System/Dg.h"

#include <array>
#include <cmath>
#include <random>

namespace
{
constexpr int N_SIDES = 8;
constexpr int N_BASE = 3 * N_SIDES;
constexpr int N_PEAK = N_BASE + 1;
// The three cross sections run counterclockwise from the foot to the widest ledge to the crown.
using TRockShape = std::array<CVec3, N_PEAK + 1>;

const std::array<CVec2, N_SIDES> directions = { CVec2( 1, 0 ),
                                                CVec2( 0.7071f, 0.7071f ),
                                                CVec2( 0, 1 ),
                                                CVec2( -0.7071f, 0.7071f ),
                                                CVec2( -1, 0 ),
                                                CVec2( -0.7071f, -0.7071f ),
                                                CVec2( 0, -1 ),
                                                CVec2( 0.7071f, -0.7071f ) };

TRockShape BuildShape( const CVec3 &vSize, float fRoughness, uint32_t nSeed )
{
	std::mt19937 random( nSeed );
	// Sample the high 24 bits explicitly so different standard libraries use the same distribution.
	auto vary = [&]() { return fRoughness * ( static_cast<float>( random() >> 8 ) / 8388608.0f - 1.0f ); };
	const float fLeanX = vary() * vSize.x * 0.12f;
	const float fLeanY = vary() * vSize.y * 0.12f;
	TRockShape shape;
	shape[N_BASE].Set( 0, 0, 0 );
	shape[N_PEAK].Set( fLeanX, fLeanY, vSize.z );
	for ( int i = 0; i < N_SIDES; ++i )
	{
		const CVec2 &vDir = directions[i];
		const float fRadius = 0.45f + 0.05f * vary();
		const float fSkew = 0.18f * vary();
		const float x = vSize.x * fRadius * ( vDir.x - fSkew * vDir.y );
		const float y = vSize.y * fRadius * ( vDir.y + fSkew * vDir.x );
		const float fFoot = 0.83f + 0.12f * vary();
		const float fLedgeZ = vSize.z * ( 0.33f + 0.17f * vary() );
		const float fCrown = 0.61f + 0.20f * vary();
		const float fCrownZ = vSize.z * ( 0.80f + 0.15f * vary() );
		shape[i].Set( x * fFoot, y * fFoot, 0 );
		shape[N_SIDES + i].Set( x, y, fLedgeZ );
		shape[2 * N_SIDES + i].Set( fLeanX + x * fCrown, fLeanY + y * fCrown, fCrownZ );
	}
	return shape;
}

CVec2 GetRockUV( int nPoint )
{
	// The BigStone seasonal atlases all have a large slanted rock swatch in their upper left.
	// Unwrap the surface in concentric rings inside it, keeping the unused atlas background out.
	const CVec2 vCenter( 0.29f, 0.22f );
	if ( nPoint >= N_BASE )
	{
		return vCenter;
	}
	constexpr float radii[] = { 1.0f, 0.78f, 0.42f };
	const CVec2 &v = directions[nPoint % N_SIDES];
	return vCenter + CVec2( 0.22f * v.x - 0.035f * v.y, 0.085f * v.x + 0.105f * v.y ) * radii[nPoint / N_SIDES];
}

void AddFace( NGScene::CObjectInfo::SData *pMesh, const TRockShape &shape, int a, int b, int c )
{
	const CVec3 ab = shape[b] - shape[a];
	const CVec3 ac = shape[c] - shape[a];
	const CVec2 uvAB = GetRockUV( b ) - GetRockUV( a );
	const CVec2 uvAC = GetRockUV( c ) - GetRockUV( a );
	const float fSign = uvAB.x * uvAC.y < uvAB.y * uvAC.x ? -1.0f : 1.0f;
	CVec3 vNormal = ab ^ ac;
	Normalize( &vNormal );
	CVec3 vTangent = ( ab * uvAC.y - ac * uvAB.y ) * fSign;
	Normalize( &vTangent );
	NGScene::SVertex vertex;
	NGfx::CalcCompactVector( &vertex.normal, vNormal );
	NGfx::CalcCompactVector( &vertex.texU, vTangent );
	NGfx::CalcCompactVector( &vertex.texV, ( vNormal ^ vTangent ) * fSign );
	const auto nFirst = static_cast<uint16_t>( pMesh->verts.size() );
	for ( int index : { a, b, c } )
	{
		vertex.pos = shape[index];
		vertex.tex = GetRockUV( index );
		pMesh->verts.push_back( vertex );
	}
	pMesh->geometry.emplace_back( nFirst, nFirst + 1, nFirst + 2 );
}

class CRockGeometry : public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CRockGeometry );

	CRockGeometry() {}

public:
	explicit CRockGeometry( const TRockShape &shape )
	{
		NGScene::CObjectInfo::SData mesh;
		mesh.geometry.reserve( N_SIDES * 6 );
		mesh.verts.reserve( N_SIDES * 18 );
		for ( int i = 0; i < N_SIDES; ++i )
		{
			const int j = ( i + 1 ) % N_SIDES;
			AddFace( &mesh, shape, N_BASE, j, i );
			for ( int ring = 0; ring < 2; ++ring )
			{
				const int a = ring * N_SIDES + i;
				const int b = ring * N_SIDES + j;
				AddFace( &mesh, shape, a, b, b + N_SIDES );
				AddFace( &mesh, shape, a, b + N_SIDES, a + N_SIDES );
			}
			AddFace( &mesh, shape, 2 * N_SIDES + i, 2 * N_SIDES + j, N_PEAK );
		}
		pValue = new NGScene::CObjectInfo;
		pValue->Assign( &mesh, false );
	}
};
} // namespace

CPtrFuncBase<NGScene::CObjectInfo> *
CreateProceduralRock( const CVec3 &vSize, float fRoughness, uint32_t nSeed, SBound *pBound )
{
	if ( !std::isfinite( vSize.x ) || !std::isfinite( vSize.y ) || !std::isfinite( vSize.z ) || vSize.x <= 0 ||
	     vSize.y <= 0 || vSize.z <= 0 || !( fRoughness >= 0 && fRoughness <= 1 ) )
	{
		return nullptr;
	}
	const TRockShape shape = BuildShape( vSize, fRoughness, nSeed );
	if ( pBound )
	{
		SBoundCalcer bounds;
		bounds.LookSet( shape, SGetSelf<CVec3>() );
		bounds.Make( pBound );
	}
	return new CRockGeometry( shape );
}
