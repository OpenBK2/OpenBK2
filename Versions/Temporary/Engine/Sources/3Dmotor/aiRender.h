#pragma once
#include "3Dmotor_export.h"


#include "Misc/2Darray.h"
#include "aiInterval.h"
#include "3DLib/Transform.h"
#include "Render.h"
#include "Misc/Pool.h"

namespace NAI
{

const float F_INF = 1e30f;
const int N_F_INF = 0x7149f2ca;//1e30f//*(const int*)&F_INF;
struct SConvexHull;
class _3DMOTOR_EXPORT CFastRenderer: public CRasterizer<CFastRenderer>
{
public:	
	struct SResult
	{
		union
		{
			float fDist[2];
			struct 
			{
				float fEnter, fExit;
			};
		};
		CPtr<CObjectBase> pObject;
		SResult *pNext;
	};

	CArray2D<SResult*> resGrid;

	int nTraceFrame;
	CArray2D<int> gridFrames;

	CFastRenderer();
	bool TestSphere( const CVec3 &ptCenter, float fR ) { return ts.IsIn( SSphere( ptCenter, fR ) ); }
	void GetPoints( std::vector<CVec3> *pEnters, std::vector<CVec3> *pExits ) const;
	void GetPoints( std::vector<CVec3> *pEnters, std::vector<CVec3> *pExits, int x, int y ) const;
	void GetDir( CVec3 *pRes, float x, float y ) const;
	void GetCoordsClamped( const CVec3 &v, float *pX, float *pY );
	//! Angle in radians; region is inclusive
	void Init( const CTransformStack &transformStack, int nHalfSize );
	void InitParallel( const CVec2 &_ptOrigin, float fAngle, float fStep, const CTRect<int> &region );
	void InitParallel( const SHMatrix &cameraPos, float fHalfSize, int nHalfSize );
	//! fAngle is FOV in degrees
	void InitProjective( const SHMatrix &cameraPos, float fDistance, float fAngle, int nHalfSize, float fAspect = 1.0f );
	void InitProjective( const CVec3 &src, const CVec3 &dst, const CVec2 &halfSquare, int nHalfSize );
	void InitProjective( const CVec3 &src, const CVec3 &dst, float fHalfSquare, int nHalfSize );
	void InitSingleRay( const CVec3 &src, const CVec3 &dst );
	void TraceEntity( const std::vector<SConvexHull> &e, bool bTerrain );
	void TraceEntity( const SConvexHull &e, bool bTerrain );
	void SortIntervals();
	void ReduceTerrain();

private:
	CTRect<int> region; // with exclusive borders
	CPtr<CObjectBase> pCurrentObject;
	SHMatrix transform, backForPoints;
	CTransformStack ts; // for bounding volume tests
	bool bPerspective, bUseInvertOrder;
	float fPerPixelShiftX, fPerPixelShiftY;
	CVec3 ptFrom;
	CArray2D<float> distMult;
	typedef CPool<SResult> TPool;
	TPool res;
	TPool::SIterator objectStart;

	bool DoRenderBackface() const { return true; }

	void ClipVertical( int *pnY )
	{
		(*pnY) = (std::max)( *pnY, region.y1 );
		(*pnY) = (std::min)( *pnY, region.y2-1 );
	}

	void ClipVertical( int *pnSY, int *pnFY2, int *pnFY )
	{
		(*pnSY) = (std::max)( *pnSY, region.y1 );
		(*pnFY2) = (std::min)( *pnFY2, region.y2 );
		(*pnFY) = (std::min)( *pnFY, region.y2 );
	}
	void ClipHorizontal( int *pnSX, int *pnFX )
	{
		(*pnSX) = (std::max)( *pnSX, region.x1 );
		(*pnFX) = (std::min)( *pnFX, region.x2 );
	}
	SResult* AddResult()
	{
		SResult *pRes = res.Alloc();
		pRes->fDist[0] = F_INF;
		pRes->fDist[1] = F_INF;
		//pRes->nSourceIdx = nSourceIdx;
		pRes->pObject = 0;
		return pRes;
	}
	void RasterSpan( int nY, int nLeft, int nRight, float fZ, float fDZ, int nBackface )
	{
		int y = nY - region.y1;
		SResult **pRow = &resGrid[y][0] - region.x1;
		SResult **p = pRow + nLeft, **pFinal = pRow + nRight;
		float *pDistMul = &distMult[y][0] + ( nLeft - region.x1 );
		int *pFrames = &gridFrames[y][0] - region.x1 + nLeft;
		for (; p < pFinal; ++p, fZ += fDZ, ++pDistMul, ++pFrames )
		{
			if ( *pFrames == nTraceFrame )
				continue;

			*pFrames = nTraceFrame;

			float fDist = fZ;
			if ( fZ < FP_EPSILON )
				return;

			if ( bPerspective )
				fDist = pDistMul[0] / fZ;

			SResult **pResGrid = p;
			SResult *pResult = *pResGrid;
			SResult *pNode = AddResult();
			pNode->pObject = pCurrentObject;
			pNode->fDist[0] = fDist;
			pNode->pNext = 0;
			

			if ( pResult == 0 )
			{
				*pResGrid = pNode;
			}
			else
			{
				if ( pResult->fDist[0] > fDist )
				{
					*pResGrid = pNode;
					pNode->pNext = pResult;
				}
				else
				{
					while ( pResult->pNext && pResult->pNext->fDist[0] > fDist )
						pResult = pResult->pNext;

					pNode->pNext = pResult->pNext;
					pResult->pNext = pNode;
				}
			}
		}
	}

	void SetRegion( const CTRect<int> &region );
	void RealTraceEntity( const SConvexHull &e );
	void CalcDistMul();
	void SetSource( const SSourceInfo *_pSrc, int _nUserID );
	void ConvertResults( bool bTerrain );
	int operator&( CStructureSaver &f ) { ASSERT(0); return 0; }

	friend class CRasterizer<CFastRenderer>;
};

}


