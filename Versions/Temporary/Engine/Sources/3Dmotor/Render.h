#pragma once

#include <boost/config.hpp>

inline int GetBits( const float *f ) { return *(const int*)f; }

struct SGradientMatrix
{
	float _11, _12, _13;
	float _21, _22, _23;
	float _d1, _d2, _d3;
};

inline void CalcGradient( const SGradientMatrix &m, CVec4 *pRes, float f1, float f2, float f3 )
{
	pRes->x = m._11 * f1 + m._12 * f2 + m._13 * f3;
	pRes->y = m._21 * f1 + m._22 * f2 + m._23 * f3;
	pRes->z = 0;
	pRes->w = m._d1 * f1 + m._d2 * f2 + m._d3 * f3;
}

inline void PrepareGradientMatrix( SGradientMatrix *pRes, const CVec3 &vA, const CVec3 &vB, const CVec3 &vC )
{
	float f1 = vB.x * vC.y - vC.x * vB.y;
	float f2 = vC.x * vA.y - vA.x * vC.y;
	float f3 = vA.x * vB.y - vB.x * vA.y;
	float fD = f1 + f2 + f3;
	float fD1 = 1 / fD;
	memset( pRes, 0, sizeof(*pRes ) );
	pRes->_11 = ( vB.y - vC.y ) * fD1;
	pRes->_12 = -( vA.y - vC.y ) * fD1;
	pRes->_13 = ( vA.y - vB.y ) * fD1;
	pRes->_21 = -( vB.x - vC.x ) * fD1;
	pRes->_22 = ( vA.x - vC.x ) * fD1;
	pRes->_23 = -( vA.x - vB.x ) * fD1;
	pRes->_d1 = f1 * fD1;
	pRes->_d2 = f2 * fD1;
	pRes->_d3 = f3 * fD1;
}

struct SProjectedPoint
{
	CVec3 res, src;

	void Project()
	{
		res.z = 1 / src.z;
		res.x = src.x * res.z;
		res.y = src.y * res.z;
	}
	void Transform( const SHMatrix &xform, const CVec3 &_src )
	{
		src.x = xform._11 * _src.x + xform._12 * _src.y + xform._13 * _src.z + xform._14;
		src.y = xform._21 * _src.x + xform._22 * _src.y + xform._23 * _src.z + xform._24;
		src.z = xform._41 * _src.x + xform._42 * _src.y + xform._43 * _src.z + xform._44;
		Project();
	}
};

