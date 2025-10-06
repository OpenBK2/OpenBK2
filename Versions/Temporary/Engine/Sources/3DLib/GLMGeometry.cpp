#include "stdafx.h"
#include "GLMGeometry.h"
#include "RectPacker.h"
#include "3Dmotor/GfxBuffers.h"

namespace NGScene
{
struct SLightmapInfo
{
	WORD n1, n2, n3, n4;    // vertices indexing
	char nOrder1, nOrder2;  // first(1,2,3) | 0x4 for inverse
};

struct SLMLOD
{
	std::vector<NRectPacker::SRect> lmaps;
	CTPoint<int> lmSize;  // in pixels
};

// CSquarePacker

struct SEdgeInfo
{
	int nU, nV, nXSize, nYSize;
};
inline bool operator==( const SEdgeInfo &a, const SEdgeInfo &b ) { return memcmp( &a, &b, sizeof(a) ) == 0; }
struct SEdgeInfoHash
{
	int operator()( const SEdgeInfo &e ) const { return e.nU ^ e.nV ^ e.nXSize ^ e.nYSize; }
};

class CSquarePacker
{
	struct STriInfo
	{
		int n1, n2, n3, n4;
		bool bInverse, bInverse1;
		char nFirst, nFirst1;
		WORD nXSize, nYSize;
		STriInfo() {}
		STriInfo( int _n1, int _n2, int _n3, bool _bInverse, char _nFirst, WORD _nXSize, WORD _nYSize )
			: n1(_n1), n2(_n2), n3(_n3), bInverse(_bInverse), nFirst(_nFirst), nFirst1(0), bInverse1(false),nXSize(_nXSize), nYSize(_nYSize){}
	};
	std::vector<STriInfo> squares;
	const CObjectInfo::SData &info;
	float fTexelsPerMeter;
	typedef std::unordered_map<SEdgeInfo, int, SEdgeInfoHash> CEdgesHash;
	CEdgesHash edgesInfo;
	int nMaxSize;

