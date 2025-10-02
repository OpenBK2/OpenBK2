#include "StdAfx.h"
#include "..\3Dlib\Transform.h"
#include "GCombiner.h"
#include "GShadowVolume.h"
#include "Render.h"
//#include "GMaterial.hpp"
//#include "GScene.h"
//#include "GSceneUtils.h"
//#include "MemObject.h"
//#include "GSceneInternal.h"
//#include "GMemBuilder.h"
//#include "GMemFormat.h"
//extern CGScene *pCurrentScene;

namespace NGScene
{

const int N_OCCLUDE_BUFFER_WIDTH = 128;
const int N_OCCLUDE_BUFFER_HEIGHT = 128;
const float F_ZBUF_SCALE = 40000;
const float F_ZBUF_SCALE_LOW = 10000;
typedef unsigned short zbuf_type;

static CVec3 vCamDirs[6] = 
{
	CVec3( 1, 0, 0 ),
	CVec3( 0, 1, 0 ),
	CVec3( 0, 0, 1 ),
	CVec3(-1, 0, 0 ),
	CVec3( 0,-1, 0 ),
	CVec3( 0, 0,-1 )
};
static vector< CObj<CObjectBase> > nodes;

const zbuf_type N_HZ_MAX = 65535;
class CHZBuffer : public IHZBuffer
{
	OBJECT_BASIC_METHODS(CHZBuffer);
	vector<CArray2D<zbuf_type> > depthBuffer;
	float fZBufScale;
public:
	void Initialize( int nXSize, int nYSize, float _fZBufScale )
	{
		fZBufScale = _fZBufScale;
		int nSize = Max( nXSize, nYSize );
		int nLevels = 0;
		while ( (nSize>>nLevels) > 4 )
			++nLevels;
		depthBuffer.resize( nLevels + 1 );
		depthBuffer[0].SetSizes( nXSize, nYSize );
		for ( int k = 1; k <= nLevels; ++k )
		{
			int nX = ( nXSize + (1<<k) - 1 ) >> k;
			int nY = ( nYSize + (1<<k) - 1 ) >> k;
			depthBuffer[k].SetSizes( nX, nY );
		}
	}
	CArray2D<zbuf_type>& GetDepthBuffer() { return depthBuffer[0]; }
	void BuildHZ()
	{
		for ( int k = 1; k < depthBuffer.size(); ++k )
		{
			const CArray2D<zbuf_type> &src = depthBuffer[ k - 1 ];
			CArray2D<zbuf_type> &dst = depthBuffer[ k ];
			int nXSize = Min( src.GetSizeX() / 2, dst.GetSizeX() );
			int nYSize = Min( src.GetSizeY() / 2, dst.GetSizeY() );
			int nMask1 = 0xffff, nMask2 = 0xffff0000;
			_asm
			{
				movd mm6, nMask1
				movd mm7, nMask2
			}
			for ( int y = 0; y < nYSize; ++y )
			{
				int nShift = src.GetSizeX() * 2;
				const void *pSrc = &src[y*2][0];
				void *pDst = &dst[y][0];
				__asm
				{
					mov esi, pSrc
					mov edi, pDst
					mov ebx, nShift
					mov ecx, nXSize
					shr ecx, 1
lp:
					movq mm0, [esi]
					movq mm1, [esi+ebx]
					add esi, 8
					add edi, 4
					dec ecx
					movq mm2, mm0
					psubusw mm2, mm1
					psubw mm0, mm2
					movq mm1, mm0
					psrl mm0, 16
					movq mm2, mm0
					psubusw mm2, mm1
					psubw mm0, mm2
					movq mm1, mm0
					psrl mm0, 16
					pand mm1, mm6
					pand mm0, mm7
					por mm0, mm1
					movd [edi-4], mm0
					jnz lp
					mov ecx, nXSize
					test ecx, 1
					jz fin
					movd mm0, [esi]
					movd mm1, [esi+ebx]
					movq mm2, mm0
					psubusw mm2, mm1
					psubw mm0, mm2
					movq mm1, mm0
					psrl mm0, 16
					movq mm2, mm0
					psubusw mm2, mm1
					psubw mm0, mm2
					movd eax, mm0
					mov [edi], ax
fin:
				}
					//for ( int x = 0; x < nXSize; ++x )
					//{
					//dst[y][x] = Min(
					//	src[y*2  ][x*2  ], Min(
					//	src[y*2  ][x*2+1], Min(
					//	src[y*2+1][x*2  ], 
					//	src[y*2+1][x*2+1] ) ) );
					//}
			}
			_asm emms
			if ( nYSize * 2 < src.GetSizeY() )
			{
				ASSERT( nYSize * 2 + 1 == src.GetSizeY() );
				for ( int y = nYSize; y < dst.GetSizeY(); ++y )
				{
					for ( int x = 0; x < nXSize; ++x )
						dst[y][x] = Min(
							src[y*2][x*2  ],
							src[y*2][x*2+1] );
				}
			}
			else
			{
				for ( int y = nYSize; y < dst.GetSizeY(); ++y )
				{
					for ( int x = 0; x < nXSize; ++x )
						dst[y][x] = N_HZ_MAX;
				}
			}
			if ( nXSize * 2 < src.GetSizeX() )
			{
				ASSERT( nXSize * 2 + 1 == src.GetSizeX() );
				for ( int x = nXSize; x < dst.GetSizeX(); ++x )
				{
					for ( int y = 0; y < nYSize; ++y )
						dst[y][x] = Min(
							src[y*2  ][x*2],
							src[y*2+1][x*2] );
				}
			}
			else
			{
				for ( int x = nXSize; x < dst.GetSizeX(); ++x )
				{
					for ( int y = 0; y < nYSize; ++y )
						dst[y][x] = N_HZ_MAX;
				}
			}
			for ( int x = nXSize; x < dst.GetSizeX(); ++x )
			{
				for ( int y = nYSize; y < dst.GetSizeY(); ++y )
					dst[y][x] = N_HZ_MAX;
			}
		}
	}
	bool IsVisible( const CTRect<float> &r, zbuf_type nCheck ) const
	{
		int nX1 = Float2Int( ( r.x1 + 1 ) * 0.5f * depthBuffer[0].GetSizeX() - 0.5f );
		int nX2 = Float2Int( ( r.x2 + 1 ) * 0.5f * depthBuffer[0].GetSizeX() - 0.5f );
		int nY1 = Float2Int( ( r.y1 + 1 ) * 0.5f * depthBuffer[0].GetSizeY() - 0.5f );
		int nY2 = Float2Int( ( r.y2 + 1 ) * 0.5f * depthBuffer[0].GetSizeY() - 0.5f );
		int nMax = Max( nX2 - nX1, nY2 - nY1 ), nLevel = 0;
		while ( (nMax>>nLevel) > 3 && nLevel < depthBuffer.size() - 1 )
			++nLevel;
		const CArray2D<zbuf_type> &l = depthBuffer[nLevel];
		nX1 = Max( (nX1 >> nLevel)-0, 0 );
		nX2 = Min( (nX2 >> nLevel)+0, l.GetSizeX() - 1 );
		nY1 = Max( (nY1 >> nLevel)-0, 0 );
		nY2 = Min( (nY2 >> nLevel)+0, l.GetSizeY() - 1 );
		for ( int y = nY1; y <= nY2; ++y )
		{
			for ( int x = nX1; x <= nX2; ++x )
			{
				if ( nCheck > l[y][x] )
					return true;
			}
		}
		return false;
	}
	bool IsVisible( const SSphere &s, CTransformStack *pTS ) const
	{
		CTRect<float> r;
		if ( !pTS->GetCoverRect( &r, s.ptCenter, s.fRadius ) )
			return false;
		const SHMatrix &m = pTS->Get().forward;
		const CVec3 &v = s.ptCenter;
		float fDist = m.wx * v.x + m.wy * v.y + m.wz * v.z + m.ww;
		if ( fDist < s.fRadius )
			return true;
		float fCheck = fZBufScale / ( fDist - s.fRadius );
		if ( fCheck > N_HZ_MAX )
			return true;
		return IsVisible( r, fCheck );
	}
	void SetBaseZ( int x, int y, zbuf_type n ) { depthBuffer[0][y][x] = n; }
};

// Visible part generator

int __forceinline Float2IntScale( const float fpVar, const float fpScale )
{
	int nRet;
	__asm 
	{
		fld dword ptr fpVar
		fmul dword ptr fpScale
		fistp nRet
	}
	return nRet;
}
class CPartsRender;
class CPartsRender: public CRasterizer<CPartsRender>
{
	zbuf_type *pDepthBufferBase;
	unsigned short *pIndexBufferBase;
	int nWidth, nHeight;
	int nCurrentID;
	float fZBufScale;
	CObj<CHZBuffer> pHZBuffer;
	CArray2D<unsigned short> indexBuffer;
	vector<int> refCount;
	