// all points assumed to be projected & scaled to fit viewport
// rasterizer perform perspective division only
// rasterizers tries to conform DirectX rasterizing standard - points are checked by its centers
const float F_RASTERIZER_NEAR_PLANE = 0.01f;
const int N_F_RASTERIZER_NEAR_PLANE = 0x3c23d70a;//0.01f//*(int*)&F_RASTERIZER_NEAR_PLANE;
template<class T>
class CRasterizer
{
	struct SEdgeInfo
	{
		int nSY, nFY;
		float fDX, fX0;
	};
	struct SZGradientInfo
	{
		float fZ0, fZx, fZy;
	};
	// sLeft - single edge, sRight - upper edge, sRight2 - lower edge; [nSY;nFY2) - upper edge; [nFY2;nFY) - lower edge
	void RasterTriangleLow( const SEdgeInfo &sLeft, const SEdgeInfo &sRight, const SEdgeInfo &sRight2, 
		const SZGradientInfo &zGradient, int nSY, int nFY2, int nFY, int nBack )
	{
		ASSERT( sLeft.nSY == nSY && sLeft.nFY == nFY );
		ASSERT( sRight.nSY == nSY );
		ASSERT( (sRight.nFY == nFY && nFY2 == nFY) || (sRight.nFY == nFY2 && sRight2.nSY == nFY2 && sRight2.nFY == nFY ) );
		T *pThis = static_cast<T*>( this );
		pThis->ClipVertical( &nSY, &nFY2, &nFY );
		float fY = nSY + 0.5f;
		float fZBase = zGradient.fZ0 + fY * zGradient.fZy;// + 0.5f * zGradient.fZx; // added in CalcZGradient
		float fLeftX = sLeft.fX0 + sLeft.fDX * fY;
		float fRightX = sRight.fX0 + sRight.fDX * fY;
		int nY = nSY;
		for ( ; nY < nFY2; ++nY, fY += 1, fLeftX += sLeft.fDX, fRightX += sRight.fDX, fZBase += zGradient.fZy )
		{
			int nLeft = Float2Int( fLeftX );//+ 0.49999f );
			int nRight = Float2Int( fRightX );//+ 0.49999f );
			int nBackface;
			if ( nLeft > nRight )
			{
				std::swap( nLeft, nRight );
				nBackface = nBack^1;
			}
			else if ( nRight > nLeft )
				nBackface = nBack;
			else
				continue;
			if ( nBackface && !pThis->DoRenderBackface() )
				continue;
			pThis->ClipHorizontal( &nLeft, &nRight );
			
			float fZ = fZBase + nLeft * zGradient.fZx;
			pThis->RasterSpan( nY, nLeft, nRight, fZ, zGradient.fZx, nBackface ); // right exclusive
		}

		fRightX = sRight2.fX0 + sRight2.fDX * fY;
		for ( ; nY < nFY; ++nY, fY += 1, fLeftX += sLeft.fDX, fRightX += sRight2.fDX, fZBase += zGradient.fZy )
		{
			int nLeft = Float2Int( fLeftX );//+ 0.49999f );
			int nRight = Float2Int( fRightX );//+ 0.49999f );
			int nBackface;
			if ( nLeft > nRight )
			{
				std::swap( nLeft, nRight );
				nBackface = nBack^1;
			}
			else if ( nRight > nLeft )
				nBackface = nBack;
			else
				continue;
			if ( nBackface && !pThis->DoRenderBackface() )
				continue;
			pThis->ClipHorizontal( &nLeft, &nRight );

			float fZ = fZBase + nLeft * zGradient.fZx;
			pThis->RasterSpan( nY, nLeft, nRight, fZ, zGradient.fZx, nBackface ); // right exclusive
		}
	}
	void InitEdge( SEdgeInfo *pRes, const CVec3 &a, const CVec3 &dif, int nSY, int nFY )
	{
		float fY1 = 1 / dif.y;
		pRes->fDX = dif.x * fY1;
		pRes->fX0 = a.x - (a.y) * pRes->fDX;
		pRes->nSY = nSY;
		pRes->nFY = nFY;
	}

