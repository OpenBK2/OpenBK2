#include "StdAfx.h"
#include "GPostEffects.h"

namespace NGfx
{

// CMonochromeEffect

void CMonochromeEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( "Monochrome" );
	pRC->SetVertexShader( "Render2DVS" );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_LINEAR );
}

// CTwilightEffect

void CTwilightEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	float fTimeVal = ( Float2Int( GetTickCount() * fTime ) % 1000 ) / 500.0f ;

	static float o1=0.0f;
	static float o2=0.0f;

	o1=0.01f*sin(GetTickCount()*0.01f);
	o2=0.0f*cos(GetTickCount()*0.01f);


	pRC->SetAlphaCombine(NGfx::COMBINE_ALPHA );

	pRC->SetPixelShader( "Twilight" );
	pRC->SetVertexShader( "TwilightVS" );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0.0f , - fTimeVal ) );
	pRC->SetTexture( 0, pTex, FILTER_LINEAR );
	pRC->SetTexture( 1, pNoise, FILTER_LINEAR );
	//pRC->SetTexture( 2, pGlow, FILTER_LINEAR );
	//pRC->SetTexture( 3, pMask, FILTER_LINEAR );
//	pRC->SetPSConst( 1, CVec4( fCoeff, fCoeff, fCoeff, fCoeff ) );
//	pRC->SetPSConst( 2, CVec4( 1 - fCoeff, 1 - fCoeff, 1 - fCoeff, 1 - fCoeff ) );
//	pRC->SetPSConst( 3, CVec4( 0, fTimeVal, 0, 0 ) );
}

// CGausianBlurEffect

void CGausianBlurEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	pRC->SetPixelShader( "GausianBlur" );
	pRC->SetVertexShader( "GausianBlurVS" );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );
	pRC->SetVSConst( 17, CVec4( bVert ? 0 :  0.001f, bVert ?  0.001f : 0, 0, 0 ) );
	pRC->SetTexture( 0, pTex, FILTER_LINEAR );
}

NGfx::CTexture* ApplyFiltersBase( NGfx::CTexture *pSrc, const CTRect<float> &srcRect, const CTRect<float> &rectRegDS, const vector<CPtr<I2DEffect> > &filters )
{
	CTRect<float> rectReg;
	NGfx::GetRegisterSize( &rectReg );

	NGfx::CRenderContext rc;
	rc.SetVirtualRT();

	int nRegDst = 1;
	CTRect<float> rectSrc( srcRect ), rectDst( rectRegDS );
	for ( int nTemp = 0; nTemp < filters.size(); ++nTemp )
	{
		rc.SetRegister( nRegDst );
		NGfx::CopyTexture( rc, CVec2( rectReg.Width(), rectReg.Height() ), rectDst, pSrc, rectSrc, CVec4( 1, 1, 1, 1 ), filters[nTemp] );

		if ( nRegDst == 1 )
		{
			nRegDst = 2;
			pSrc = NGfx::GetRegisterTexture( 1 );
		}
		else
		{
			nRegDst = 1;
			pSrc = NGfx::GetRegisterTexture( 2 );
		}

		rectSrc = rectRegDS;
		rectDst = rectRegDS;
	}

	return pSrc;
}

void ApplyFilters( int nDownSample, const vector<CPtr<I2DEffect> > &filters )
{
	if ( filters.empty() )
		return;

	CTRect<float> rectReg;
	NGfx::GetRegisterSize( &rectReg );
	CTRect<float> rectRegDS( 0, 0, (int)rectReg.Width() / nDownSample, (int)rectReg.Height() / nDownSample );

	NGfx::CTexture *pSrc = ApplyFiltersBase( NGfx::GetRegisterTexture( 0 ), rectReg, rectRegDS, filters );

	ASSERT (pSrc);

	NGfx::CRenderContext rc;
	rc.SetVirtualRT();
	rc.SetRegister( 0 );
	NGfx::CopyTexture( rc, CVec2( rectReg.Width(), rectReg.Height() ), rectReg, pSrc, rectRegDS );
}

void ApplyFilters( NGfx::CTexture *pTarget, const CTRect<float> &targetRect, int nDownSample, const vector<CPtr<I2DEffect> > &filters )
{
	if ( filters.empty() )
		return;

	CTRect<float> rectRegDS( 0, 0, (int)targetRect.Width() / nDownSample, (int)targetRect.Height() / nDownSample );

	NGfx::CTexture *pSrc = ApplyFiltersBase( pTarget, targetRect, rectRegDS, filters );

	NGfx::CRenderContext rc;
	rc.SetTextureRT( pTarget );
	NGfx::CopyTexture( rc, CVec2( targetRect.Width(), targetRect.Height() ), targetRect, pSrc, rectRegDS );
}

} // NAMESPACE


