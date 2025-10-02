#include "StdAfx.h"
#include "GfxBenchmark.h"
#include "GfxRender.h"
#include "GfxBuffers.h"
#include "3Dlib/Transform.h"
#include "GfxShaders.h"
#include "Gfx.h"
#include "Misc/HPTimer.h"
#include "GfxInternal.h"
//#include <D3D9.h>
//#include "..\Misc\Win32Helper.h"


// ? tnl mode
// ? measure texture amount influence
namespace NGfx
{
const int N_GRID_SIZE = 100;
const int N_FILL_PASSES = 200;
struct STestData
{
	CObj<CGeometry> pGeom;
	CObj<CTriList> pTris, pManyTris;
	CObj<CTexture> pTex, pDepth;

	void FillTex( CObj<CTexture> *pRes, int nSize, int nMips )
	{
		*pRes = NGfx::MakeTexture( nSize, nSize, nMips, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
		static int nRnd;
		++nRnd;
		for ( int nLevel = 0; nLevel < nMips; ++nLevel )
		{
			NGfx::CTextureLock<NGfx::SPixel8888> t( *pRes, nLevel, NGfx::INPLACE );
			int nSizeX = t.GetSizeX(), nSizeY = t.GetSizeY();
			for ( int y = 0; y < nSizeY; ++y )
			{
				for ( int x = 0; x < nSizeX; ++x )
					t[y][x].dwColor = x * x + y * x + y * y + nRnd;
			}
		}
	}
	void Init()
	{
		CBufferLock<SGeomVecFull> g( &pGeom, (N_GRID_SIZE+1) * (N_GRID_SIZE+1) );
		SGeomVecFull zeroVec;
		Zero( zeroVec );
		zeroVec.texU.dw = 0xffffffff;
		zeroVec.texV.dw = 0xffffffff;
		for ( int y = 0; y <= N_GRID_SIZE; ++y )
		{
			for ( int x = 0; x <= N_GRID_SIZE; ++x )
			{
				float fU = y / ( (float)N_GRID_SIZE );
				float fV = x / ( (float)N_GRID_SIZE );
				int nDst = y * (N_GRID_SIZE+1) + x;
				SGeomVecFull &dst = g[ nDst ];
				dst = zeroVec;
				dst.pos = CVec3( -2 + 4 * fU, -2 + 4 * fV, 0 );
				CalcCompactVector( &dst.normal, CVec3(0,0,1) );
				CalcTexCoords( &dst.tex, fU, fV );
				//CalcTexCoords( &dst.tex, fV, fU ); // somehow this is faster
			}
		}

		CBufferLock<S3DTriangle> t( &pTris, N_FILL_PASSES * 2 );
		for ( int i = 0; i < N_FILL_PASSES; ++i )
		{
			t[i*2+0] = NGfx::S3DTriangle( i%5, N_GRID_SIZE * (N_GRID_SIZE+1) + N_GRID_SIZE, N_GRID_SIZE );
			t[i*2+1] = NGfx::S3DTriangle( i%3, N_GRID_SIZE * (N_GRID_SIZE+1), N_GRID_SIZE * (N_GRID_SIZE+1) + N_GRID_SIZE );
		}
		//t[0] = NGfx::S3DTriangle( N_GRID_SIZE * (N_GRID_SIZE+1), N_GRID_SIZE * (N_GRID_SIZE+1) + N_GRID_SIZE, N_GRID_SIZE );
		//t[0] = NGfx::S3DTriangle( 0, N_GRID_SIZE * (N_GRID_SIZE+1), N_GRID_SIZE );
		CBufferLock<S3DTriangle> tMany( &pManyTris, N_GRID_SIZE * N_GRID_SIZE );
		for ( int y = 0; y < N_GRID_SIZE; ++y )
		{
			for ( int x = 0; x < N_GRID_SIZE; ++x )
			{
				int nBase = y * ( N_GRID_SIZE + 1 ) + x;
				tMany[ y * N_GRID_SIZE + x ] = NGfx::S3DTriangle( nBase, nBase + ( N_GRID_SIZE + 1 ) + 1, nBase + 1 );
			}
		}

		// if pTex & pDepth are stored in AGP memory performance seriously suffers
		// to avoid this influence these small sizes textures can be used
		FillTex( &pTex, 8, 2 );
		FillTex( &pDepth, 8, 1 );
	}
	void ClearTex() { pTex = 0; pDepth = 0; }
	void InitFullSizeTex()
	{
		FillTex( &pTex, 512, 7 ); //FillTex( &pTex, 1024, 5 );
		FillTex( &pDepth, 1024, 1 );
	}
};

static float TimeRender( NGfx::CRenderContext &rc, int nPasses, CGeometry *pGeom, CTriList *pTris )
{
	rc.AddPrimitive( pGeom, pTris );
	rc.Flush();
	vector<float> times;
	FlushQueue();
	for ( int i = 0; i < 5; ++i )
	{
		NHPTimer::STime tBegin;
		NHPTimer::GetTime( &tBegin );
		for ( int i = 0; i < nPasses; ++i )
			rc.AddPrimitive( pGeom, pTris );
		rc.Flush();
		FlushQueue();
		times.push_back( NHPTimer::GetTimePassed( &tBegin ) );
		//Flip();
	}
	sort( times.begin(), times.end() );
	return times[ times.size() / 2 ];
}

static SPerformanceInfo perf;
//extern NWin32Helper::com_ptr<IDirect3DDevice9> pDevice;
void PerformBenchmark()
{
	int nOldAniso = nUseAnisotropy;
	nUseAnisotropy = 1;
	ForceTextureFilterSetup();

	CRenderContext rc;

	CTransformStack ts;
	ts.MakeProjective( 1 );
	SHMatrix cameraPos;
	MakeMatrix( &cameraPos, CVec3( 0,0,4 ), CVec3(0,0,-1) );
	ts.SetCamera( cameraPos );

	STestData td;
	td.Init();

	{
		rc.ClearBuffers();
		rc.SetTransform( ts.Get() );

		rc.SetPixelShader( psG3DiffuseTex );
		rc.SetVertexShader( vsG3DiffuseTex );
		rc.SetTransform( ts.Get() );

		rc.SetVSConst( 14, CVec4(0.25,0.25,0.25,0.25) );
		rc.SetVSConst( 15, CVec4(0,0,-1,0) );

		rc.SetVSConst( 24, CVec4(0,0,0.001f,0.1f) );
		rc.SetVSConst( 25, CVec4( 0.2f, 0, 0, 0 ) );
		rc.SetVSConst( 26, CVec4( 0, 0.2f, 0, 0 ) );
		rc.SetVSConst( 27, CVec4(0,0,1,1) );

		rc.SetVSConst( 29, CVec4(0.5,0.5,0.5,0.5) );
		rc.SetVSConst( 30, CVec4( 0.4f, 0.6f,0,0) );
		rc.SetTexture( 0, td.pTex, NGfx::FILTER_BEST );
		rc.SetTexture( 1, td.pDepth, NGfx::FILTER_LINEAR );
		rc.SetAlphaRef( 0 );
		rc.SetVSConst( 35, CVec4(0,0,0,1e-5f) );
		rc.SetPSConst( 4, CVec4( 0,0,0, 124/256.0f ) );

		CVec2 vScreenSize = GetScreenRect();
		float fPixels = N_FILL_PASSES * 0.25 * vScreenSize.x * vScreenSize.y;

		float fFillTime = TimeRender( rc, 1, td.pGeom, td.pTris );
		perf.fPSRate = fPixels / fFillTime / 1000000;

		const int N_TRIS_PASSES = 50;
		float fTriTime = TimeRender( rc, N_TRIS_PASSES, td.pGeom, td.pManyTris );
		perf.fTriangleRate = N_TRIS_PASSES * N_GRID_SIZE * N_GRID_SIZE / fTriTime / 1000000;

		td.InitFullSizeTex();
		rc.SetTexture( 0, td.pTex, NGfx::FILTER_BEST );
		rc.SetTexture( 1, td.pDepth, NGfx::FILTER_LINEAR );
		fFillTime = TimeRender( rc, 1, td.pGeom, td.pTris );
		perf.fFillRate = fPixels / fFillTime / 1000000;

		//vector<NWin32Helper::com_ptr<IDirect3DVertexBuffer9> > stuff;
		//for ( int nSafe = 0; nSafe < 1000; ++nSafe )
		//{
		//	td.ClearTex();
		//	NWin32Helper::com_ptr<IDirect3DVertexBuffer9> pHold;
		//	HRESULT hr = pDevice->CreateVertexBuffer( 1024 * 1024 * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, pHold.GetAddr(), 0 );
		//	if ( !pHold || hr != D3D_OK )
		//		break;
		//	stuff.push_back( pHold );
		//	td.InitFullSizeTex();
		//	fFillTime = TimeRender( rc, N_FILL_PASSES, td.pGeom, td.pTris );
		//	float fFR = fPixels / fFillTime / 1000000;
		//	DebugTrace( "%g", fFR );
		//}

		perf.fCPUclock = NHPTimer::GetClockRate() / 1000000;
	}

	nUseAnisotropy = nOldAniso;
	ForceTextureFilterSetup();
}

const SPerformanceInfo &GetPerformanceInfo()
{
	return perf;
}
}

