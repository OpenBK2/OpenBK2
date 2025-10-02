#include "StdAfx.h"
#include "aiRender.h"
#include "aiObject.h"

namespace NAI
{

inline CVec3 Unhomogen( const CVec4 &v ) { return CVec3( v.x / v.w, v.y / v.w, v.z / v.w ); }

// CFastRenderer

CFastRenderer::CFastRenderer() : bUseInvertOrder(false), bPerspective(false)
{
}

void CFastRenderer::SetRegion( const CTRect<int> &_region )
{
	region = _region;
	resGrid.SetSizes( region.Width(), region.Height() );
	resGrid.FillEvery( 0 );
	res.Clear();

	gridFrames.SetSizes( region.Width(), region.Height() );

	pCurrentObject = 0;
}

void CFastRenderer::GetDir( CVec3 *pRes, float x, float y ) const
{
	ASSERT( bPerspective );
	CVec3 base( fPerPixelShiftX * (x+0.5f+region.x1), fPerPixelShiftY * (y+0.5f+region.y1), 1 );
	Normalize( &base );
	CVec4 res;
	backForPoints.RotateHVector( &res, base );
	*pRes = Unhomogen( res ) - ptFrom;
	//CVec4 res;
	//backForPoints.RotateHVector( &res, base );
	//*pRes = Unhomogen(res) - ptFrom;
	Normalize( pRes );
}

void CFastRenderer::GetCoordsClamped( const CVec3 &v, float *pX, float *pY )
{
	CVec4 vRes;
	transform.RotateHVector( &vRes, v );
	if ( bPerspective )
	{
		vRes.x /= vRes.w;
		vRes.y /= vRes.w;
	}
	*pX = Clamp( vRes.x - 0.5f, (float)region.x1, region.x2 - 2.001f ) - region.x1;
	*pY = Clamp( vRes.y - 0.5f, (float)region.y1, region.y2 - 2.001f ) - region.y1;
}

void CFastRenderer::GetPoints( vector<CVec3> *pEnters, vector<CVec3> *pExits, int x, int y ) const
{
	for( SResult *p = resGrid[y][x]; p; p = p->pNext )
	{
		SResult &interval = *p;
		float fEnter = interval.fEnter;
		float fExit = interval.fExit;
		if ( bPerspective )
		{
			CVec3 base( fPerPixelShiftX * (x+0.5f+region.x1), fPerPixelShiftY * (y+0.5f+region.y1), 1 );
			Normalize( &base );
			CVec4 res;
			backForPoints.RotateHVector( &res, base * fEnter );
			pEnters->push_back( Unhomogen(res) );
			backForPoints.RotateHVector( &res, base * fExit );
			pExits->push_back( Unhomogen(res) );
			//ASSERT( fabs( fEnter - fabs( pEnters->back() - ptFrom ) ) < 1e-3f );
			//CVec3 vDir;
			//GetDir( &vDir, x, y );
			//ASSERT( fabs( pEnters->back() - ( ptFrom + vDir * p->fEnter ) ) < 1e-4f );
		}
		else
		{
			CVec3 base, res;
			backForPoints.RotateHVector( &base, CVec3( 0.5f + x + region.x1, 0.5f + y + region.y1, 0 ) );
			backForPoints.RotateVector( &res, CVec3( 0, 0, fEnter ) );
			pEnters->push_back( base + res );
			backForPoints.RotateVector( &res, CVec3( 0, 0, fExit ) );
			pExits->push_back( base + res );
		}
	}
}

void CFastRenderer::GetPoints( vector<CVec3> *pEnters, vector<CVec3> *pExits ) const
{
	pEnters->resize( 0 );
	pExits->resize( 0 );
	for ( int y = 0; y < resGrid.GetSizeY(); ++y )
	{
		for ( int x = 0; x < resGrid.GetSizeX(); ++x )
			GetPoints( pEnters, pExits, x, y );
	}
}

void CFastRenderer::CalcDistMul()
{
	distMult.SetSizes( region.Width(), region.Height() );
	if ( bPerspective )
	{
		//SHMatrix invProj;
		//InvertMatrix( &invProj, ts.GetProjection().forward );
		//float f1M = invProj._33, f1A = invProj._34;
		//float f2M = invProj._43, f2A = invProj._44;
		for ( int y = 0; y < distMult.GetSizeY(); ++y )
		{
			for ( int x = 0; x < distMult.GetSizeX(); ++x )
			{
				float fDistMul = sqrt( 1 + 
					sqr( ( x + region.x1 + 0.5f ) * fPerPixelShiftX ) +
					sqr( ( y + region.y1 + 0.5f ) * fPerPixelShiftY ) );
				distMult[y][x] = fDistMul;
				//float fDepth = ( f1A + f1M * f ) / ( f2A + f2M * f );
			}
		}
	}
	else
		distMult.FillEvery( 1 );
}

void CFastRenderer::Init( const CTransformStack &transformStack, int nHalfSize )
{
	ptFrom = transformStack.Get().forward.GetTranslation();
	ts = transformStack;
	transform = ts.Get().forward;

	transform.x *= nHalfSize;
	transform.y *= nHalfSize;

	CTRect<int> region( -nHalfSize, -nHalfSize, nHalfSize, nHalfSize );
	SetRegion( region );

	fPerPixelShiftX = fabs( transformStack.GetProjection().backward.x ) / nHalfSize;
	fPerPixelShiftY = fabs( transformStack.GetProjection().backward.y ) / nHalfSize; 
	bPerspective = true;
	CalcDistMul();
	backForPoints = ts.Get().backward * ts.GetProjection().forward;
}

void CFastRenderer::InitParallel( const CVec2 &_ptOrigin, float fAngle, float fStep, const CTRect<int> &region )
{
	ptFrom = CVec3(_ptOrigin, 0 );
	// fill projection info
	SFBTransform t;
	MakeMatrix( &t, CVec3(fStep, fStep, 1), CVec3(_ptOrigin.x,_ptOrigin.y,0), fAngle );
	transform = t.backward;
	// shift by 0.5 pixel
	transform.x += transform.w * 0.499f; // should be 0.5f, but until terrain patches are not shifted this is required
	transform.y += transform.w * 0.499f;
	// init ts
	ts.MakeParallel( region.Width() + 0.01f, region.Height() + 0.01f );
	SHMatrix cam;
	CVec2 ptCenter( _ptOrigin.x, _ptOrigin.y );
	CVec2 ptXDir( cos(fAngle) * fStep, sin(fAngle) * fStep );
	ptCenter += ptXDir * ( (region.x1 + region.x2) * 0.5f );
	ptCenter += CVec2(-ptXDir.y, ptXDir.x) * ( (region.y1 + region.y2) * 0.5f );
	MakeMatrix( &cam, FP_PI2, fAngle, 0, CVec3( ptCenter.x, ptCenter.y, 0 ) );
	ts.SetCamera( cam );
	// setup grids
	SetRegion( CTRect<int>( region.x1, region.y1, region.x2 + 1, region.y2 + 1 ) );
	//grid.SetSizes( region.Width() + 1, region.Height() + 1 );
	//origin.x = region.x1;
	//origin.y = region.y1;
	bPerspective = false;
	CalcDistMul();

	InvertMatrix( &backForPoints, transform );
	bUseInvertOrder = true;
}

void CFastRenderer::InitParallel( const SHMatrix &cameraPos, float fHalfSize, int nHalfSize )
{
	ptFrom = cameraPos.GetTranslation();
	// init ts
	ts.MakeParallel( fHalfSize * 2, fHalfSize * 2, -5000, 5000 );
	ts.SetCamera( cameraPos );
	transform = ts.Get().forward;
	transform.x *= nHalfSize;
	transform.y *= nHalfSize;
	transform.z = transform.z * 10000 - transform.w * 5000;
	// setup grids
	SetRegion( CTRect<int>( -nHalfSize, -nHalfSize, nHalfSize, nHalfSize ) );
	bPerspective = false;
	CalcDistMul();
	InvertMatrix( &backForPoints, transform );
	bUseInvertOrder = false;
}

void CFastRenderer::InitProjective( const SHMatrix &cameraPos, float fDistance, float fAngle, int nHalfSize, float fAspect )
{
	ptFrom = cameraPos.GetTranslation();
	ts.MakeProjective( fAspect, fAngle, 0.1f, fDistance );
	ts.SetCamera( cameraPos );
	transform = ts.Get().forward;
	CTRect<int> region( -nHalfSize, -nHalfSize, nHalfSize, nHalfSize );
	transform.x *= nHalfSize;
	transform.y *= nHalfSize;
	//
	SetRegion( region );
	//grid.SetSizes( region.Width(), region.Height() );
	//origin = CTPoint<int>( -nHalfSize, -nHalfSize );

	fPerPixelShiftX = tan( ToRadian(fAngle) * 0.5f ) / nHalfSize;
	fPerPixelShiftY = fPerPixelShiftX * fAspect;
	bPerspective = true;
	CalcDistMul();
	backForPoints = ts.Get().backward * ts.GetProjection().forward;
}

void CFastRenderer::InitProjective( const CVec3 &src, const CVec3 &dst, float fHalfSquare, int nHalfSize )
{
	SHMatrix m;
	CVec3 ptDir = dst - src;
	MakeMatrix( &m, src, dst - src );
	float fDistance = fabs( ptDir ) + 0.1f;
	float fAngle = 2 * ToDegree( atan2( fHalfSquare, fDistance ) );
	InitProjective( m, fDistance, fAngle, nHalfSize );
}

void CFastRenderer::InitProjective( const CVec3 &src, const CVec3 &dst, const CVec2 &halfSquare, int nHalfSize )
{
	SHMatrix m;
	CVec3 ptDir = dst - src;
	MakeMatrix( &m, src, dst - src );
	float fDistance = fabs( ptDir ) + 0.1f;
	float fAngle = 2 * ToDegree( atan2( halfSquare.x, fDistance ) );
	InitProjective( m, fDistance, fAngle, nHalfSize, halfSquare.y / halfSquare.x );
}

void CFastRenderer::InitSingleRay( const CVec3 &src, const CVec3 &dst )
{
	ptFrom = src;//CVec3(_ptOrigin, 0 );
	SHMatrix cam;
	CVec3 ptDir = dst - src;
	MakeMatrix( &cam, src, dst - src );
	float fDist = fabs( ptDir );
	ts.MakeParallel( 1, 1, 0, fDist );//0.01f, 0.01f );
	ts.SetCamera( cam );

	transform = ts.Get().forward;
	transform.x += transform.w * 0.5f;
	transform.y += transform.w * 0.5f;
	transform.z *= fDist;

	// setup grids
	SetRegion( CTRect<int>( 0, 0, 1, 1 ) );
	bPerspective = false;
	CalcDistMul();

	InvertMatrix( &backForPoints, transform );
	bUseInvertOrder = false;
}

void CFastRenderer::RealTraceEntity( const SConvexHull &e )
{
	++nTraceFrame;

	// project points
	static vector<SProjectedPoint> projected;
	static vector<CVec3> flatProjected;
	//vector<CVec4> projectedP;
	if ( bPerspective )
	{
		if ( e.points.size() > projected.size() )
			projected.resize( e.points.size() );
		//projectedP.resize( e.points.size() );
		SHMatrix xform;
		Multiply( &xform, transform, e.trans );
		CVec2 vMins( FP_MAX_VALUE, FP_MAX_VALUE );
		CVec2 vMaxs( -FP_MAX_VALUE, -FP_MAX_VALUE );
		for ( int i = 0; i < e.points.size(); ++i )
		{
			const CVec3 &src = e.points[i];
			SProjectedPoint &dst = projected[i];
			dst.Transform( xform, src );
			vMins.Minimize( CVec2( dst.res.x, dst.res.y ) );
			vMaxs.Maximize( CVec2( dst.res.x, dst.res.y ) );
		}
		if ( vMaxs.x < vMins.x + 1.0f || vMaxs.y < vMins.y + 1.0f )
		{
			// To force to raster the object
			CVec2 vMiddle = ( vMins+vMaxs ) / 2.0f;
			for ( int i = 0; i < e.points.size(); ++i )
			{
				SProjectedPoint &dst = projected[i];
				if ( dst.res.x < vMiddle.x )
					dst.res.x -= 1.0f;
				else
					dst.res.x += 1.0f;

				if ( dst.res.y < vMiddle.y )
					dst.res.y -= 1.0f;
				else
					dst.res.y += 1.0f;
			}
		}
	}
	else
	{
		if ( e.points.size() > flatProjected.size() )
			flatProjected.resize( e.points.size() );
		//projected.resize( e.points.size() );
		SHMatrix xform;
		Multiply( &xform, transform, e.trans );
		for ( int i = 0; i < e.points.size(); ++i )
		{
			const CVec3 &src = e.points[i];
			CVec3 &dst = flatProjected[i];
			xform.RotateHVector( &dst, src );
		}
	}
	// rasterize every triangle
	const vector<SEdge> &edges = e.tris.edges;
	const vector<STriangle> &mesh = e.tris.mesh;
	for ( int i = 0; i < mesh.size(); ++i )
	{
		int i1, i2, i3;
		const STriangle &t = mesh[i];
		if ( t.i1 & 0x8000 )
		{
			i1 = edges[ t.i1 & 0x7fff ].wFinish; 
			i2 = edges[ t.i1 & 0x7fff ].wStart;
		}
		else
		{
			i1 = edges[ t.i1 & 0x7fff ].wStart; 
			i2 = edges[ t.i1 & 0x7fff ].wFinish;
		}
		if ( t.i2 & 0x8000 )
			i3 = edges[ t.i2 & 0x7fff ].wStart;
		else
			i3 = edges[ t.i2 & 0x7fff ].wFinish;
		//
		if ( bPerspective )
		{
			if ( (projected[i3].res.y-projected[i1].res.y)*(projected[i2].res.x-projected[i1].res.x) > (projected[i2].res.y-projected[i1].res.y)*(projected[i3].res.x-projected[i1].res.x) )
				Raster( projected[i1], projected[i3], projected[i2] );
		}
		else
		{
			if ( bUseInvertOrder )
				RasterNoClip( flatProjected[i1], flatProjected[i2], flatProjected[i3] );
			else
				RasterNoClip( flatProjected[i1], flatProjected[i3], flatProjected[i2] );
		}
	}
}

void CFastRenderer::SetSource( const SSourceInfo *_pSrc, int _nUserID ) 
{
	pCurrentObject = _pSrc->pUserData;
	objectStart = TPool::SIterator( &res );
}

void CFastRenderer::ConvertResults( bool bTerrain )
{
	return;
	/*
	if ( bTerrain )
	{
		for ( TPool::SIterator i( &res ); i != objectStart; --i )
		{
			SResult &r = *i.p;
			if ( r.pObject )
				continue;
			// find first node in chain
			SResult *pFirst = i.p;
			for ( SResult *pTest = pFirst; pTest && pTest->pObject == 0; pTest = pTest->pNext )
				pFirst = pTest;
			// look first node - if enter is NAN - set to inf
			if ( pFirst->fDist[0] == F_INF )
				pFirst->fDist[0] = -1e10f;
			// if enter > exit
			if ( pFirst->fDist[0] > pFirst->fDist[1] )
			{
				// add new node in front
				SResult *pNew = AddResult();
				pNew->pNext = pFirst->pNext;
				pNew->fDist[0] = bPerspective ? 0 : -1e10f;
				pFirst->pNext = pNew;
				// shift exits down
				SResult *pIdx = i.p;
				float fExit = r.fDist[1];
				for(;;)
				{
					pIdx = pIdx->pNext;
					if ( pIdx == 0 || pIdx->pObject != 0 )
						break;
					swap( fExit, pIdx->fDist[1] );
				}
				r.fDist[1] = 1e10f;
			}
			// look last node - if exit is NAN - set to inf
			if ( r.fDist[1] == F_INF )
			{
				if ( r.fDist[0] == F_INF )
				{
					ASSERT( r.pNext );
					if ( r.pNext )
						r = *r.pNext;
				}
				else
					r.fDist[1] = 1e10f;
			}
			// assert that every intersection has positive depth and set source
			for ( SResult *p = i.p; p != 0 && p->pObject == 0; p = p->pNext )
			{
				ASSERT( p->fDist[0] <= p->fDist[1] );
				p->pObject = pCurrentObject;
			}
		}
	}
	else
	{
		for ( TPool::SIterator i( &res ); i != objectStart; --i )
		{
			SResult &r = *i.p;
			if ( r.pObject )
				continue;
			if ( bPerspective )
			{
				// insert zeros to the start
				while ( r.fDist[0] == F_INF )
				{
					float *pPrev = &r.fDist[0];
					for( SResult *pIdx = i.p; ; )
					{
						pIdx = pIdx->pNext;
						if ( !pIdx || pIdx->pObject )
							break;
						*pPrev = pIdx->fDist[0];
						pPrev = &pIdx->fDist[0];
					}
					*pPrev = 0;
				}
			}
			// number of entrences should be equal to number of exits
//			ASSERT( r.fDist[0] != F_INF && r.fDist[1] != F_INF );
			// recheck every intersection and set sources for a chain
			for ( SResult *p = i.p; p != 0 && p->pObject == 0; p = p->pNext )
			{
				p->fDist[1] = Max( p->fDist[0], p->fDist[1] );
				p->pObject = pCurrentObject;
			}
		}
	}
	*/
}

static bool WalkChain( CFastRenderer::SResult **p )
{
	if ( !*p )
		return false;
	bool bRet = false;
	for(;;)
	{
		CFastRenderer::SResult *pCur = *p;
		CFastRenderer::SResult *pTest = pCur->pNext;
		if ( !pTest )
			break;
		if ( pCur->fEnter > pTest->fEnter )
		{
			pCur->pNext = pTest->pNext;
			pTest->pNext = pCur;
			*p = pTest;
			bRet = true;
		}
		p = &(*p)->pNext;
	}
	return bRet;
}

void CFastRenderer::SortIntervals()
{
	return;
	for ( int y = 0; y < resGrid.GetSizeY(); ++y )
	{
		for ( int x = 0; x < resGrid.GetSizeX(); ++x )
		{
			while ( WalkChain( &resGrid[y][x] ) ) ;
		}
	}
}

void CFastRenderer::ReduceTerrain()
{
	for ( int y = 0; y < resGrid.GetSizeY(); ++y )
	{
		for ( int x = 0; x < resGrid.GetSizeX(); ++x )
		{
			for( CFastRenderer::SResult *p = resGrid[y][x]; p; p = p->pNext )
			{
				if ( p->pObject == 0 ) // terrain
					p->fEnter = p->fExit - 0.01f;
			}
		}
	}
}

void CFastRenderer::TraceEntity( const vector<SConvexHull> &hulls, bool bTerrain )
{
}

void CFastRenderer::TraceEntity( const SConvexHull &e, bool bTerrain )
{
	SetSource( &e.src, e.nUserID );
	ASSERT( bTerrain || e.tris.bClosed );
	RealTraceEntity( e );
	if ( objectStart == TPool::SIterator( &res ) )
		int aaa = 0;
}

}