	BOOST_FORCEINLINE bool CalcZGradient( SZGradientInfo *pInfo, const CVec3 &vA, const CVec3 &vCB, const CVec3 &vAC )
	{
		float fArea = -vAC.x * vCB.y + vAC.y * vCB.x;
		if ( fArea == 0 )
			return false;
		float fD = 1 / fArea;
		pInfo->fZx = fD * (-vCB.y * vAC.z + vAC.y * vCB.z );
		pInfo->fZy = fD * ( vCB.x * vAC.z - vAC.x * vCB.z );
		pInfo->fZ0 = vA.z - (vA.x-0.5f) * pInfo->fZx - vA.y * pInfo->fZy;
		return true;
	}
	void RasterTriangle( const CVec3 &vA, const CVec3 &vB, const CVec3 &vC )
	{
		int nA = Float2Int( vA.y );//+ 0.49999f );
		int nB = Float2Int( vB.y );//+ 0.49999f );
		int nC = Float2Int( vC.y );//+ 0.49999f );
		if ( nA == nB && nA == nC )
			return;
		//int nTestAx = RealFloat2Int( vA.x );
		//if ( nTestAx == RealFloat2Int( vB.x ) && nTestAx == RealFloat2Int( vC.x ) )
		//	return;
		int nLeft = 0, nRight = 0;
		CVec3 vEdge1( vB - vA ), vEdge2( vC - vB ), vEdge3( vA - vC );
		SZGradientInfo zGrad;
		if ( !CalcZGradient( &zGrad, vA, vEdge2, vEdge3 ) )
			return;
		SEdgeInfo sLeft[2], sRight[2];
		
		if ( vEdge1.y > 0 )
			InitEdge( &sLeft[nLeft++], vA, vEdge1, nA, nB );
		else if ( vEdge1.y < 0 )
			InitEdge( &sRight[nRight++], vB, vEdge1, nB, nA );
		
		if ( vEdge2.y > 0 )
			InitEdge( &sLeft[nLeft++], vB, vEdge2, nB, nC );
		else if ( vEdge2.y < 0 )
			InitEdge( &sRight[nRight++], vC, vEdge2, nC, nB );
		
		if ( vEdge3.y > 0 )
			InitEdge( &sLeft[nLeft++], vC, vEdge3, nC, nA );
		else if ( vEdge3.y < 0 )
			InitEdge( &sRight[nRight++], vA, vEdge3, nA, nC );
		
		SEdgeInfo *pLeft, *pRight;
		int nEdges, nBack;
		if ( nLeft == 1 )
		{
			pLeft = sLeft;
			pRight = sRight;
			nEdges = nRight;
			nBack = 0;
		}
		else if ( nRight == 1 )
		{
			pLeft = sRight;
			pRight = sLeft;
			nEdges = nLeft;
			nBack = 1;
		}
		else
			return;
		if ( nEdges > 1 )
		{
			if ( pRight[0].nSY > pRight[1].nSY )
				RasterTriangleLow( pLeft[0], pRight[1], pRight[0], zGrad, pLeft[0].nSY, pRight[1].nFY, pLeft[0].nFY, nBack );
			else
				RasterTriangleLow( pLeft[0], pRight[0], pRight[1], zGrad, pLeft[0].nSY, pRight[0].nFY, pLeft[0].nFY, nBack );
		}
		else if ( nEdges > 0 )
			RasterTriangleLow( pLeft[0], pRight[0], pRight[0], zGrad, pLeft[0].nSY, pRight[0].nFY, pLeft[0].nFY, nBack );
	}
	void Intersect( SProjectedPoint *pRes, const SProjectedPoint &vA, const SProjectedPoint &vB )
	{
		float ffA = vA.src.z - F_RASTERIZER_NEAR_PLANE;// - vA.w;
		float ffB = vB.src.z - F_RASTERIZER_NEAR_PLANE;// - vB.w;
		float fKoef1 = 1 / (ffB - ffA), fA = fKoef1 * ffB, fB = -fKoef1 * ffA;
		pRes->src.x = vA.src.x * fA + vB.src.x * fB;
		pRes->src.y = vA.src.y * fA + vB.src.y * fB;
		pRes->src.z = vA.src.z * fA + vB.src.z * fB;
		pRes->Project();
	}
	void RenderClipped1( const SProjectedPoint &v1, const SProjectedPoint &v2, const SProjectedPoint &v3 )
	{
		SProjectedPoint vI12;
		Intersect( &vI12, v1, v2 );
		SProjectedPoint vI13;
		Intersect( &vI13, v1, v3 );
		RasterTriangle( v1.res, vI12.res, vI13.res );
	}
	void RenderClipped2( const SProjectedPoint &v1, const SProjectedPoint &v2, const SProjectedPoint &v3 )
	{
		SProjectedPoint vI13;
		Intersect( &vI13, v1, v3 );
		SProjectedPoint vI23;
		Intersect( &vI23, v2, v3 );
		RasterTriangle( v1.res, vI23.res, vI13.res );
		RasterTriangle( v1.res, v2.res, vI23.res );
	}
	void RasterClipped( const SProjectedPoint &v1, const SProjectedPoint &v2, const SProjectedPoint &v3, int nTemp )
	{
		ASSERT( nTemp < 8 );
		switch ( nTemp )
		{
		case 1:
			RenderClipped2( v1, v2, v3 );
			break;
		case 2:
			RenderClipped2( v3, v1, v2 );
			break;
		case 3:
			RenderClipped1( v1, v2, v3 );
			break;
		case 4:
			RenderClipped2( v2, v3, v1 );
			break;
		case 5:
			RenderClipped1( v2, v3, v1 );
			break;
		case 6:
			RenderClipped1( v3, v1, v2 );
			break;
		case 7:
			break;
		default:
			// nTemp is a three bit mask and Raster only reaches here for 1 to 7, so the
			// seven cases above are exhaustive. Boost's spelling of this is __assume(0)
			// on MSVC and __builtin_unreachable() elsewhere, and carries its own
			// semicolon; boost/config.hpp is already included at the top of this file.
			BOOST_UNREACHABLE_RETURN()
			break;
		}
	}
public:
	void Raster( const SProjectedPoint &v1, const SProjectedPoint &v2, const SProjectedPoint &v3 )
	{
		unsigned int nTemp;
		nTemp = ( GetBits( &v1.src.z ) < N_F_RASTERIZER_NEAR_PLANE ) * 4;//( v1.z > v1.w || v1.z < -v1.w ) * 4;
		nTemp |= ( GetBits( &v2.src.z ) < N_F_RASTERIZER_NEAR_PLANE ) * 2;//( v2.z > v2.w || v2.z < -v2.w ) * 2;
		nTemp |= ( GetBits( &v3.src.z ) < N_F_RASTERIZER_NEAR_PLANE ) * 1;//( v3.z > v3.w || v3.z < -v3.w ) * 1;
		//nTemp = ( v1.src.z < F_RASTERIZER_NEAR_PLANE ) * 4;//( v1.z > v1.w || v1.z < -v1.w ) * 4;
		//nTemp |= ( v2.src.z < F_RASTERIZER_NEAR_PLANE ) * 2;//( v2.z > v2.w || v2.z < -v2.w ) * 2;
		//nTemp |= ( v3.src.z < F_RASTERIZER_NEAR_PLANE ) * 1;//( v3.z > v3.w || v3.z < -v3.w ) * 1;
		if ( nTemp == 0 )
			RasterTriangle( v1.res, v2.res, v3.res );
		else
			RasterClipped( v1, v2, v3, nTemp );
	}
	void RasterNoClip( const CVec3 &v1, const CVec3 &v2, const CVec3 &v3 )
	{
		RasterTriangle( v1, v2, v3 );
	}
};

