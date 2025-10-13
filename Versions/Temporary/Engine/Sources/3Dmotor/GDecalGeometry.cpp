#include "stdafx.h"
#include "GDecalGeometry.h"
#include "3DLib/Transform.h"
#include "GCombiner.h"

#include <cstdint>

namespace NGScene
{
const float F_SHIFT = 0.004f;

struct STriangleCreator
{
	std::vector<STriangle> *pDst;
	int nRoot, nPrev;

	STriangleCreator( std::vector<STriangle> *_pDst ) : pDst(_pDst), nRoot(-1), nPrev(-1) {}
	void AddVertex( int n )
	{
		if ( nRoot == -1 )
		{
			nRoot = n;
			return;
		}
		if ( nPrev == -1 )
		{
			nPrev = n;
			return;
		}
		pDst->push_back( STriangle( nRoot, nPrev, n ) );
		nPrev = n;
	}
};

struct SQuadProjection
{
	// projection info
	CVec3 vVecU, vVecV, vProjectDirection;
	float fU0, fV0;
	// short cuts for sphere check
	float fUleng, fVleng;
	CVec3 vOrigin;

	bool CheckSphere( const CVec3 &ptCenter, float fRadius )
	{
		float fV = fV0 + ptCenter *  vVecV;
		float fDV = fRadius * fVleng;
		if ( fV - fDV > 1 || fV + fDV < 0 )
			return false;
		float fU = fU0 + ptCenter *  vVecU;
		float fDU = fRadius * fUleng;
		if ( fU-fDU > 1 || fU + fDU < 0 )
			return false;
		return true;
	}
	void Setup( const CVec3 &_vOrigin, const CVec3 &_vNormal, const CVec2 &vSize, float fRotation, const CVec2 &_vShift )
	{
		float fCos = cos( fRotation ), fSin = sin( fRotation );
		CVec3 vU( vSize.x * fCos, vSize.x * fSin, 0 );
		CVec3 vV( -vSize.y * fSin, vSize.y * fCos, 0 );
		vVecU = CVec3( -vU.y, 0, -vU.x );
		vVecV = CVec3( -vV.y, 0, -vV.x );

		SHMatrix m;
		MakeMatrix( &m, VNULL3, _vNormal );
		m.RotateHVector( &vVecU, vVecU );
		m.RotateHVector( &vVecV, vVecV );

		fUleng = 1 / fabs( vVecU );
		vVecU = vVecU * ( fUleng * fUleng );
		fVleng = 1 / fabs( vVecV );
		vVecV = vVecV * ( fVleng * fVleng );

		vProjectDirection = _vNormal;
		Normalize( &vProjectDirection );
		fU0 = -( _vOrigin * vVecU ) + _vShift.x;
		fV0 = -( _vOrigin * vVecV ) + _vShift.y;
		vOrigin = _vOrigin;
	}
	CVec2 Project( const CVec3 &v ) { return CVec2( fU0 + v * vVecU, fV0 + v * vVecV ); }
};

struct SSPoint
{
	CVec3 point, normal;
	CVec2 uv;
};

struct SShadowPoly
{
	std::vector<SSPoint> points;
};

class CShadowBuilder
{
	CObjectInfo::SData info;
	CPtr<CObjectInfo> pRes;
	// projection info
	SQuadProjection projection;
	CVec3 vShift;
	float fNormalEdge, fDepthMargin;

