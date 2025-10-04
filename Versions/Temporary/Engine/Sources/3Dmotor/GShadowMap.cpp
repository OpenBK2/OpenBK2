#include "stdafx.h"
#include "GShadowMap.h"
#include "3DLib/Transform.h"
#include "4dCalcs.h"

namespace NGeometry
{

struct SPolygon
{
	vector<CVec4> vertices;
	CVec4 vPlane;
};

struct SPolyhedron
{
	vector<SPolygon> facets;
};

struct SEdge
{
	CVec4 vStart, vEnd;
	SEdge() {}
	SEdge( const CVec4 &_s, const CVec4 &_e ) : vStart(_s), vEnd(_e) {}
};

static void CalcIntersection( CVec4 *p, const CVec4 &_a, float fa, const CVec4 &_b, float fb )
{
	CVec4 a(_a), b(_b);
	float aw = a.w, bw = b.w;
	if ( aw == bw )
	{
		float f1 = 1 / (fb - fa);
		*p = a * (fb*f1) - b * (fa*f1);
		return;
	}
	if ( aw == 0 )
	{
		if ( fa == 0 )
		{
			*p = a;
			return;
		}
		*p = b - (fb / fa) * a;
		return;
	}
	if ( bw == 0 )
	{
		if ( fb == 0 )
		{
			*p = b;
			return;
		}
		*p = a - (fa / fb) * b;
		return;
	}
	a.x *= bw; a.y *= bw; a.z *= bw; a.w *= bw; fa *= bw;
	b.x *= aw; b.y *= aw; b.z *= aw; b.w *= aw; fb *= aw;
	float f1 = 1 / (fb - fa);
	*p = a * (fb*f1) - b * (fa*f1);
	Normalize( p );
}

static void Split( SPolyhedron *pRes, const SPlane &p )
{
	SPolyhedron src = *pRes;
	list<SEdge> edges;
	pRes->facets.resize(0);
	for ( int k = 0; k < src.facets.size(); ++k )
	{
		SPolygon d;
		SPolygon &s = src.facets[k];
		for ( int k = 0; k < s.vertices.size(); ++k )
		{
			if ( s.vertices[k].w < 0 )
				s.vertices[k] = -s.vertices[k];
		}
		int iPrev = s.vertices.size() - 1;
		float fPrevSide = p.vec4 * s.vertices[iPrev], fSide;
		CVec4 vEnter, vExit;
		bool bWasIntersected = false;
		for ( int i = 0; i < s.vertices.size(); iPrev = i, fPrevSide = fSide, ++i )
		{
			const CVec4 &vPrev = s.vertices[ iPrev ];
			const CVec4 &v = s.vertices[ i ];
			fSide = p.vec4 * v;
			if ( fPrevSide < 0 )
			{
				if ( fSide < 0 )
					continue;
				bWasIntersected = true;
				if ( fSide > 0 )
				{
					CalcIntersection( &vEnter, vPrev, fPrevSide, v, fSide );
					d.vertices.push_back( vEnter );
				}
				else
					vEnter = v;
				d.vertices.push_back( v );
			}
			else
			{
				if ( fSide < 0 )
				{
					ASSERT( fPrevSide >= 0 );
					if ( fPrevSide > 0 )
					{
						CalcIntersection( &vExit, v, fSide, vPrev, fPrevSide );
						d.vertices.push_back( vExit );
					}
					else
						vExit = vPrev;
				}
				else
					d.vertices.push_back( v );
			}
		}
		if ( !d.vertices.empty() )
		{
			d.vPlane = s.vPlane;
			pRes->facets.push_back( d );
		}
		if ( bWasIntersected )
			edges.push_back( SEdge( vEnter, vExit ) );
	}
	if ( !edges.empty() )
	{
		SPolygon onPlane;
		onPlane.vertices.push_back( edges.back().vEnd );
		edges.pop_back();
		while ( !edges.empty() )
		{
			CVec4 v = onPlane.vertices.back();
			bool bFound = false;
			for ( list<SEdge>::iterator i = edges.begin(); i != edges.end(); ++i )
			{
				if ( v == i->vStart )
				{
					bFound = true;
					onPlane.vertices.push_back( i->vEnd );
					edges.erase( i );
					break;
				}
			}
			ASSERT( bFound );
			if ( !bFound )
				break;
		}
		onPlane.vPlane = CVec4( -p.vec4.x, -p.vec4.y, -p.vec4.z, p.vec4.w );
		pRes->facets.push_back( onPlane );
	}
}

static void SetPlane( SPolygon *pRes, const CVec4 &a, const CVec4 &b, const CVec4 &c )
{
	CVec3 ab = Get3DDir( b, a ), ac = Get3DDir( c, a );
	//Normalize( &ab );
	//Normalize( &ac );
	CVec3 vNormal = ab ^ ac;
	float fDist = a.x * vNormal.x + a.y * vNormal.y + a.z * vNormal.z;
	if ( a.w < 0 )
		fDist = -fDist;
	pRes->vPlane.Set( vNormal * fabs(a.w), fDist );
	Normalize( &pRes->vPlane );
}

static SPolygon GetTriangle( const CVec4 &a, const CVec4 &b, const CVec4 &c )
{
	SPolygon r;
	r.vertices.push_back( a );
	r.vertices.push_back( b );
	r.vertices.push_back( c );
	SetPlane( &r, a, b, c );
	return r;
}

static SPolygon GetQuad( const CVec4 &a, const CVec4 &b, const CVec4 &c, const CVec4 &d )
{
	SPolygon r;
	r.vertices.push_back( a );
	r.vertices.push_back( b );
	r.vertices.push_back( c );
	r.vertices.push_back( d );
	SetPlane( &r, a, b, c );
	return r;
}


struct SHashedHedron
{
	vector<CVec4> pts;
	struct SEdge
	{
		int nStart, nEnd;
		SEdge() {}
		SEdge( int _s, int _e ) : nStart(_s), nEnd(_e) {}
	};
	struct SPolygon
	{
		vector<int> edges;
		CVec4 vPlane;
	};
	vector<SEdge> edges;
	vector<SPolygon> polys;
};

static int AddPoint( SHashedHedron *pRes, const CVec4 &v )
{
	for ( int k = 0; k < pRes->pts.size(); ++k )
	{
		if ( pRes->pts[k] == v )
			return k;
	}
	pRes->pts.push_back( v );
	return pRes->pts.size() - 1;
}

static int AddEdge( SHashedHedron *pRes, int nStart, int nEnd )
{
	for ( int k = 0; k < pRes->edges.size(); ++k )
	{
		SHashedHedron::SEdge &e = pRes->edges[k];
		if ( e.nStart == nStart && e.nEnd == nEnd )
			return k;
		if ( e.nStart == nEnd && e.nEnd == nStart )
			return k|0x80000000;
	}
	pRes->edges.push_back( SHashedHedron::SEdge( nStart, nEnd ) );
	return pRes->edges.size() - 1;
}

static void Assign( SHashedHedron *pRes, const SPolyhedron &src )
{
	pRes->polys.resize( src.facets.size() );
	for ( int k = 0; k < src.facets.size(); ++k )
	{
		const SPolygon &p = src.facets[k];
		SHashedHedron::SPolygon &d = pRes->polys[k];
		d.vPlane = p.vPlane;
		int nPrev = AddPoint( pRes, p.vertices[ p.vertices.size() - 1 ] ), nTek;
		for ( int i = 0; i < p.vertices.size(); nPrev = nTek, ++i )
		{
			nTek = AddPoint( pRes, p.vertices[i] );
			d.edges.push_back( AddEdge( pRes, nPrev, nTek ) );
		}
	}
}

static bool IsClosed( const SHashedHedron &src )
{
	vector<int> count;
	count.resize( src.edges.size(), 0 );
	for ( int k = 0; k < src.polys.size(); ++k )
	{
		for ( int i = 0; i < src.polys[k].edges.size(); ++i )
		{
			int nEdge = src.polys[k].edges[i];
			if ( nEdge & 0x80000000 )
				--count[ nEdge&0x7fffffff ];
			else
				++count[nEdge];
		}
	}
	bool bRes = true;
	for ( int i = 0; i < count.size(); ++i )
		bRes &= count[i] == 0;
	return bRes;
}

static void Assign( SPolyhedron *pRes, const SHashedHedron &src )
{
	pRes->facets.resize( src.polys.size() );
	for ( int k = 0; k < src.polys.size(); ++k )
	{
		const SHashedHedron::SPolygon &s = src.polys[k];
		SPolygon &d = pRes->facets[k];
		d.vPlane = s.vPlane;
		for ( int i = 0; i < s.edges.size(); ++i )
		{
			int nEdge = s.edges[i];
			if ( nEdge & 0x80000000 )
				d.vertices.push_back( src.pts[ src.edges[ nEdge&0x7fffffff ].nEnd ] );
			else
				d.vertices.push_back( src.pts[ src.edges[ nEdge ].nStart ] );
		}
	}
}

}
namespace NGScene
{

static void MakeShadowOccludersHull( NGeometry::SPolyhedron *pRes, const NGeometry::SPolyhedron &src, const CVec3 &ptDir )
{
	NGeometry::SHashedHedron h;
	NGeometry::Assign( &h, src );
	//ASSERT( NGeometry::IsClosed( h ) );

	vector<int> up( h.polys.size() );
	for ( int k = 0; k < h.polys.size(); ++k )
	{
		const CVec4 vPlane = h.polys[k].vPlane;
		up[k] = vPlane * CVec4(ptDir,0) < 0;
	}

	vector<int> rPolys( h.edges.size(), 0 ), lPolys( h.edges.size(), 0 );
	for ( int k = 0; k < h.polys.size(); ++k )
	{
		NGeometry::SHashedHedron::SPolygon &p = h.polys[k];
		for ( int i = 0; i < p.edges.size(); ++i )
		{
			if ( p.edges[i] & 0x80000000 )
				lPolys[ p.edges[i] & 0x7fffffff ] = k;
			else
				rPolys[ p.edges[i] ] = k;
		}
	}

	CVec4 vInf( -ptDir, 0 );
	for ( int k = 0; k < h.edges.size(); ++k )
	{
		CVec4 vStart = h.pts[ h.edges[k].nStart ];
		CVec4 vEnd = h.pts[ h.edges[k].nEnd ];
		if ( up[ rPolys[k] ] != up[ lPolys[k] ] )
		{
			if ( up[ rPolys[k] ] )
				pRes->facets.push_back( NGeometry::GetTriangle( vStart, vEnd, vInf ) );
			else
				pRes->facets.push_back( NGeometry::GetTriangle( vEnd, vStart, vInf ) );
		}
	}
	for ( int k = 0; k < up.size(); ++k )
	{
		if ( up[k] )
			continue;
		pRes->facets.push_back( src.facets[k] );
	}
	NGeometry::SHashedHedron hTest;
	NGeometry::Assign( &hTest, *pRes );
	//ASSERT( NGeometry::IsClosed( hTest ) );
}

static float Discr( float f, float fStep )
{
	int n = Float2Int( f / fStep );
	return n * fStep;
}
static CVec3 Discr( const CVec3 &v, float fStep )
{
	CVec3 vRes( Discr( v.x, fStep ), Discr( v.y, fStep ), Discr( v.z, fStep ) );
	return vRes;
}

static float Dot4( const CVec4 &a, const CVec4 &b ) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

static float SetBits( float f, int nBits, int nBitVal )
{
	int nMask = ( 1 << nBits ) - 1;
	int nVal = nBitVal ? nMask : 0;
	int nRes = *(int*)&f;
	nRes = ( nRes & ~nMask ) | nVal;
	return *(float*)&nRes;
}

static float CalcNLP( float f )
{
	float fR = 7 + f * f;
	return f * log( fR ) / log( 2.0f ) / sqrt( fR );
}
static void SolveNLP( float fMin, float fMax, float *pfA, float *pfB )
{
	float a = CalcNLP( fMin );
	float b = CalcNLP( fMax );

	float fShiftStep = 1.0 / 512;
	float fMult = ( 1 - fShiftStep ) / ( b - a );
	fMult = SetBits( fMult, 20, 0 );
	*pfA = fMult;
	float fShift = Discr( - *pfA * a + fShiftStep / 2, fShiftStep );
	*pfB = fShift;
}

static void CalcMinMax( const vector<CVec3> &points, const CVec4 &vDir, float *pfMin, float *pfMax )
{
	float fMin = 1e38f, fMax = -1e38f;
	for ( int k = 0; k < points.size(); ++k )
	{
		float f = Dot4( CVec4( points[k], 1 ), vDir );
		fMax = Max( fMax, f );
		fMin = Min( fMin, f );
	}
	*pfMin = fMin;
	*pfMax = fMax;
}
static float CalcRange( const vector<CVec3> &points, const CVec4 &vDir )
{
	float fMin, fMax;
	CalcMinMax( points, vDir, &fMin, &fMax );
	return fMax - fMin;
}

static void DefaultShadowMatrix( SNLProjectionInfo *pRes, CTransformStack *pShadowGeomTS )
{
	*pRes = SNLProjectionInfo();
	pShadowGeomTS->MakeParallel( 1, 1 );
}

struct SAreaCalcInfo
{
	vector<CVec3> points;
	CVec3 vXBase, vYBase, vCamera;
	float fCW;//, fMinimalScale;
};
static float CalcArea( float fRot, const SAreaCalcInfo &info, CVec4 *pX, CVec4 *pY )
{
	float fSin = sin( ToRadian( fRot ) ), fCos = cos( ToRadian( fRot ) ); 

	CVec3 vX, vY;
	vX =  fCos * info.vXBase + fSin * info.vYBase;
	vY = -fSin * info.vXBase + fCos * info.vYBase;

	CVec4 &vTestX = *pX, &vTestY = *pY;
	if ( info.fCW == 0 )
	{
		vTestX = CVec4( vX, 0 );
		vTestY = CVec4( vY, 0 );
	}
	else
	{
		vTestX = CVec4( vX, -info.vCamera * vX );
		vTestY = CVec4( vY, -info.vCamera * vY );
	}

	return CalcRange( info.points, vTestX ) * CalcRange( info.points, vTestY );
}

void MakeShadowMatrix( SNLProjectionInfo *pRes, CTransformStack *pShadowGeomTS, float fMinimalSize,
	const CTransformStack &ts, const CVec3 &vLightDir, float fMaxHeight, const SBound &_sceneBound,
	float fSceneHeight, SShadowMatrixAlign *pOldAlign )
{
	extern bool bNewShadows;

	if ( _sceneBound.s.fRadius == 0 )
	{
		DefaultShadowMatrix( pRes, pShadowGeomTS );
		return;
	}
	CVec4 vC, v00, v01, v10, v11, vC1, vN00, vN01, vN11, vN10;
	ts.Get().backward.RotateHVector( &vC, CVec4(0,0,1,0) );
	ts.Get().backward.RotateHVector( &vN00, CVec4(-1,-1,0,1) );
	ts.Get().backward.RotateHVector( &vN01, CVec4(-1, 1,0,1) );
	ts.Get().backward.RotateHVector( &vN11, CVec4( 1, 1,0,1) );
	ts.Get().backward.RotateHVector( &vN10, CVec4( 1,-1,0,1) );
	ts.Get().backward.RotateHVector( &v00, CVec4(-1,-1,1,1) );
	ts.Get().backward.RotateHVector( &v01, CVec4(-1, 1,1,1) );
	ts.Get().backward.RotateHVector( &v11, CVec4( 1, 1,1,1) );
	ts.Get().backward.RotateHVector( &v10, CVec4( 1,-1,1,1) );
	ts.Get().backward.RotateHVector( &vC1, CVec4( 0, 0,1,1) );


	CVec4 pl1, pl2;
	float val;

	ts.Get().forward.RotateHVectorTransposed( &pl1, CVec4( 0, 0,1, 0) );
	ts.Get().forward.RotateHVectorTransposed( &pl2, CVec4( 0, 0,1,-1) );

	val = 1.0f / sqrtf ( pl1.x * pl1.x + pl1.y * pl1.y + pl1.z * pl1.z ); 
	pl1 *= val;

	val = 1.0f / sqrtf ( pl2.x * pl2.x + pl2.y * pl2.y + pl2.z * pl2.z ); 
	pl2 *= val;

	// occluderHedron - potential shadow casters bound
	// projHedron - hedron used for projection mapping
	NGeometry::SPolyhedron occluderHedron, projHedron;
	{
		// hedron - intersection of scene bound and camera frustrum
		NGeometry::SPolyhedron hedron;

		hedron.facets.push_back( NGeometry::GetQuad( vN10, vN00, v00, v10 ) );
		hedron.facets.push_back( NGeometry::GetQuad( vN11, vN10, v10, v11 ) );
		hedron.facets.push_back( NGeometry::GetQuad( vN01, vN11, v11, v01 ) );
		hedron.facets.push_back( NGeometry::GetQuad( vN00, vN01, v01, v00 ) );
		hedron.facets.push_back( NGeometry::GetQuad( vN00, vN10, vN11, vN01 ) );
		hedron.facets.push_back( NGeometry::GetQuad( v00, v01, v11, v10 ) );

		SBound sceneBound = _sceneBound;
		sceneBound.Extend( 0.5f );
		CVec3 vSceneMin = sceneBound.s.ptCenter - sceneBound.ptHalfBox;
		CVec3 vSceneMax = sceneBound.s.ptCenter + sceneBound.ptHalfBox;
		vSceneMin = Discr( vSceneMin, 0.5 );
		vSceneMax = Discr( vSceneMax, 0.5 );
		NGeometry::Split( &hedron, SPlane( CVec3(0,0, 1), -Max( -0.5f, vSceneMin.z ) ) );
		NGeometry::Split( &hedron, SPlane( CVec3( 1,0,0), -vSceneMin.x ) );
		NGeometry::Split( &hedron, SPlane( CVec3(-1,0,0), vSceneMax.x ) );
		NGeometry::Split( &hedron, SPlane( CVec3(0, 1,0), -vSceneMin.y ) );
		NGeometry::Split( &hedron, SPlane( CVec3(0,-1,0), vSceneMax.y ) );

		if( bNewShadows )
		{
			pl1.w -= 100;
			NGeometry::Split( &hedron, -pl1 );			
		}
		projHedron = hedron;
		NGeometry::Split( &hedron, SPlane( CVec3(0,0,-1), Clamp( vSceneMax.z, 0.5f, fMaxHeight * 1.05f ) ) );
		NGeometry::Split( &projHedron, SPlane( CVec3(0,0,-1), Clamp( fSceneHeight, 0.5f, fMaxHeight * 1.05f ) ) );

		MakeShadowOccludersHull( &occluderHedron, hedron, vLightDir );
	}

	if ( occluderHedron.facets.empty() )
	{
		DefaultShadowMatrix( pRes, pShadowGeomTS );
		return;
	}

	const SHMatrix &mCamera = ts.Get().forward;

	// collect all points
	float fMinW = 1e38f;
	CVec3 vMin( +1e38f, +1e38f, +1e38f ), vMax( -1e38f, -1e38f, -1e38f );

	float fMinX = +1e38f;
	float fMinY = +1e38f;

	float fMaxX = -1e38f;
	float fMaxY = -1e38f;

	CVec3 xDir = vLightDir ^ CVec3(1,0,0);
	Normalize( &xDir );

	CVec3 yDir = vLightDir ^ CVec3(0,1,0);
	Normalize( &yDir );

	hash_map<CVec3,bool, SVec3Hash> pointHash;
	for ( int k = 0; k < projHedron.facets.size(); ++k )
	{
		const NGeometry::SPolygon &p = projHedron.facets[k];
		for ( int i = 0; i < p.vertices.size(); ++i )
		{
			CVec3 vPos = Unhomogen( p.vertices[i] );
			fMinW	= Min( fMinW, Dot4( mCamera.w, CVec4(vPos,1) ) );

			float fX = xDir * vPos;
			float fY = yDir * vPos;

			fMinX = min ( fX, fMinX );
			fMinY = min ( fY, fMinY );

			fMaxX = max ( fX, fMaxX );
			fMaxY = max ( fY, fMaxY );

			vMin.Minimize( vPos );
			vMax.Maximize( vPos );

			pointHash[ vPos ];
		}
	}

	CVec3 vSceneMax = _sceneBound.s.ptCenter + _sceneBound.ptHalfBox;
	CVec3 vSceneMin = _sceneBound.s.ptCenter - _sceneBound.ptHalfBox;

	float dX = fMaxX - fMinX;
	float dY = fMaxY - fMinY;

	static int cnt = 4;
	static float ofMinX = +1e38f;
	static float ofMinY = +1e38f;
	static float ofMaxX = -1e38f;
	static float ofMaxY = -1e38f;
	static CVec4 svDirX;
	static CVec4 svDirY;

	bool bFirstCase = 
	( ( ofMaxX - ofMinX ) > dX * 1.3f ) || ( ( ofMaxY - ofMinY ) > dY * 1.3f );

	bool bSecondCase = 
		!( ofMinX < fMinX && 
		ofMinY < fMinY && 
		ofMaxX > fMaxX &&
		ofMaxY > fMaxY );

	pRes->bNeedUpdate = false;
	if( bFirstCase || bSecondCase || cnt > 0)
	{
		ofMinX =  fMinX - dX * 0.1f;
		ofMaxX =  fMaxX + dX * 0.1f;
		
		ofMinY =  fMinY - dY * 0.1f;
		ofMaxY =  fMaxY + dY * 0.1f;

		pRes->bNeedUpdate = true;
		svDirX = CVec4 ( xDir / (ofMaxX - ofMinX), -ofMinX / (ofMaxX - ofMinX) );
		svDirY = CVec4 ( yDir / (ofMaxY - ofMinY), -ofMinY / (ofMaxY - ofMinY) );

		cnt--;
	}

	pRes->fMaxX = ofMaxX;
	pRes->fMaxY = ofMaxY;
	pRes->fMinX = ofMinX;
	pRes->fMinY = ofMinY;
	pRes->lvShift = CVec4 ( 1, 1, 0, 0 );

	pRes->vDirX = svDirX;
	pRes->vDirY = svDirY;
	
	SAreaCalcInfo areaInfo;
	for ( hash_map<CVec3,bool, SVec3Hash>::iterator i = pointHash.begin(); i != pointHash.end(); ++i )
		areaInfo.points.push_back( i->first );

	// base orientation
	areaInfo.vXBase = vLightDir ^ CVec3(1,0,0);
	if ( fabs2(areaInfo.vXBase) < 0.1f )
		areaInfo.vXBase = vLightDir ^ CVec3(0,1,0);
	Normalize( &areaInfo.vXBase );
	areaInfo.vYBase = -areaInfo.vXBase ^ vLightDir;
	Normalize( &areaInfo.vYBase );

	areaInfo.fCW = vC.w;

	// scaling
	if ( fMinW > fMinimalSize )
		fMinimalSize = fMinW;
	float fProjectionScale = 1 / fMinimalSize; // 0.5// 1 // 0.2
	if ( areaInfo.fCW == 0 )
		fProjectionScale = 1e-5f; // practically linear projection
	fProjectionScale = Max( 1e-5f ,fProjectionScale );

	// select u, v orientation
	if ( areaInfo.fCW != 0 )
	{
		CVec3 v = Unhomogen( vC );
		areaInfo.vCamera.x = Discr( v.x, fMinimalSize * 0.5f );
		areaInfo.vCamera.y = Discr( v.y, fMinimalSize * 0.5f );
		areaInfo.vCamera.z = Discr( v.z, fMinimalSize * 0.5f );
	}

	CVec4 vXBest, vYBest;
	float fPrevArea = CalcArea( pOldAlign->fRotation, areaInfo, &vXBest, &vYBest );
	float fBestArea = fPrevArea;
	for ( float fRot = 0; fRot < 90; fRot += 1 )
	{
		CVec4 vTestX, vTestY;
		float fArea = CalcArea( fRot, areaInfo, &vTestX, &vTestY );
		if ( fArea < fBestArea && fArea < fPrevArea * 0.95f )
		{
			pOldAlign->fRotation = fRot;
			fBestArea = fArea;
			vXBest = vTestX;
			vYBest = vTestY;
		}
	}
	pRes->vTexU = vXBest;
	pRes->vTexV = vYBest;
	//if ( fBestArea != fPrevArea )
	//	DebugTrace( "Changed rotation to %g", pOldAlign->fRotation );

	pRes->vTexU *= fProjectionScale;
	pRes->vTexV *= fProjectionScale;

	// calc projection params
	float fUMin, fUMax, fVMin, fVMax;
	CalcMinMax( areaInfo.points, pRes->vTexU, &fUMin, &fUMax );
	CalcMinMax( areaInfo.points, pRes->vTexV, &fVMin, &fVMax );

	pRes->vMinMax = CVec4( fUMin, fUMax, fVMin, fVMax );
	SolveNLP( fUMin, fUMax, &pRes->vShift.x, &pRes->vShift.z );
	SolveNLP( fVMin, fVMax, &pRes->vShift.y, &pRes->vShift.w );

	// initialize shadow geom bound, Identity projection is used
	pShadowGeomTS->Make();

	

	if( bNewShadows && pRes->bNeedUpdate )
	{
		pShadowGeomTS->AddClipPlane(pRes->vDirX);
		
		pShadowGeomTS->AddClipPlane
			(  
			CVec4( -pRes->vDirX.x, -pRes->vDirX.y, -pRes->vDirX.z, 1.0f - pRes->vDirX.w )
			);

		pShadowGeomTS->AddClipPlane(pRes->vDirY);
	}
	else
	for ( int k = 0; k < occluderHedron.facets.size(); ++k )
	{
		const CVec4 &vPlane = occluderHedron.facets[k].vPlane;
		CVec3 v( vPlane.x, vPlane.y, vPlane.z );
		float f = fabs( v );
		pShadowGeomTS->AddClipPlane( CVec4( -vPlane.x / f, -vPlane.y / f, -vPlane.z / f, vPlane.w / f ) );
	}
}

//void MakeSceneGeometryBound( SBound *pRes, const CTransformStack &ts, float fMaxHeight )
//{
//	CVec4 vC, v00, v01, v10, v11, vC1;
//	ts.Get().backward.RotateHVector( &vC, CVec4(0,0,1,0) );
//	ts.Get().backward.RotateHVector( &v00, CVec4(-1,-1,1,1) );
//	ts.Get().backward.RotateHVector( &v01, CVec4(-1, 1,1,1) );
//	ts.Get().backward.RotateHVector( &v11, CVec4( 1, 1,1,1) );
//	ts.Get().backward.RotateHVector( &v10, CVec4( 1,-1,1,1) );
//	ts.Get().backward.RotateHVector( &vC1, CVec4( 0, 0,1,1) );
//	NGeometry::SPolyhedron hedron, occluderHedron;
//	hedron.facets.push_back( NGeometry::GetTriangle( vC, v00, v10 ) );
//	hedron.facets.push_back( NGeometry::GetTriangle( vC, v10, v11 ) );
//	hedron.facets.push_back( NGeometry::GetTriangle( vC, v11, v01 ) );
//	hedron.facets.push_back( NGeometry::GetTriangle( vC, v01, v00 ) );
//	hedron.facets.push_back( NGeometry::GetQuad( v00, v01, v11, v10 ) );
//	NGeometry::Split( &hedron, SPlane( CVec3(0,0,1), 0 ) );
//	NGeometry::Split( &hedron, SPlane( CVec3(0,0,-1), fMaxHeight * 1 ) );
//	SBoundCalcer bc;
//	for ( int i = 0; i < hedron.facets.size(); ++i )
//	{
//		const NGeometry::SPolygon &p = hedron.facets[i];
//		for ( int k = 0; k < p.vertices.size(); ++k )
//			bc.Add( Unhomogen( p.vertices[k] ) );
//	}
//	bc.Make( pRes );
//}

}