// special mapping to conform DirectX mapping standard
struct STextureMapping
{
	CVec4 ptDU, ptDV;

	void SetGradient( const CVec4 &srcU, const CVec4 &srcV )
	{
		ptDU.x = srcU.x; ptDU.y = srcU.y; ptDU.w = srcU.w + ( srcU.x + srcU.y ) * 0.5f;// - 0.5f;
		ptDV.x = srcV.x; ptDV.y = srcV.y; ptDV.w = srcV.w + ( srcV.x + srcV.y ) * 0.5f;// - 0.5f;
	}
};

template<class T, class TElement>
class CArrayRasterizer: public CRasterizer<T>
{
public:
	CArray2D<TElement> res;
	//! set region with exclusive upper borders
	void SetRegion( const CTRect<int> &_region )
	{
		region = _region;
		res.SetSizes( region.Width(), region.Height() );
	}
	void SetTextureMapping( const CVec4 &_ptDU, const CVec4 &_ptDV )
	{
		texMapping.SetGradient( _ptDU, _ptDV );
	}
protected:
	CTRect<int> region; // with exclusive borders
	STextureMapping texMapping;
	
	void ClipVertical( int *pnSY, int *pnFY2, int *pnFY )
	{
		(*pnSY) = (std::max)( *pnSY, region.y1 );
		(*pnFY) = (std::min)( *pnFY, region.y2 );
		(*pnFY2) = (std::min)( *pnFY2, region.y2 );
	}
	void ClipHorizontal( int *pnSX, int *pnFX )
	{
		(*pnSX) = (std::max)( *pnSX, region.x1 );
		(*pnFX) = (std::min)( *pnFX, region.x2 );
	}
};