	bool DoRenderBackface() const { return false; }

	void ClipVertical( int *pnY )
	{
		(*pnY) = Max( *pnY, 0 );
		(*pnY) = Min( *pnY, nHeight-1 );
	}

	void ClipVertical( int *pnSY, int *pnFY2, int *pnFY )
	{
		(*pnSY) = Max( *pnSY, 0 );
		(*pnFY2) = Min( *pnFY2, nHeight );//N_OCCLUDE_BUFFER_HEIGHT );
		(*pnFY) = Min( *pnFY, nHeight );//N_OCCLUDE_BUFFER_HEIGHT );
	}
	void ClipHorizontal( int *pnSX, int *pnFX )
	{
		(*pnSX) = Max( *pnSX, 0 );
		(*pnFX) = Min( *pnFX, nWidth );//N_OCCLUDE_BUFFER_WIDTH );
	}
	__forceinline void RasterSpan( int nY, int nLeft, int nRight, float fZ, float fDZ, int nBackface )
	{
		ASSERT( pDepthBufferBase == &pHZBuffer->GetDepthBuffer()[0][0] );
		ASSERT( pIndexBufferBase = &indexBuffer[0][0] );
		ASSERT( nY >= 0 && nY < pHZBuffer->GetDepthBuffer().GetSizeY() );
		ASSERT( nLeft >= 0 && nRight <= pHZBuffer->GetDepthBuffer().GetSizeX() );
		int nShift = nY * nWidth;
		zbuf_type *pRow = pDepthBufferBase + nShift, *pElem = pRow + nLeft, *pFinal = pRow + nRight;
		unsigned short *pIndexElem = pIndexBufferBase + nShift + nLeft;
		int nZ = Float2IntScale( fZ, fZBufScale ), nDZ = Float2IntScale( fDZ, fZBufScale );
		int *pRefCount = &refCount[0];
		int nCurID = nCurrentID;
		for ( ; pElem < pFinal; ++pElem, ++pIndexElem )
		{
			ASSERT( nZ >= 0 );
			if ( nZ > 0xffff || nZ > *pElem )
			{
				--pRefCount[ *pIndexElem ];
				++pRefCount[ nCurID ];
				*pIndexElem = nCurID;
				*pElem = nZ;
			}
			nZ += nDZ;
		}
	}
public:
	CPartsRender( int _nWidth, int _nHeight, float _fZBufScale ): nWidth(_nWidth), nHeight(_nHeight), fZBufScale(_fZBufScale)
	{
		pHZBuffer = new CHZBuffer;
		pHZBuffer->Initialize( nWidth, nHeight, fZBufScale );
		indexBuffer.SetSizes( nWidth, nHeight );
		pDepthBufferBase = &pHZBuffer->GetDepthBuffer()[0][0];
		pIndexBufferBase = &indexBuffer[0][0];
	}
	int GetWidth() const { return nWidth; }
	int GetHeight() const { return nHeight; }
	void InitZBuffer( const SHMatrix &proj, float fRadius )
	{
		CArray2D<zbuf_type> &depthBuffer = pHZBuffer->GetDepthBuffer();
		indexBuffer.FillZero();
		//float fZMul = proj._33, fZAdd = proj._34;
		//float fWMul = proj._43, fWAdd = proj._44;
		for ( int nY = 0; nY < nHeight; ++nY )
		{
			float fY = ( nY + 0.5f - nHeight * 0.5f ) * (2.0f / nHeight);
			for ( int nX = 0; nX < nWidth; ++nX )
			{
				float fX = ( nX + 0.5f - nWidth * 0.5f ) * (2.0f / nWidth);
				float fLeng = sqrt( sqr(fX) + sqr(fY) + 1 );
				float fZ = fRadius / fLeng;
				float fZt = 1 / fZ;//( fZAdd + fZMul * fZ ) / ( fWAdd + fWMul * fZ );
				int nZValue = Float2IntScale( fZt, fZBufScale );
				nZValue = Min( nZValue, 65535 );
				depthBuffer[nY][nX] = nZValue;//Float2Int( fZt * fZBufScale );//fZt;//
			}
		}
	}
	void FastInitZBuffer()
	{
		CArray2D<zbuf_type> &depthBuffer = pHZBuffer->GetDepthBuffer();
		depthBuffer.FillZero();
		indexBuffer.FillZero();
	}
	void SetCurrentID( int n ) { nCurrentID = n; }
	void InitRefs( int n ) { refCount.resize( 0 ); refCount.resize( n, 0 ); }
	int GetRefs( int n ) { return refCount[n]; }
	CHZBuffer* GetHZBuffer() const { return pHZBuffer; }
	friend class CRasterizer<CPartsRender>;
};

static int CountParts( const list<SRenderPartSet> &l )
{
	int nRes = 0;
	for ( list<SRenderPartSet>::const_iterator i = l.begin(); i != l.end(); ++i )
		nRes += i->pParts->size();
	return nRes;
}

struct SCompareRPS
{
	CVec3 vZ;
	SCompareRPS( const CVec4 &_vZ ) : vZ(_vZ.x, _vZ.y, _vZ.z) {}
	bool operator()( const SRenderPartSet &a, const SRenderPartSet &b ) const
	{
		if ( a.nFloorMask > b.nFloorMask )
			return true;
		if ( a.nFloorMask == b.nFloorMask )
			return a.pGeometry->pVertices->GetBound().s.ptCenter * vZ < b.pGeometry->pVertices->GetBound().s.ptCenter * vZ;
		return false;
	}
};
static void RenderStuff( CPartsRender &pr, IRender *pRender, CTransformStack *pTS, list<SRenderPartSet> *pListParts,
	float fDensityLimit, float fMaxR2, CHZBuffer *pHZOptimize )
{
	list<SRenderPartSet> &listParts = *pListParts;
	SHMatrix sRes;
	sRes = pTS->Get().forward;
	sRes.x = ( sRes.x * 0.5f + sRes.w * 0.5f ) * pr.GetWidth();
	sRes.y = ( sRes.y * 0.5f + sRes.w * 0.5f ) * pr.GetHeight();
	listParts.sort( SCompareRPS(sRes.w) );

	pr.InitRefs( CountParts( listParts ) + 1 );
	int nIDCounter = 0;
	int nPrevFloorMask = 0;
	bool bFirstPass = true;
	for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
	{
		SRenderPartSet &rps = *i;

		if ( rps.nFloorMask != nPrevFloorMask )
		{
			if ( pHZOptimize )
				pHZOptimize->BuildHZ();
			nPrevFloorMask = rps.nFloorMask;
			bFirstPass = false;
		}

		const vector<SSphere> &bounds = rps.pGeometry->pVertices->GetBounds();
		for ( int nPart = 0; nPart < rps.pParts->size(); ++nPart )
		{
			pr.SetCurrentID( ++nIDCounter );
			
			if ( !rps.parts.IsSet( nPart ) )
				continue;
			if ( !rps.opaque.IsSet( nPart ) )
				continue;
			IPart *pPart = rps.GetPart( nPart );
			if ( !IsValid( pPart ) )
				continue;
			float fCheck = pPart->fAverageTriArea;
			if ( fCheck < fDensityLimit )
			{
				rps.opaque.Reset( nPart );
				continue;
			}
			if ( fMaxR2 > 0 && fabs2( pPart->vBVMax - pPart->vBVMin ) < fMaxR2 )
			{
				rps.opaque.Reset( nPart );
				continue;
			}

			if ( pHZOptimize && !bFirstPass )
			{
				if ( !pHZOptimize->IsVisible( bounds[nPart], pTS ) )
					continue;
			}
			
			vector<CVec3> points;
			vector<STriangle> tris;
			TransformPart( pPart, &points, &tris );
			
			static vector<SProjectedPoint> verticesSet;
			if ( points.size() > verticesSet.size() )
				verticesSet.resize( points.size() );
			for ( int nVert = 0; nVert < points.size(); nVert++ )
			{
				const CVec3 &src = points[nVert];
				verticesSet[nVert].Transform( sRes, src );
			}
			
			for ( int i = 0; i < tris.size(); ++i )
			{
				const STriangle &tri = tris[i];
				const SProjectedPoint &v1 = verticesSet[ tri.i1 ];
				const SProjectedPoint &v2 = verticesSet[ tri.i2 ];
				const SProjectedPoint &v3 = verticesSet[ tri.i3 ];
				pr.Raster( v1, v3, v2 ); // reverse triangle order because directX is using negative Oy direction
			}
		}
	}
}

void GeneratePartList( IRender *pRender, const CVec3 &vCenter, float fRadius, 
	list<SRenderPartSet> *pRes, IRender::EDepthType eType, const SGroupSelect &mask )
{
	CPartsRender pr( N_OCCLUDE_BUFFER_WIDTH, N_OCCLUDE_BUFFER_HEIGHT, F_ZBUF_SCALE_LOW );
	nodes.clear();

	for ( int nTemp = 0; nTemp < 6; nTemp++ )
	{
		SHMatrix cameraTransf;
		MakeMatrix( &cameraTransf, vCenter, vCamDirs[nTemp] );

		CTransformStack sTransform;
		sTransform.MakeProjective( CVec2( N_OCCLUDE_BUFFER_WIDTH, N_OCCLUDE_BUFFER_HEIGHT ), 90, 0.1f, fRadius * 2.0f );
		sTransform.SetCamera( cameraTransf );
	
		list<SRenderPartSet> listParts;
		pRender->FormPartList( &sTransform, &listParts, eType, mask );
		pr.InitZBuffer( sTransform.GetProjection().forward, fRadius );
		RenderStuff( pr, pRender, &sTransform, &listParts, 0, 0, 0 );

		CObj<CHZBuffer> pHZ = pr.GetHZBuffer();
		pHZ->BuildHZ();

		int nID = 0;
		for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
		{
			SRenderPartSet &rps = *i, *pDst = 0;
			for ( list<SRenderPartSet>::iterator k = pRes->begin(); k != pRes->end(); ++k )
			{
				if ( k->pNode == rps.pNode )
				{
					pDst = &(*k);
					break;
				}
			}
			if ( !pDst )
				pDst = &*pRes->insert( pRes->end(), SRenderPartSet( rps.pNode, rps.pParts, rps.pGeometry, rps.nFloorMask ) );
			const vector<SSphere> &bounds = rps.pGeometry->pVertices->GetBounds();
			for ( int k = 0; k < rps.pParts->size(); ++k )
			{
				nID++;
				bool bIsVisible = false;
				if ( rps.parts.IsSet(k) && !rps.opaque.IsSet( k ) )
				{
					float fDist = fabs( bounds[k].ptCenter - vCenter ) - bounds[k].fRadius;
					if ( fDist < 0.5f * fRadius )
						bIsVisible = pHZ->IsVisible( bounds[k], &sTransform );
				}
				bIsVisible |= pr.GetRefs( nID ) != 0;
				if ( bIsVisible )
					pDst->parts.Set( k );
			}
		}
		//if ( nTemp == 0 )
		//{
		//	nodes.clear();
		//	SGroupInfo sInfo( 0, 0xFEFFFFFF );
		//	CPtr<IMaterial> pMaterial = CreateMaterial( CVec3( 1, 0.3f, 0.3f ) );
		//	
		//	CTransformStack sStack;
		//	sStack.Init();
		//	CPtr<CCFBTransform> pTransform = new CCFBTransform( sStack.Get() );
		//	
		//	for( int nTempX = 0; nTempX < pr.depthBuffer.GetSizeX(); nTempX++ )
		//	{
		//		for( int nTempY = 0; nTempY < pr.depthBuffer.GetSizeY(); nTempY++ )
		//		{
		//			if ( pr.depthBuffer[nTempY][nTempX].nIndex != 0 )
		//			{
		//				CVec4 vRes, vSrc( 
		//					( ( nTempX + 0.5f ) / pr.GetWidth() - 0.5f ) * 2, 
		//					( ( nTempY + 0.5f ) / pr.GetHeight() - 0.5f ) * 2, 
		//					pr.depthBuffer[nTempY][nTempX].fDepth, 1 );
		//				pTS->Get().backward.RotateHVector( &vRes, vSrc );
		//				ASSERT( vRes.w != 0 );
		//				CVec3 vPos( vRes.x / vRes.w, vRes.y / vRes.w, vRes.z / vRes.w );
		//				CObj<CMemObject> pModelBuilder( new CMemObject );
		//				pModelBuilder->CreateSphere( vPos, 0.06f, 0 );
		//				
		//				nodes.push_back( pCurrentScene->CreatePart( CreateObjectInfo( pModelBuilder, CVec4(1,1,1,1) ), pMaterial, pTransform, sInfo ) );
		//				//nodes.push_back( pCurrentScene->CreatePart( modelBuilder.CreateObjectInfo(), pMaterial, pTransform, sInfo ) );
		//			}
		//		}
		//	}
		//}
	}
}

void MakeInvisibleElementsList( IRender *pRender, CTransformStack *pTS, 
	const SGroupSelect &_mask, const CVec2 &screenSize, CIgnorePartsHash *pIgnore,
	CObj<IHZBuffer> *pHZBuffer )
{
	CPartsRender pr( Clamp( (int)screenSize.x / 2, 4, 400 ), Clamp( (int)screenSize.y / 2, 4, 300 ), F_ZBUF_SCALE );
	list<SRenderPartSet> listParts;
	pRender->FormPartList( pTS, &listParts,IRender::DT_STATIC, _mask );
	pr.FastInitZBuffer();
	CHZBuffer *pHZ = pr.GetHZBuffer();

	RenderStuff( pr, pRender, pTS, &listParts, 0, 0.5f * 0.5f, pHZ );
	pHZ->BuildHZ();
	*pHZBuffer = pHZ;

	int nID = 0;
	for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
	{
		SRenderPartSet &rps = *i;
		CIgnorePartsHash::iterator res = pIgnore->end();
		const vector<SSphere> &bounds = rps.pGeometry->pVertices->GetBounds();
		for ( int k = 0; k < rps.pParts->size(); ++k )
		{
			++nID;
			//if ( !rps.opaque.IsSet(k) )
			//	continue;
			//if ( !rps.parts.IsSet( k ) )
			//	continue;
			bool bIsVisible = false;
			if ( rps.parts.IsSet(k) && !rps.opaque.IsSet( k ) )
				bIsVisible = pHZ->IsVisible( bounds[k], pTS );
			bIsVisible |= pr.GetRefs( nID ) != 0;

			if ( !bIsVisible )
			{
				if ( res == pIgnore->end() )
				{
					(*pIgnore)[ rps.pNode.GetPtr() ].Clear();
					res = pIgnore->find( rps.pNode.GetPtr() );
				}
				res->second.Set( k );
			}
		}
	}
}

void MakeInvisibleElementsListFast( IRender *pRender, CTransformStack *pTS, 
	const SGroupSelect &_mask, const CVec2 &screenSize, CIgnorePartsHash *pIgnore, 
	CObj<IHZBuffer> *pHZBuffer )
{
	CPartsRender pr( Clamp( (int)screenSize.x / 2, 4, 400 ), Clamp( (int)screenSize.y / 2, 4, 300 ), F_ZBUF_SCALE );
	list<SRenderPartSet> listParts;
	pRender->FormPartList( pTS, &listParts,IRender::DT_STATIC, _mask );
	pr.FastInitZBuffer();
	CHZBuffer *pHZ = pr.GetHZBuffer();

	RenderStuff( pr, pRender, pTS, &listParts, 0.19f, 0.5f * 0.5f, pHZ );
	pHZ->BuildHZ();
	*pHZBuffer = pHZ;

	int nID = 0;
	for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
	{
		SRenderPartSet &rps = *i;
		CIgnorePartsHash::iterator res = pIgnore->end();
		const vector<SSphere> &bounds = rps.pGeometry->pVertices->GetBounds();
		for ( int k = 0; k < rps.pParts->size(); ++k )
		{
			++nID;
			//if ( !rps.opaque.IsSet(k) )
			//	continue;
			//if ( !rps.parts.IsSet( k ) )
			//	continue;
			bool bIsVisible = false;
			if ( rps.parts.IsSet(k) && !rps.opaque.IsSet( k ) )
				bIsVisible = pHZ->IsVisible( bounds[k], pTS );
			bIsVisible |= pr.GetRefs( nID ) != 0;

			if ( !bIsVisible )
			{
				if ( res == pIgnore->end() )
				{
					(*pIgnore)[ rps.pNode.GetPtr() ].Clear();
					res = pIgnore->find( rps.pNode.GetPtr() );
				}
				res->second.Set( k );
			}
		}
	}
}

// Shadow volumes generator

class CShadowVolumeBuilder
{
	enum EE
	{
		N_MAX_POINTS = 10
	};
	struct SPoly
	{
		CVec3 points[N_MAX_POINTS];
		int nSize;

