#include "StdAfx.h"
#include "GRenderFactor.h"
#include "GfxBuffers.h"
#include "GfxEffects.h"
#include "..\Misc\2DArray.h"
namespace NGScene
{

static CObj<NGfx::CTexture> pSpecularResponse, pUniformBump;
static CObj<NGfx::CTexture> pBlackTexture, pSpecularResponseR300, pWhiteTexture, pDefaultLightmap;
static CObj<NGfx::CTexture> pFalloffLookup, pChecker;
static CObj<NGfx::CTexture> p16bitDepthLookup;
static void InitSolidTexture( CObj<NGfx::CTexture> *pTexture, const NGfx::SPixel8888 &color, int nSize )
{
	*pTexture = NGfx::MakeTexture( nSize, nSize, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( *pTexture, 0, NGfx::INPLACE );
	for ( int x = 0; x < nSize; ++x )
	{
		for ( int y = 0; y < nSize; ++y )
			lock[y][x] = color;
	}
}

NGfx::CTexture* GetUniformBump()
{
	if ( IsValid(pUniformBump) )
		return pUniformBump;
	InitSolidTexture( &pUniformBump, NGfx::SPixel8888( 128, 128, 255, 255 ), 1 );
	return pUniformBump;
}

NGfx::CTexture* GetBlackTexture()
{
	if ( IsValid(pBlackTexture) )
		return pBlackTexture;
	InitSolidTexture( &pBlackTexture, NGfx::SPixel8888( 0,0,0,0 ), 1 );
	return pBlackTexture;
}

NGfx::CTexture* GetWhiteTexture()
{
	if ( IsValid(pWhiteTexture) )
		return pWhiteTexture;
	InitSolidTexture( &pWhiteTexture, NGfx::SPixel8888( 255,255,255,255 ), 1 );
	return pWhiteTexture;
}

NGfx::CTexture* GetDefaultLightmap()
{
	if ( IsValid(pDefaultLightmap) )
		return pDefaultLightmap;
	InitSolidTexture( &pDefaultLightmap, NGfx::SPixel8888( 0,0,0,255 ), 1 );
	return pDefaultLightmap;
}

static void RefreshSpecularResponse()
{
	if ( IsValid( pSpecularResponse ) )
		return;
	const int N_SIZE = 512;
	pSpecularResponse = NGfx::MakeTexture( N_SIZE, N_SIZE, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pSpecularResponse, 0, NGfx::INPLACE );
	for ( int y = 0; y < N_SIZE; ++y )
	{
		float fRo = (y + 0.5f) / N_SIZE;
		float fPower = triple( fabs(fRo) ) * 256;
		float fResMul;
		fResMul = sqrt(sqrt( Max( fPower, 1.0f ) ));
		if ( fRo == 0 )
			fResMul = 1;
		for ( int x = 0; x < N_SIZE; ++x )
		{
			float fX = (x + 0.5f) / (N_SIZE);
			if ( fX > 0.5f )
			{
				fX = ( fX - 0.5f ) * 2;
				fX = sqrt( fX * 4 ); // fX = length(N-H)
				fX = Max( fX, (float)sqrt(0.5f) );
			}
			else
				fX = sqrt( fX ); // fX = length(N-H)
			fX = cos( 2 * asin( fX * 0.5f ) ); // fX = (N*H)
			float fRes;

			fX = fabs(fX);
			if ( fX > 0 )
				fRes = (float)(pow(fX, fPower)) * fResMul;
			else
				fRes = 0;
			fRes = Clamp( fRes * 128, 0.0f, 255.0f );
			char cRes = Float2Int( fRes );
			lock[y][x] = NGfx::SPixel8888( cRes, cRes, cRes, cRes );
			//lock[y][x] = NGfx::SPixel8888( y, x, 0, 0 );
		}
	}
}

static void RefreshSpecularResponseR300()
{
	if ( IsValid( pSpecularResponseR300 ) )
		return;
	const int N_SIZE = 512;
	pSpecularResponseR300 = NGfx::MakeTexture( N_SIZE, N_SIZE, 1, NGfx::SPixelFloat::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixelFloat> lock( pSpecularResponseR300, 0, NGfx::INPLACE );
	for ( int y = 0; y < N_SIZE; ++y )
	{
		float fRo = (y + 0.5f) / N_SIZE;
		float fPower = triple( fabs(fRo) ) * 256;
		float fResMul;
		fResMul = sqrt(sqrt( Max( fPower, 1.0f ) ));
		if ( fRo == 0 )
			fResMul = 1;
		for ( int x = 0; x < N_SIZE; ++x )
		{
			float fX = (x + 0.5f) / (N_SIZE); // fX = (N*H)
			float fRes;

			fX = fabs(fX);
			if ( fX > 0 )
				fRes = (float)(pow(fX, fPower)) * fResMul;
			else
				fRes = 0;
			lock[y][x].r = fRes;
		}
	}
}

NGfx::CTexture* GetSpecularResponse()
{
	RefreshSpecularResponse();
	return pSpecularResponse;
}

NGfx::CTexture* GetSpecularResponseR300()
{
	RefreshSpecularResponseR300();
	return pSpecularResponseR300;
}

static CObj<NGfx::CCubeTexture> pNormalize;
const int N_CUBE_SIZE = 128;
static float Rnd()
{
	static int nA = 23453425;
	nA = nA * 8923457 + 4937852;
	return ( ( nA >> 8 ) & 0xffff ) * ( 1.0f / 65535 ) - 0.5f;
}
static void NormaliseFace( NGfx::CCubeTexture *pTex, NGfx::EFace f )
{
	CVec3 n;
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pTex, f, 0, NGfx::INPLACE );
	for ( int x = 0; x < N_CUBE_SIZE; ++x )
	{
		float w = ((float)x) / (N_CUBE_SIZE - 1) * 2 - 1;
		for ( int y = 0; y < N_CUBE_SIZE; ++y )
		{
			float h = ((float)y) / (N_CUBE_SIZE - 1) * 2 - 1;
			switch( f )
			{
				case NGfx::POSITIVE_X: n = CVec3( 1.0f,    -h,    -w); break;
				case NGfx::NEGATIVE_X: n = CVec3(-1.0f,    -h,     w); break;
				case NGfx::POSITIVE_Y: n = CVec3(    w,  1.0f,     h); break;
				case NGfx::NEGATIVE_Y: n = CVec3(    w, -1.0f,    -h); break;
				case NGfx::POSITIVE_Z: n = CVec3(    w,    -h,  1.0f); break;
				case NGfx::NEGATIVE_Z: n = CVec3(   -w,    -h, -1.0f); break;
				default: ASSERT( 0 ); break;
			}
			if ( fabs2(n) > 0 )
				Normalize( &n );
			float fZ = n.z;
			n += CVec3(1,1,1);
			n *= 127;
			lock[y][x] = NGfx::SPixel8888( 
				Float2Int(n.x + Rnd()), 
				Float2Int(n.y + Rnd()), 
				Float2Int(n.z + Rnd()), 
				fZ > 0 ? pow( fZ, 8 ) * 255 : 0
				);
		}
	}
}

NGfx::CCubeTexture* GetNormalizeTexture()
{
	if ( IsValid( pNormalize ) )
		return pNormalize;
	pNormalize = NGfx::MakeCubeTexture( N_CUBE_SIZE, 1, NGfx::SPixel8888::ID, NGfx::REGULAR );
	NormaliseFace( pNormalize, NGfx::POSITIVE_X );
	NormaliseFace( pNormalize, NGfx::POSITIVE_Y );
	NormaliseFace( pNormalize, NGfx::POSITIVE_Z );
	NormaliseFace( pNormalize, NGfx::NEGATIVE_X );
	NormaliseFace( pNormalize, NGfx::NEGATIVE_Y );
	NormaliseFace( pNormalize, NGfx::NEGATIVE_Z );
	return pNormalize;
}

static void Normalize( CArray2D<float> *pRes )
{
	float fM = 0, fA = 0;
	for ( int y = 0; y < pRes->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pRes->GetSizeX(); ++x )
		{
			fM += sqr( (*pRes)[y][x] );
			fA += (*pRes)[y][x];
		}
	}
	fA /= pRes->GetSizeX() * pRes->GetSizeY(); 
	fM /= pRes->GetSizeX() * pRes->GetSizeY(); 
	for ( int y = 0; y < pRes->GetSizeY(); ++y )
	{
		for ( int x = 0; x < pRes->GetSizeX(); ++x )
		{
			float f = ( (*pRes)[y][x] - fA ) / fM;
			f = ( f / 6 + 0.5f ) * 256;
			(*pRes)[y][x] = Clamp( f, 0.0f, 255.0f );
		}
	}
}

NGfx::CTexture* GetLightFallLookup()
{
	if ( IsValid( pFalloffLookup ) )
		return pFalloffLookup;
	const int N_SIZE = 256;
	pFalloffLookup = NGfx::MakeTexture( N_SIZE, 1, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pFalloffLookup, 0, NGfx::INPLACE );
	for ( int x = 0; x < N_SIZE; ++x )
	{
		for ( int y = 0; y < 1; ++y )
		{
			//float fQ2 = ( y + 0.5f ) / N_SIZE;
			float fDot = ( x + 0.5f ) / N_SIZE;
			//float fQ1 = fQ2;//sqrt( fQ2 );
			float fQ1 = 1;
			float fDistance = Clamp( fDot / fQ1, 0.0f, 1.0f );
			float fFall = 1 / ( 1 + sqr( fDistance * NGfx::F_POINT_FALLOFF ) ) * sqrt( sqrt( 1 - fDistance ) );
			char cRes = Float2Int( fFall * 255 );
			lock[y][x] = NGfx::SPixel8888( cRes, cRes, cRes, cRes );
		}
	}
	return pFalloffLookup;
}

NGfx::CTexture* GetCheckerTexture()
{
	if ( IsValid( pChecker ) )
		return pChecker;
	const int N_SIZE = 256;
	pChecker = NGfx::MakeTexture( N_SIZE, N_SIZE, 1, NGfx::SPixel8888::ID, NGfx::REGULAR, NGfx::WRAP );
	NGfx::CTextureLock<NGfx::SPixel8888> lock( pChecker, 0, NGfx::INPLACE );
	for ( int x = 0; x < N_SIZE; ++x )
	{
		for ( int y = 0; y < N_SIZE; ++y )
		{
			char cRes = ( ( x + y ) & 1 ) ? 255 : 0;
			lock[y][x] = NGfx::SPixel8888( cRes, cRes, cRes, cRes );
		}
	}
	return pChecker;
}

NGfx::CTexture* Get16bitDepthLookup()
{
	if ( IsValid( p16bitDepthLookup ) )
		return p16bitDepthLookup;
	const int N_SIZE = 256;
	p16bitDepthLookup = NGfx::MakeTexture( N_SIZE, 1, 1, NGfx::SPixel565::ID, NGfx::REGULAR, NGfx::CLAMP );
	NGfx::CTextureLock<NGfx::SPixel565> lock( p16bitDepthLookup, 0, NGfx::INPLACE );
	for ( int x = 0; x < N_SIZE; ++x )
	{
		lock[0][x] = NGfx::SPixel565( 0, x >> 2, x & 3 );
	}
	return p16bitDepthLookup;
}

}
using namespace NGScene;