	void PushTriangle( int n1, int n2, int n3, char nFirst );
public:
	CSquarePacker( int _nMaxSize, const CObjectInfo::SData &_info, float _fTexelPerMeter )
		: info(_info), fTexelsPerMeter(_fTexelPerMeter), nMaxSize(_nMaxSize)
	{
	}
	void AddTriangle( int n1, int n2, int n3 );
	void Build( SLMLOD *pLMLOD, std::vector<SLightmapInfo> *pLMaps );
};

// round either to pow2 or pow2 + prev_pow2
inline int RoundToGood( int n )
{
	for ( int nRes = 2; nRes != 0; nRes <<= 1 )
	{
		if ( nRes >= n )
			return nRes;
		int nTest = nRes + ( nRes >> 1 );
		if ( nTest >= n )
			return nTest;
	}
	ASSERT(0);
	return n;
}

// n1 - vertex with largest angle
void CSquarePacker::PushTriangle( int n1, int n2, int n3, char nFirst )
{
	const CVec3 &v1 = info.verts[n1].pos;//GetPositions()[ info.GetPositionIndices()[n1] ];
	const CVec3 &v2 = info.verts[n2].pos;//GetPositions()[ info.GetPositionIndices()[n2] ];
	const CVec3 &v3 = info.verts[n3].pos;//GetPositions()[ info.GetPositionIndices()[n3] ];
	// calc sizes
	float fSize21 = fabs( v2 - v1 );
	float fSize31 = fabs( v3 - v1 );
	// select largest size (&swap indices if needed)
	bool bInverse = false;
	if ( fSize31 > fSize21 )
	{
		std::swap( fSize21, fSize31 );
		std::swap( n2, n3 );
		bInverse = true;
	}
	// add to storage
	int nXSize = (std::max)( Float2Int( fSize21 * fTexelsPerMeter ), 2 );
	int nYSize = (std::max)( Float2Int( fSize31 * fTexelsPerMeter ), 2 );
	nXSize = RoundToGood( nXSize );
	nYSize = RoundToGood( nYSize );
	ASSERT( nYSize <= nXSize );
	nXSize = (std::min)( nXSize, nMaxSize );
	nYSize = (std::min)( nYSize, nXSize );
	if ( nXSize == nYSize && n2 > n3 )
	{
		std::swap( n2, n3 );
		bInverse = !bInverse;
	}
	SEdgeInfo edge;
	edge.nU = n2;
	edge.nV = n3;
	edge.nXSize = nXSize;
	edge.nYSize = nYSize;
	CEdgesHash::iterator i = edgesInfo.find( edge );
	if ( i != edgesInfo.end() )
	{
		STriInfo &tri = squares[i->second];
		if ( tri.n2 != n2 )
		{
			std::swap( n2, n3 );
			bInverse = !bInverse;
		}
		ASSERT( tri.n2 == n2 && tri.n3 == n3 );
		tri.n4 = n1;
		tri.nFirst1 = nFirst;
		tri.bInverse1 = bInverse;
		edgesInfo.erase( i );
		return;
	}
	if ( nXSize != nYSize )
		std::swap( edge.nU, edge.nV );
	edgesInfo[edge] = squares.size();
	squares.push_back( STriInfo( n1, n2, n3, bInverse, nFirst, nXSize, nYSize ) );
}

void CSquarePacker::AddTriangle( int n1, int n2, int n3 )
{
	// select largest angle
	const CVec3 &v1 = info.verts[n1].pos;//GetPositions()[ info.GetPositionIndices()[n1] ];
	const CVec3 &v2 = info.verts[n2].pos;//GetPositions()[ info.GetPositionIndices()[n2] ];
	const CVec3 &v3 = info.verts[n3].pos;//GetPositions()[ info.GetPositionIndices()[n3] ];
	float f1 = fabs2( v2 - v3 );
	float f2 = fabs2( v1 - v3 );
	float f3 = fabs2( v1 - v2 );
	if ( f2 > f1 )
	{
		if ( f2 > f3 )
			PushTriangle( n2, n3, n1, 2 );
		else
			PushTriangle( n3, n1, n2, 3 );
	}
	else
	{
		if ( f3 > f1 )
			PushTriangle( n3, n1, n2, 3 );
		else
			PushTriangle( n1, n2, n3, 1 );
	}
}

void CSquarePacker::Build( SLMLOD *pLMLOD, std::vector<SLightmapInfo> *pLMaps )
{
	SLMLOD &lmLOD = *pLMLOD;
	std::vector<SLightmapInfo> &lmaps = *pLMaps;
	lmaps.resize( squares.size() );
	lmLOD.lmaps.resize( squares.size() );
	for ( int i = 0; i < squares.size(); ++i )
	{
		const STriInfo &tri = squares[i];
		SLightmapInfo &res = lmaps[i];
		res.n1 = tri.n1;
		res.n2 = tri.n2;
		res.n3 = tri.n3;
		res.nOrder1 = tri.nFirst;
		if ( tri.bInverse )
			res.nOrder1 |= 4;
		if ( tri.nFirst1 )
		{
			res.n4 = tri.n4;
			res.nOrder2 = tri.nFirst1;
			if ( tri.bInverse1 )
				res.nOrder2 |= 4;
		}
		else
		{
			res.n4 = 0xffff;
			res.nOrder2 = 0;
		}
		NRectPacker::SRect &r = lmLOD.lmaps[i];
		r.nXSize = tri.nXSize;
		r.nYSize = tri.nYSize;
	}
	NRectPacker::PackRects( &lmLOD.lmaps, &lmLOD.lmSize );
	//		ASSERT( lmLOD.lmSize.x <= nLMSize );
	//		ASSERT( lmLOD.lmSize.y <= nLMSize );
}

static void CalcMirroredVector( NGfx::SCompactVector *pRes, 
	const NGfx::SCompactVector &a, const NGfx::SCompactVector &b, const NGfx::SCompactVector &c )
{
	CVec3 v = NGfx::GetVector(a) + NGfx::GetVector(b) - NGfx::GetVector(c);
	Normalize( &v );
	NGfx::CalcCompactVector( pRes, v );
}

static void CopyVertex( SVertex *pDst, CVec2 *pSecondTex, const SVertex &src, const CVec2 &tex )
{
	*pDst = src;
	NGfx::SShortTextureUV lm;
	NGfx::CalcLMCoords( &lm, tex.x, tex.y );
	// using inverse xform from one used in GCombiner
	*pSecondTex = NGfx::GetTexCoords( lm );//tex;
}

static void GenVertices( std::vector<SVertex> *pRes, std::vector<CVec2> *pSecondTex, const std::vector<SVertex> &src,
	const SLMLOD &lmLOD, const std::vector<SLightmapInfo> &lmaps, int nLMSize, const CTPoint<int> &_shift,
	float fLMUVShift )
{
	// hard times - lm vertex buffer is required
	float fLMSize1 = 1.0f / nLMSize;//NGfx::GetLMTexResolution();
	ASSERT( lmLOD.lmaps.size() == lmaps.size() );
	int nLMs = lmaps.size();
	pRes->resize( nLMs * 4 );
	pSecondTex->resize( pRes->size() );
	// Создаем общий вертекс буфер, в котором слиты все кусочки
	int iVert = 0;
	{
		//CTRect<int> lmRegion = p->GetLMRegion();
		CTRect<int> lmRegion;
		lmRegion.Set( 
			_shift.x, _shift.y, 
			_shift.x + lmLOD.lmSize.x, _shift.y + lmLOD.lmSize.y );
		int nShiftX = lmRegion.x1;
		int nShiftY = lmRegion.y1;
		for ( int k = 0; k < lmaps.size(); ++k )
		{
			const SLightmapInfo &lm = lmaps[k];
			const SVertex &v1 = src[ lm.n1 ];
			const SVertex &v2 = src[ lm.n2 ];
			const SVertex &v3 = src[ lm.n3 ];
			SVertex v4;
			if ( lm.nOrder2 == 0 )
			{
				//float fAlpha = ( ( v2.pos - v3.pos ) * ( v1.pos - v3.pos ) ) / fabs2( v2.pos - v3.pos );
				////ASSERT( fAlpha >= 0 && fAlpha <= 1 );
				//fAlpha = Max( Min( fAlpha, 1.0f ), 0.0f );
				//v4.pos = v3.pos * fAlpha + v2.pos * ( 1 - fAlpha );
				//v4.normal = v3.normal * fAlpha + v2.normal * ( 1 - fAlpha );
				//Normalize( &v4.normal );
				//v4.texU = v3.texU * fAlpha + v2.texU * ( 1 - fAlpha );
				//Normalize( &v4.texU );
				//v4.texV = v3.texV * fAlpha + v2.texV * ( 1 - fAlpha );
				//Normalize( &v4.texV );
				//v4.color = v1.color;
				v4.pos = v2.pos + v3.pos - v1.pos;
				CalcMirroredVector( &v4.normal, v2.normal, v3.normal, v1.normal );
				CalcMirroredVector( &v4.texU, v2.texU, v3.texU, v1.texU );
				CalcMirroredVector( &v4.texV, v2.texV, v3.texV, v1.texV );
			}
			else
				v4 = src[ lm.n4 ];
			const NRectPacker::SRect &lmmap = lmLOD.lmaps[k];
			float fU = ( lmmap.nXShift + nShiftX ) * fLMSize1;
			float fV = ( lmmap.nYShift + nShiftY ) * fLMSize1;
			float fDU = lmmap.nXSize * fLMSize1;
			float fDV = lmmap.nYSize * fLMSize1;
			float fDelta = fLMUVShift * fLMSize1;
			CopyVertex( &(*pRes)[iVert+0], &(*pSecondTex)[iVert+0], v1, CVec2( fU + fDelta, fV + fDelta ) );
			CopyVertex( &(*pRes)[iVert+1], &(*pSecondTex)[iVert+1], v2, CVec2( fU + fDU - fDelta, fV + fDelta ) );
			CopyVertex( &(*pRes)[iVert+2], &(*pSecondTex)[iVert+2], v3, CVec2( fU + fDelta, fV + fDV - fDelta ) );
			CopyVertex( &(*pRes)[iVert+3], &(*pSecondTex)[iVert+3], v4, CVec2( fU + fDU - fDelta, fV + fDV - fDelta ) );
			iVert += 4;
		}
	}
}

static void BuildLM( const CObjectInfo::SData &src, float fLMResolution, int nLMSize, SLMLOD *pLOD, std::vector<SLightmapInfo> *pLMaps )
{
	SLMLOD &lmLOD = *pLOD;
	std::vector<SLightmapInfo> &lmaps = *pLMaps;

	// calc lightmaps
	std::vector<STriangle> tris = src.geometry;
	CSquarePacker packer( nLMSize, src, fLMResolution );
	for ( int k = 0; k < tris.size(); ++k )
		packer.AddTriangle( tris[k].i1, tris[k].i2, tris[k].i3 );

	packer.Build( &lmLOD, &lmaps );
}

void MakeLMGeometry( CObjectInfo::SData *pRes, CTPoint<int> *pSize, const CObjectInfo::SData &src, 
	float fLMResolution, int nLMSize, const CTPoint<int> &_shift )
{
	SLMLOD lmLOD;
	std::vector<SLightmapInfo> lmaps;
	BuildLM( src, fLMResolution, nLMSize, &lmLOD, &lmaps );

	std::vector<STriangle> lmTris;
	for ( int k = 0; k < lmaps.size(); ++k )
	{
		int nOffset = k * 4;
		const SLightmapInfo &lm = lmaps[k];
		if ( lm.nOrder1 & 4 )
			lmTris.push_back( STriangle( nOffset + 0, nOffset + 2, nOffset + 1 ) );
		else
			lmTris.push_back( STriangle( nOffset + 0, nOffset + 1, nOffset + 2 ) );
		if ( lm.nOrder2 == 0 )
			continue;
		if ( lm.nOrder2 & 4 )
			lmTris.push_back( STriangle( nOffset + 3, nOffset + 2, nOffset + 1 ) );
		else
			lmTris.push_back( STriangle( nOffset + 3, nOffset + 1, nOffset + 2 ) );
	}
	pRes->geometry = lmTris;

	GenVertices( &pRes->verts, &pRes->secondTex, src.verts, lmLOD, lmaps, nLMSize, _shift, 0.5f );

	*pSize = lmLOD.lmSize;
}

void MakeLMCalcGeometry( CObjectInfo::SData *pRes, CTPoint<int> *pSize, const CObjectInfo::SData &src, 
	float fLMResolution, int nLMSize, const CTPoint<int> &_shift, std::vector<SLMQuad> *pQuads )
{
	SLMLOD lmLOD;
	std::vector<SLightmapInfo> lmaps;
	BuildLM( src, fLMResolution, nLMSize, &lmLOD, &lmaps );

	std::vector<STriangle> lmTris;
	lmTris.resize( lmaps.size() * 2 );
	for ( int k = 0; k < lmaps.size(); ++k )
	{
		int nOffset = k * 4;
		STriangle &tri1 = lmTris[ k*2 ];
		tri1.i1 = nOffset + 0;
		tri1.i2 = nOffset + 1;
		tri1.i3 = nOffset + 2;
		STriangle &tri2 = lmTris[ k*2 + 1 ];
		tri2.i1 = nOffset + 3;
		tri2.i2 = nOffset + 2;
		tri2.i3 = nOffset + 1;
	}
	pRes->geometry = lmTris;

	GenVertices( &pRes->verts, &pRes->secondTex, src.verts, lmLOD, lmaps, nLMSize, _shift, 0.45f );

	*pSize = lmLOD.lmSize;

	if ( pQuads )
	{
		pQuads->resize( lmLOD.lmaps.size() );
		for ( int k = 0; k < pQuads->size(); ++k )
		{
			const SLightmapInfo &lm = lmaps[k];
			const NRectPacker::SRect &r = lmLOD.lmaps[k];
			SLMQuad &q = (*pQuads)[k];
			q.bFull = lm.nOrder2 != 0;
			q.quad.SetRect( r.nXShift, r.nYShift, r.nXShift + r.nXSize, r.nYShift + r.nYSize );
		}
	}
}

void MakeSData( CObjectInfo::SData *pRes, const CObjectInfo &src )
{
	pRes->geometry = src.GetGeometry();
	const std::vector<CVec3> &pos = src.GetPositions();
	const std::vector<SUVInfo> &verts = src.GetVertices();
	const std::vector<SRealVertexWeight> &weights = src.GetWeights();
	const std::vector<WORD> &posIndices = src.GetPositionIndices();
	pRes->verts.resize( verts.size() );
	for ( int k = 0; k < verts.size(); ++k )
	{
		SVertex &v = pRes->verts[k];
		const SUVInfo &uv = verts[k];
		v.pos = pos[ posIndices[ k ] ];
		v.normal = uv.normal;
		v.texU = uv.texU;
		v.texV = uv.texV;
		v.tex = NGfx::GetTexCoords( uv.tex );
	}
	pRes->weights.resize( verts.size() );
	if( weights.empty() )return;
	for ( int k = 0; k < verts.size(); ++k )
	{
		const SRealVertexWeight &w = weights[posIndices[k]];
		SVertexWeight &vw = pRes->weights[k];
		ASSERT( sizeof( vw.cBoneIndices ) == sizeof( w.cBoneIndices ) );
		ASSERT( sizeof( vw.fWeights ) == sizeof( w.fWeights ) );
		memcpy( vw.cBoneIndices, w.cBoneIndices, sizeof(vw.cBoneIndices) );
		memcpy( vw.fWeights, w.fWeights, sizeof(vw.fWeights) );
	}
}
}