		SPoly(): nSize(0) {}
		int GetSize() const { return nSize; }
		bool IsEmpty() const { return GetSize() < 3; }
		void AddVertex( const CVec3 &v ) { ASSERT( nSize < N_MAX_POINTS ); points[nSize++] = v; }
	};

	CVec3 vCenter;
	float fRadius, fHullRadius;
	vector<CVec3> &resPoints;
	vector<STriangle> &resTris;

	typedef hash_map<CVec3, int, SVec3Hash> CPointHash;
	CPointHash pointHash;
	vector<STriangle> tris;

	int AddPoint( const CVec3 &a );
	int AddBackPoint( const CVec3 &a );
	void AddBackTriangle( int nPlane, const SPoly &poly );
	void AddEdge( int n1, int n2 );
	float CalcPointNorm( const CVec3 &p1 );
public:
	CShadowVolumeBuilder( const CVec3 &_vCenter, float _fRadius, vector<CVec3> *_pResPoints, vector<STriangle> *_pResTris )
		: vCenter(_vCenter), fRadius(_fRadius), resPoints(*_pResPoints), resTris(*_pResTris), fHullRadius(_fRadius * FP_SQRT_3)
	{ 
		resPoints.resize( 0 );
		resTris.resize( 0 );
	}
	void AddTriangle( const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 );
	void BuildResult();
	float GetHullRadius() const { return fHullRadius; }
};

int CShadowVolumeBuilder::AddPoint( const CVec3 &a )
{
	CPointHash::iterator i = pointHash.find( a );
	if ( i != pointHash.end() )
		return i->second;
	int nRes = resPoints.size();
	pointHash[a] = nRes;
	resPoints.push_back( a );
	return nRes;
}

int CShadowVolumeBuilder::AddBackPoint( const CVec3 &a )
{
	CVec3 v = a - vCenter;
	float fLeng = fabs(v.x) + fabs(v.y) + fabs(v.z) + 1e-20f;
	v = vCenter + v * ( fRadius * FP_SQRT_3 / fLeng );
	return AddPoint( v );
}

static void CalcMiddle( CVec3 *pRes, const CVec3 &a, float fA, const CVec3 &b, float fB )
{
	float f1 = 1 / ( fB - fA );
	fA *= f1;
	fB *= f1;
	pRes->x = ( a.x * fB - b.x * fA );
	pRes->y = ( a.y * fB - b.y * fA );
	pRes->z = ( a.z * fB - b.z * fA );
}

void CShadowVolumeBuilder::AddEdge( int n1, int n2 )
{
	int nIndices[10];
	CVec3 points[10];
	points[0] = resPoints[n1];
	points[1] = resPoints[n2];
	int nSize = 2;
	for ( int nPlane = 0; nPlane < 3; ++nPlane )
	{
		for ( int k = 0; k < nSize - 1; ++k )
		{
			float fCur  = points[k  ].m[ nPlane ] - vCenter.m[ nPlane ];
			float fNext = points[k+1].m[ nPlane ] - vCenter.m[ nPlane ];
			float fTest = fCur * fNext;
			if ( fTest < 0 )
			{
				for ( int m = nSize; m > k+1; --m )
					points[m] = points[m-1];
				CalcMiddle( &points[k+1], points[k], fCur, points[k+2], fNext );
				nSize++;
				break;
			}
		}
	}
	nIndices[0] = n2;
	nIndices[1] = n1;
	for ( int k = 0; k < nSize; ++k )
		nIndices[k + 2] = AddBackPoint( points[k] );
	for ( int k = 2; k < nSize + 2; ++k )
		resTris.push_back( STriangle( nIndices[0], nIndices[k-1], nIndices[k] ) );
}

void CShadowVolumeBuilder::AddBackTriangle( int nPlane, const SPoly &poly )
{
	if ( nPlane == 3 )
	{
		int nIndices[ N_MAX_POINTS ];
		// project points onto octahedron
		for ( int k = 0; k < poly.GetSize(); ++k )
			nIndices[k] = AddBackPoint( poly.points[k] );
		for ( int k = 2; k < poly.GetSize(); ++k )
			resTris.push_back( STriangle( nIndices[0], nIndices[k-1], nIndices[k] ) );
		return;
	}
	SPoly pos, neg;
	const CVec3 *pPrev = &poly.points[ poly.GetSize() - 1 ];
	float fPrev = pPrev->m[ nPlane ] - vCenter.m[ nPlane ];
	for ( int k = 0; k < poly.GetSize(); ++k )
	{
		const CVec3 *pCur = &poly.points[ k ];
		float fCur = pCur->m[ nPlane ] - vCenter.m[ nPlane ];
		if ( fCur > 0 )
		{
			if ( fPrev < 0 )
			{
				CVec3 vCenter;
				CalcMiddle( &vCenter, *pCur, fCur, *pPrev, fPrev );
				pos.AddVertex( vCenter );
				neg.AddVertex( vCenter );
			}
			pos.AddVertex( *pCur );
		}
		else if ( fCur < 0 )
		{
			if ( fPrev > 0 )
			{
				CVec3 vCenter;
				CalcMiddle( &vCenter, *pCur, fCur, *pPrev, fPrev );
				pos.AddVertex( vCenter );
				neg.AddVertex( vCenter );
			}
			neg.AddVertex( *pCur );
		}
		else
		{
			pos.AddVertex( *pCur );
			neg.AddVertex( *pCur );
		}
		fPrev = fCur;
		pPrev = pCur;
	}
	if ( !pos.IsEmpty() )
		AddBackTriangle( nPlane + 1, pos );
	if ( !neg.IsEmpty() )
		AddBackTriangle( nPlane + 1, neg );
}

struct SEdge
{
	int nStart, nFinish;