	void AddShadowTriangle( SShadowPoly *pTri );
	void GeneratePointPosition( SSPoint *pRes, const CVec3 &pnt )
	{
		pRes->point = pnt;
		//pRes->normal = normal;
		pRes->uv = projection.Project( pnt );
	}
	uint16_t AddPoint( SSPoint &pnt );
public:
	CShadowBuilder( CObjectInfo *_pRes, float _fNormalEdge, float _fDepthMargin ) 
		: pRes(_pRes), fNormalEdge(_fNormalEdge), fDepthMargin(_fDepthMargin)
	{
		info.geometry.resize(0);
	}
	~CShadowBuilder() { pRes->Assign( &info, false ); }
	void Setup( const CVec3 &vOrigin, const CVec3 &vNormal, const CVec2 &vSize, float fRotation, const CVec2 &_vShift );
	void AddObject( const CObjectInfo &info, const SDiscretePos &_srcPos );
};

inline float mixedMult( const CVec3 &a, const CVec3 &b, const CVec3 &c )
{
	return a.x*b.y*c.z + a.y*b.z*c.x + a.z*b.x*c.y - a.z*b.y*c.x - a.y*b.x*c.z - a.x*b.z*c.y;
}

static void CalcWeighted( CVec3 *pRes, const CVec3 &a, float fA, const CVec3 &b, float fB )
{
	pRes->x = ( a.x * fB - b.x * fA );
	pRes->y = ( a.y * fB - b.y * fA );
	pRes->z = ( a.z * fB - b.z * fA );
}

// мощная работа по наложению тени ( no welding yet )
// calc average point
static void CalcMiddlePoint( const SSPoint &a, const SSPoint &b, SSPoint *pRes, float fA, float fB )
{
	float f1 = 1 / ( fB - fA );
	fA *= f1;
	fB *= f1;
	CalcWeighted( &pRes->point, a.point, fA, b.point, fB );
	CalcWeighted( &pRes->normal, a.normal, fA, b.normal, fB );
	Normalize( &pRes->normal );
	pRes->uv[0] = a.uv[0] * fB - b.uv[0] * fA;
	pRes->uv[1] = a.uv[1] * fB - b.uv[1] * fA;
}

static void Split( SShadowPoly *pRes, const SShadowPoly &src, const std::vector<float> &f )
{
	ASSERT( src.points.size() == f.size() );
	int nRes = 0;
	if ( src.points.size() < 2 )
	{
		pRes->points.resize( 0 );
		return;
	}
	pRes->points.resize( src.points.size() + 2 );
	int nPrev = src.points.size() - 1;
	for ( int k = 0; k < src.points.size(); nPrev = k++ )
	{
		float fPrev = f[nPrev], fCur = f[k];
		if ( fCur >= 0 )
		{
			if ( fPrev < 0 && fCur > 0 )
				CalcMiddlePoint( src.points[nPrev], src.points[k], &pRes->points[nRes++], fPrev, fCur );
			pRes->points[nRes++] = src.points[k];
		}
		else
		{
			if ( fPrev > 0 )
				CalcMiddlePoint( src.points[nPrev], src.points[k], &pRes->points[nRes++], fPrev, fCur );
		}
	}
	pRes->points.resize( nRes );
}

static std::vector<float> fPlaneOffsets;
static bool DoSplit( SShadowPoly *pRes, const SShadowPoly &src, const CVec3 &vUV )
{
	fPlaneOffsets.resize( src.points.size() );
	for ( int k = 0; k < src.points.size(); ++k )
		fPlaneOffsets[k] = src.points[k].uv[0] * vUV.x + src.points[k].uv[1] * vUV.y + vUV.z;
	Split( pRes, src, fPlaneOffsets );
	return pRes->points.size() > 2;
}

inline void ProjectNormalized( NGfx::SCompactVector *pRes, const CVec3 &src, const CVec3 &vNormal )
{
	CVec3 vRes;
	vRes = src - vNormal * (src * vNormal);
	Normalize( &vRes );
	NGfx::CalcCompactVector( pRes, vRes );
}
uint16_t CShadowBuilder::AddPoint( SSPoint &pnt )
{
	int nRes = info.verts.size();
	info.verts.resize( nRes + 1 );
	SVertex &v = info.verts[nRes];
	v.pos = pnt.point;
	v.tex.u = pnt.uv[0];
	v.tex.v = 1 - pnt.uv[1];
	NGfx::CalcCompactVector( &v.normal, pnt.normal );
	ProjectNormalized( &v.texU, projection.vVecU, pnt.normal );
	ProjectNormalized( &v.texV, projection.vVecV, pnt.normal );
	return nRes;
}

// clip by planes
float fClipPlanes[4][3] = { {1,0,0}, {0,1,0}, {-1,0,1}, {0,-1, 1} };
static SShadowPoly tBuf;
void CShadowBuilder::AddShadowTriangle( SShadowPoly *pTri )
{
	//float fp1 = fpC0 + tri.pnts[0].point * vecC; // clip by front plane
	SShadowPoly *pTSrc, *pTDst;
	pTSrc = pTri;
	pTDst = &tBuf;
	for ( int k = 0; k < 4; ++k )
	{
		DoSplit( pTDst, *pTSrc, CVec3( fClipPlanes[k][0], fClipPlanes[k][1], fClipPlanes[k][2] ) );
		if ( pTDst->points.size() < 3 )
			return;
		std::swap( pTSrc, pTDst );
	}
	STriangleCreator tc( &info.geometry );
	for ( int k = 0; k < pTSrc->points.size(); ++k )
		tc.AddVertex( AddPoint( pTSrc->points[k] ) );
}

void CShadowBuilder::AddObject( const CObjectInfo &info, const SDiscretePos &_srcPos )
{
	SShadowPoly tri;
	SFBTransform trans;
	_srcPos.MakeMatrix( &trans );
	const std::vector<STriangle> &infoTris = info.GetGeometry();
	const std::vector<CVec3> &infoPositions = info.GetPositions();
	std::vector<SSPoint> projPoints( info.GetPositions().size() );
	int nTest = 0;
	for ( int k = 0; k < projPoints.size(); ++k )
	{
		CVec3 pos;
		trans.forward.RotateHVector( &pos, infoPositions[ k ] );
		SSPoint &dst = projPoints[k];
		GeneratePointPosition( &dst, pos + vShift );
		int nU = *(int*)&dst.uv.u, nV = *(int*)&dst.uv.v;
		nTest |= ( nU  > 0 );
		nTest |= ( nU < 0x3f800000 ) << 1; // < 1
		nTest |= ( nV > 0 ) << 2;
		nTest |= ( nV < 0x3f800000 ) << 3; // < 1
	}
	if ( nTest != 15 )
	{
		//DebugTrace( "%d src polygons, early exit\n", infoPolys.size() - 1 );
		return;
	}
	for ( int k = 0; k < infoTris.size(); ++k )
	{
		const STriangle &t = infoTris[k];
		int nIndices[3] = { t.i1, t.i2, t.i3 };
		// check normal
		{
			int n0 = info.GetPositionIndices()[ nIndices[0] ];
			int n1 = info.GetPositionIndices()[ nIndices[1] ];
			int n2 = info.GetPositionIndices()[ nIndices[2] ];
			// calc normal
			CVec3 vTriNormal = ( projPoints[n2].point - projPoints[n0].point ) ^ ( projPoints[n1].point - projPoints[n0].point );
			float fLeng = fabs( vTriNormal );
			if ( fLeng > 0 )
			{
				float fTest = vTriNormal * projection.vProjectDirection;
				if ( fTest >= fNormalEdge * fLeng ) //-0.01f
					continue;
			}
		}
		tri.points.resize( 3 );
		for ( int i = 0; i < 3; ++i )
		{
			CVec3 normal;
			trans.backward.RotateVectorTransposed( &normal, NGfx::GetVector( info.GetVertices()[ nIndices[i] ].normal ) );
			SSPoint &dst = tri.points[ i ];
			dst = projPoints[ info.GetPositionIndices()[ nIndices[i] ] ];
			dst.normal = normal;
		}
		if ( fDepthMargin < 1e10f )
		{
			float fDepthMax = -1e30f, fDepthMin = 1e30f;
			for ( int k = 0; k < tri.points.size(); ++k )
			{
				float fDepth = ( tri.points[k].point - projection.vOrigin ) * projection.vProjectDirection;
				fDepthMin = (std::min)( fDepth, fDepthMin );
				fDepthMax = (std::max)( fDepth, fDepthMax );
			}
			if ( fDepthMin > fDepthMargin || fDepthMax < -fDepthMargin )
				continue;
		}
		AddShadowTriangle( &tri );
	}
	//DebugTrace( "%d src polygons, %d res polygons\n", infoPolys.size() - 1, CShadowBuilder::info.geometry.polys.size() - 1 );
}

void CShadowBuilder::Setup( const CVec3 &_vOrigin, const CVec3 &_vNormal, const CVec2 &vSize, float fRotation, const CVec2 &_vShift )
{
	projection.Setup( _vOrigin, _vNormal, vSize, fRotation, _vShift );
	vShift = projection.vProjectDirection * F_SHIFT;
}

// CDecalGeometry

CDecalGeometry::CDecalGeometry( CPtrFuncBase<CObjectInfo> *p, const SDiscretePos &_srcPos,
	const CVec3 &_vOrigin, const CVec3 &_vNormal, const CVec2 &_vSize, float _fR, const CVec2 &_vShift,
	float _fNormalEdge, float _fDepthMargin )
	: pSource(p), srcPos(_srcPos), vOrigin(_vOrigin), vNormal(_vNormal), vSize(_vSize), fRotation(_fR), vShift(_vShift),
	fNormalEdge(_fNormalEdge), fDepthMargin(_fDepthMargin)
{
}

void CDecalGeometry::Recalc()
{
	pSource.Refresh();
	if ( !pSource->GetValue() )
		return;
	pValue = new CObjectInfo;
	CShadowBuilder sb( pValue, fNormalEdge, fDepthMargin );
	sb.Setup( vOrigin, vNormal, vSize, fRotation, vShift );
	sb.AddObject( *pSource->GetValue(), srcPos );
}

// CPerPolyDecal

CPerPolyDecal::CPerPolyDecal( IPart *_pPart, const std::vector<CVec3> &_srcPositions ) : pPart(_pPart), srcPositions(_srcPositions)
{
	data.geometry.resize(0);
}

void CPerPolyDecal::Recalc()
{
	if ( !IsValid(pPart) )
	{
		pPart = 0;
		data = CObjectInfo::SData();//.Clear();
		pValue = new CObjectInfo;
		pValue->Assign( &data, false );
		return;
	}
	if ( !pPart->HasLoadedObjectInfo() )
		return;
	CDGPtr<CPtrFuncBase<CObjectInfo> > pSource( pPart->GetObjectInfoNode() );
	pSource.Refresh();
	if ( !pSource->GetValue() )
	{
		ASSERT(0);
		return;
	}

	if ( srcPositions.empty() || srcPositions.size() != pSource->GetValue()->GetPositions().size() )
		TransformPart( pPart, &srcPositions, 0 );

	Recalc( &data, *pSource->GetValue(), srcPositions );

	pValue = new CObjectInfo;
	pValue->Assign( data, false );
}

// CExplosionDecalGeometry

class CObjectInfoProcessor
{
	CObjectInfo::SData &info;
	std::vector<int> vertMap;
protected:
	const std::vector<CVec3> &srcPositions;
	const std::vector<SUVInfo> &srcVertices;
	const std::vector<uint16_t> &posIndices;
	const std::vector<STriangle> &srcTris;
	const std::vector<SRealVertexWeight> &srcWeights;
public:
	CObjectInfoProcessor( CObjectInfo::SData *pInfo, const CObjectInfo &src ) 
		: info(*pInfo), srcPositions( src.GetPositions() ), srcVertices( src.GetVertices() ),
		posIndices( src.GetPositionIndices() ), vertMap( src.GetVertices().size(), -1 ),
		srcTris( src.GetGeometry() ),
		srcWeights( src.GetWeights() )
	{
		info = CObjectInfo::SData();
	}
	struct SPolygon
	{
		std::vector<SVertex> polygon;
		std::vector<int> polygonIndices;
	};
	bool HasResIndex( int nIdx ) const { return vertMap[ nIdx ] >= 0; }
	int GetResIndex( int nIdx ) const { return vertMap[ nIdx ]; }
	void AddVertex( SPolygon *pRes, int nIdx )
	{
		ASSERT( HasResIndex( nIdx ) );
		pRes->polygon.push_back( info.verts[ GetResIndex( nIdx ) ] );
		pRes->polygonIndices.push_back( nIdx );
	}
	void AddVertex( SPolygon *pRes, int nIdx, const SVertex &v )
	{
		ASSERT( !HasResIndex( nIdx ) );
		pRes->polygon.push_back( v );
		pRes->polygonIndices.push_back( nIdx );
	}
	bool IsOutside( const SPolygon &poly )//vector<SVertex> &polygon, const vector<int> &polygonIndices )
	{
		bool bXPos = false, bXNeg = false, bYPos = false, bYNeg = false;
		for ( int k = 0; k < poly.polygon.size(); ++k )
		{
			const SVertex &v = poly.polygon[k];
			bXPos |= v.tex.u > 0;
			bYPos |= v.tex.v > 0;
			bXNeg |= v.tex.u < 1;
			bYNeg |= v.tex.v < 1;
		}
		return !bXPos || !bXNeg || !bYPos || !bYNeg;
	}
	void AddPolygon( const SPolygon &poly )
	{
		STriangleCreator tc( &info.geometry );
		for ( int i = 0; i < poly.polygon.size(); ++i )
		{
			int nIdx = poly.polygonIndices[i];
			int nRes;
			if ( !HasResIndex( nIdx ) )
			{
				nRes = info.verts.size();
				info.verts.push_back( poly.polygon[i] );
				if ( !srcWeights.empty() )
					info.weights.push_back( GetWeight( srcWeights[ posIndices[ nIdx ] ] ) );
				vertMap[ nIdx ] = nRes;
			}
			else
				nRes = GetResIndex( nIdx );
			tc.AddVertex( nRes );
		}
	}
};

class CExplosionShadowBuilder : CObjectInfoProcessor
{
public:
	CExplosionShadowBuilder( CObjectInfo::SData *pInfo, const CObjectInfo &src, const std::vector<CVec3> &positions,
		const CVec3 &_vOrigin, float _fSize, float _fRotation );
};

CExplosionShadowBuilder::CExplosionShadowBuilder( CObjectInfo::SData *pInfo, const CObjectInfo &src, const std::vector<CVec3> &xformed,
	const CVec3 &vOrigin, float fSize, float _fRotation )
: CObjectInfoProcessor( pInfo, src )
{
	float fSin = sin(_fRotation), fCos = cos(_fRotation);
	for ( int k = 0; k < srcTris.size(); ++k )
	{
		const STriangle &t = srcTris[k];
		int nIndices[3] = { t.i1, t.i2, t.i3 };
		int np0 = posIndices[ t.i1 ];
		int np1 = posIndices[ t.i2 ];
		int np2 = posIndices[ t.i3 ];
		CVec3 vTriNormal = ( xformed[np2] - xformed[np0] ) ^ ( xformed[np1] - xformed[np0] );
		CVec3 vXNormal(0,0,1), vXTexU(1,0,0), vXTexV(0,1,0);
		float fLeng = fabs( vTriNormal );
		if ( fLeng > 0 )
		{
//			float fTest = vTriNormal * ( vRealOrigin - srcPositions[np0] );
//			if ( fTest >= -0.01f * fLeng )
//				continue;
			vXNormal = vTriNormal;
			Normalize( &vXNormal );
			vXTexU = vXNormal ^ CVec3(0.5f,0.23f,0);
			if ( fabs2(vXTexU) == 0 )
				vXTexU = vXNormal ^ CVec3(0,0,1);
			Normalize( &vXTexU );
			vXTexV = vXNormal ^ vXTexU;
			Normalize( &vXTexV );
		}
		// calc poly
		SPolygon poly;
		for ( int i = 0; i < 3; ++i )
		{
			int nIdx = nIndices[i];
			// calc uv
			if ( !HasResIndex( nIdx ) )
			{
				const SUVInfo &uv = srcVertices[ nIdx ];
				const CVec3 &pos = srcPositions[ posIndices[ nIdx ] ];
				const CVec3 &vXformed = xformed[ posIndices[ nIdx ] ];
				const CVec3 vTexU = NGfx::GetVector( uv.texU );
				const CVec3 vTexV = NGfx::GetVector( uv.texV );
				SVertex res;
				res.pos = pos;
				res.normal = uv.normal;
				//res.texV = CVec3(0,0,0);

				CVec3 vLightDir = vXformed - vOrigin;
				float fH = (vLightDir * vXNormal) / fSize;
				float fS = (std::max)( 0.0001f, 1 - sqr( fH ) );
				NGfx::CalcCompactVector( &res.texU, CVec3( 1, 0, 0 ) );
				NGfx::CalcCompactVector( &res.texV, CVec3( fS, sqrt(1-sqr(fS)), 0 ) );
				float fU = (vLightDir * vXTexU) / fS / fSize;
				float fV = (vLightDir * vXTexV) / fS / fSize;
				res.tex.u = fU * fCos - fV * fSin;
				res.tex.v = fU * fSin + fV * fCos;
				res.tex.u = Clamp( res.tex.u * 0.5f + 0.5f, -15.0f, 15.0f );
				res.tex.v = Clamp( res.tex.v * 0.5f + 0.5f, -15.0f, 15.0f );
				
				AddVertex( &poly, nIdx, res );
			}
			else
				AddVertex( &poly, nIdx );
		}
		if ( IsOutside( poly ) )
			continue;
		AddPolygon( poly );
	}
}

void CExplosionDecalGeometry::Recalc( CObjectInfo::SData *pRes, const CObjectInfo &info, const std::vector<CVec3> &positions )
{
	CExplosionShadowBuilder sb( pRes, info, positions, vOrigin, fSize, fRotation );
}

// CPerPolyDecalGeometry

class CPerPolyDecalBuilder : CObjectInfoProcessor
{
	SQuadProjection projection;
public:
	CPerPolyDecalBuilder( CObjectInfo::SData *pInfo, const CObjectInfo &src, const std::vector<CVec3> &positions,
		const CVec3 &vOrigin, const CVec3 &vNormal, const CVec2 &vSize, float fRotation, const CVec2 vShift );
};

CPerPolyDecalBuilder::CPerPolyDecalBuilder( CObjectInfo::SData *pInfo, const CObjectInfo &src, const std::vector<CVec3> &xformed,
	const CVec3 &vOrigin, const CVec3 &_vNormal, const CVec2 &vSize, float fRotation, const CVec2 vShift )
	: CObjectInfoProcessor( pInfo, src )
{
	projection.Setup( vOrigin, _vNormal, vSize, fRotation, vShift );
	for ( int k = 0; k < srcTris.size(); ++k )
	{
		const STriangle &t = srcTris[k];
		int nIndices[3] = { t.i1, t.i2, t.i3 };
		int np0 = posIndices[ t.i1 ];
		int np1 = posIndices[ t.i2 ];
		int np2 = posIndices[ t.i3 ];
		CVec3 vTriNormal = ( xformed[np2] - xformed[np0] ) ^ ( xformed[np1] - xformed[np0] );
		//CVec3 vXNormal(0,0,1), vXTexU(1,0,0), vXTexV(0,1,0);
		float fLeng = fabs( vTriNormal );
		/*if ( fLeng > 0 )
		{
			if ( vTriNormal * projection.vProjectDirection >= -0.4f * fLeng )
				continue;
		}*/
		// calc poly
		SPolygon poly;
		float fDepthMax = -1e30f, fDepthMin = 1e30f;
		for ( int i = 0; i < 3; ++i )
		{
			int nIdx = nIndices[i];
			// calc uv
			const CVec3 &vXformed = xformed[ posIndices[ nIdx ] ];
			float fDepth = ( vXformed - vOrigin ) * projection.vProjectDirection;
			fDepthMin = (std::min)( fDepth, fDepthMin );
			fDepthMax = (std::max)( fDepth, fDepthMax );
			if ( !HasResIndex( nIdx ) )
			{
				const SUVInfo &uv = srcVertices[ nIdx ];
				const CVec3 &pos = srcPositions[ posIndices[ nIdx ] ];
				//const CVec3 vTexU = NGfx::GetVector( uv.texU );
				//const CVec3 vTexV = NGfx::GetVector( uv.texV );
				SVertex res;
				res.pos = pos;
				res.normal = uv.normal;
				NGfx::CalcCompactVector( &res.texU, CVec3( 1, 0, 0 ) );
				NGfx::CalcCompactVector( &res.texV, CVec3( 0, 1, 0 ) );
				res.tex = projection.Project( vXformed );
				res.tex.u = Clamp( res.tex.u, -15.0f, 15.0f );
				res.tex.v = Clamp( res.tex.v, -15.0f, 15.0f );

				AddVertex( &poly, nIdx, res );
			}
			else
				AddVertex( &poly, nIdx );
		}
		//if ( fDepthMin > F_DEPTH_WINDOW || fDepthMax < -F_DEPTH_WINDOW )
		//	continue;
		if ( IsOutside( poly ) )
			continue;

		AddPolygon( poly );
	}
}

void CPerPolyDecalGeometry::Recalc( CObjectInfo::SData *pRes, const CObjectInfo &info, const std::vector<CVec3> &positions )
{
	CPerPolyDecalBuilder ppb( pRes, info, positions, vOrigin, vNormal, vSize, fRotation, vShift );
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x00442130, CDecalGeometry )
REGISTER_SAVELOAD_CLASS( 0x004c2170, CExplosionDecalGeometry )
REGISTER_SAVELOAD_CLASS( 0x006c2160, CPerPolyDecalGeometry )