	SEdge( int _nStart, int _nFinish ): nStart(_nStart), nFinish(_nFinish) {}
};
inline bool operator==( const SEdge &a, const SEdge &b ) { return a.nStart == b.nStart && a.nFinish == b.nFinish; }
struct SEdgeHash
{
	int operator()( const SEdge &a ) const { return ( a.nStart << 10 ) ^ a.nFinish; }
};
typedef hash_map<SEdge, int, SEdgeHash> CEdgesHash;

struct SEdgeTracker
{
	CEdgesHash edges;

	void AddEdge( int n1, int n2 )
	{
		SEdge e( n1, n2 ), eBack( n2, n1 );
		CEdgesHash::iterator k = edges.find( eBack );
		if ( k != edges.end() )
		{
			--k->second;
			return;
		}
		CEdgesHash::iterator i = edges.find( e );
		if ( i == edges.end() )
			edges[e] = 1;
		else
			++i->second;
	}
};

void CShadowVolumeBuilder::BuildResult()
{
	// add fronts
	for ( int k = 0; k < tris.size(); ++k )
		resTris.push_back( tris[k] );
	// add back covers
	for ( int k = 0; k < tris.size(); ++k )
	{
		SPoly p;
		const STriangle &t = tris[k];
		p.AddVertex( resPoints[t.i2] );
		p.AddVertex( resPoints[t.i1] );
		p.AddVertex( resPoints[t.i3] );
		AddBackTriangle( 0, p );
	}
	// add edges
	SEdgeTracker edges;
	for ( int k = 0; k < tris.size(); ++k )
	{
		const STriangle &t = tris[k];
		edges.AddEdge( t.i1, t.i2 );
		edges.AddEdge( t.i2, t.i3 );
		edges.AddEdge( t.i3, t.i1 );
	}
	for ( CEdgesHash::iterator i = edges.edges.begin(); i != edges.edges.end(); ++i )
	{
		for ( int k = 0; k < i->second; ++k )
			AddEdge( i->first.nStart, i->first.nFinish );
		for ( int k = -1; k >= i->second; --k )
			AddEdge( i->first.nFinish, i->first.nStart );
	}
}

float CShadowVolumeBuilder::CalcPointNorm( const CVec3 &p1 )
{
	CVec3 v = p1 - vCenter;
	return fabs(v.x) + fabs(v.y) + fabs(v.z);
}

void CShadowVolumeBuilder::AddTriangle( const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 )
{
	CVec3 vNormal( ( p2 - p1 ) ^ ( p3 - p1 ) );
	float fTest = vNormal * vCenter;
	if ( ( vNormal * p1 >= fTest ) && ( vNormal * p2 >= fTest ) && ( vNormal * p3 >= fTest ) )
		return;

	float f1 = CalcPointNorm( p1 );
	float f2 = CalcPointNorm( p2 );
	float f3 = CalcPointNorm( p3 );
	if ( f1 > fRadius * FP_SQRT_3 && f2 > fRadius * FP_SQRT_3 && f3 > fRadius * FP_SQRT_3 )
		return;
	fHullRadius = Max( fHullRadius, f1 );
	fHullRadius = Max( fHullRadius, f2 );
	fHullRadius = Max( fHullRadius, f3 );
	int n1 = AddPoint( p1 );
	int n2 = AddPoint( p2 );
	int n3 = AddPoint( p3 );
	tris.push_back( STriangle( n1, n2, n3 ) );
}


void MakeShadowVolumes( IRender *pRender, CTransformStack *pTS, const CVec3 &vCenter, 
	float fRadius, vector<STriangle> *pTris, 
	vector<CVec3> *pVertices, IRender::EDepthType eType, const SGroupSelect &mask,
	float *pHullRadius,
	CIgnorePartsHash *pIgnore )
{
	CShadowVolumeBuilder shadowBuilder( vCenter, fRadius, pVertices, pTris );
	list<SRenderPartSet> listParts;

	if ( pIgnore )
		pIgnore->clear();
	if ( eType == IRender::DT_STATIC )
	{
		GeneratePartList( pRender, vCenter, fRadius, &listParts, eType, mask );
		if ( pIgnore )
		{
			for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
			{
				SRenderPartSet &rps = *i;
				// if there are skipped parts, fill them with 1
				if ( !rps.parts.IsEmpty() )
				{
					CPartFlags &r = (*pIgnore)[ i->pNode ];
					r = rps.parts;
					r.Invert();
				}
			}
		}
	}
	else
		pRender->FormPartList( pTS, &listParts, eType, mask );

	//int nVertCount = 0;
	//int nCoverTriCount = 0;
	//list<SEdge> partEdges;
	//pList->clear();
	//pVertices->clear();
	//pCoverFaces->clear();
	for ( list<SRenderPartSet>::iterator i = listParts.begin(); i != listParts.end(); i++ )
	{
		SRenderPartSet &rps = *i;
		for ( int k = 0; k < rps.pParts->size(); ++k )
		{
			if ( !rps.parts.IsSet( k ) )
				continue;
			IPart *pPart = rps.GetPart( k );
			if ( !IsValid( pPart ) )
				continue;

			vector<CVec3> points;
			vector<STriangle> tris;
			TransformPart( pPart, &points, &tris );

			for( int nTriIndex = 0; nTriIndex != tris.size(); nTriIndex++ )
			{
				const STriangle &tri = tris[nTriIndex];
				const CVec3 &p1 = points[ tri.i1 ];
				const CVec3 &p2 = points[ tri.i2 ];
				const CVec3 &p3 = points[ tri.i3 ];
				shadowBuilder.AddTriangle( p1, p2, p3 );
			}
		}
	}
	shadowBuilder.BuildResult();
	*pHullRadius = shadowBuilder.GetHullRadius();
}

}
using namespace NGScene;
BASIC_REGISTER_CLASS( CHZBuffer )
