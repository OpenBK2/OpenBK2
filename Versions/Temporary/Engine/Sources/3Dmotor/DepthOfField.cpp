#include "stdafx.h"
#include "GfxUtils.h"
#include "GfxShaders.h"
#include "GRenderExecute.h"
#include "GRenderFactor.h"
#include "DepthOfField.h"

namespace NGScene
{

class CDepthOfFieldEffect: public NGfx::I2DEffect
{
	OBJECT_NOCOPY_METHODS(CDepthOfFieldEffect);
	//
	NGfx::CTexture *pColor, *pDepth;
	int nWidth, nHeight;
protected:
	CDepthOfFieldEffect() {}
public:
	CDepthOfFieldEffect( NGfx::CTexture *_pColor, NGfx::CTexture *_pDepth, int _nWidth, int _nHeight ) :
			pColor(_pColor), pDepth(_pDepth), nWidth(_nWidth), nHeight(_nHeight) {}
	virtual void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV );
};

void CDepthOfFieldEffect::SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV )
{
	const float dx = 1.0f / nWidth;
	const float dy = 1.0f / nHeight;

	// Scale tap 2D offsets
	const CVec4 v0(  -0.326212f * dx, -0.405805f * dy, 0.0f, 0.0f );
	const CVec4 v1(  -0.840144f * dx, -0.07358f * dy, 0.0f, 0.0f );
	const CVec4 v2(  -0.695914f * dx, 0.457137f * dy, 0.0f, 0.0f );
	const CVec4 v3(  -0.203345f * dx, 0.620716f * dy, 0.0f, 0.0f );
	const CVec4 v4(   0.96234f * dx, -0.194983f * dy, 0.0f, 0.0f );
	const CVec4 v5(   0.473434f * dx, -0.480026f * dy, 0.0f, 0.0f );
	const CVec4 v6(   0.519456f * dx, 0.767022f * dy, 0.0f, 0.0f );
	const CVec4 v7(   0.185461f * dx, -0.893124f * dy, 0.0f, 0.0f );
	const CVec4 v8(   0.507431f * dx, 0.064425f * dy, 0.0f, 0.0f );
	const CVec4 v9(   0.89642f * dx, 0.412458f * dy, 0.0f, 0.0f );
	const CVec4 v10( -0.32194f * dx, -0.932615f * dy, 0.0f, 0.0f );
	const CVec4 v11( -0.791559f * dx, -0.597705f * dy, 0.0f, 0.0f );
	pRC->SetPSConst( 0, v0 ); pRC->SetPSConst( 1, v1 ); pRC->SetPSConst( 2, v2 );	pRC->SetPSConst( 3, v3 );
	pRC->SetPSConst( 4, v4 ); pRC->SetPSConst( 5, v5 ); pRC->SetPSConst( 6, v6 );	pRC->SetPSConst( 7, v7 );
	pRC->SetPSConst( 8, v8 ); pRC->SetPSConst( 9, v9 ); pRC->SetPSConst( 10, v10 );	pRC->SetPSConst( 11, v11 );

	const float fCoCSize = 8.0f;
	const CVec4 vCoC( fCoCSize / 2.0f, 0.0f, 1.0f, 0.0f );
	pRC->SetPSConst( 26, vCoC );

	pRC->SetPixelShader( psG3DepthOfField );

	pRC->SetVertexShader( vsRender2D );
	pRC->SetVSConst( 16, CVec4( fScaleU, fScaleV, 0, 0 ) );

	pRC->SetTexture( 0, pColor, NGfx::FILTER_POINT );
	pRC->SetTexture( 1, pDepth, NGfx::FILTER_POINT );
}

void ProcessDepthOfField( const SDepthOfField *pDOF, const CSceneFragments *pScene, const CTransformStack *pTS, IRender *pRender )
{
	if ( pDOF && ( NGfx::GetHardwareLevel() == NGfx::HL_R300 ) && NGfx::CopyScreenToRegister( 0 ) )
	{
		const CVec4 vDOFParams( pDOF->fFocalDist, 1.0f / pDOF->fFocusRange, 0.0f/*1.0f / pDOF->fFocusRangeBackward*/, 0.0f );

		NGfx::CRenderContext rc;

		rc.SetVirtualRT();
		rc.SetRegister( 1 );
		rc.ClearBuffers( 0x00ffffff );

		CRenderCmdList lightOps;
		const std::vector<SRenderFragmentInfo*> &fragments = pScene->GetFragments();

		for ( int k = 0; k < fragments.size(); ++k )
		{
			if ( pScene->IsFilteredFragment( k ) )
				continue;
			const SRenderFragmentInfo &f = *fragments[k];
			if ( f.vars.fFade > 0.0f )
			{
				COpGenContext fi( &lightOps.ops, &f );
				if ( f.pMaterial )
					f.pMaterial->AddCustomOperation( &fi, RO_WRITE_Z, 0, 0, 1, &vDOFParams );
				else
					fi.AddOperation( RO_WRITE_Z, 0, 0, 1, GetWhiteTexture(), &vDOFParams );
			}
		}
		Execute( pRender, &rc, *pTS, lightOps, *pScene, SLightInfo() );

		rc.SetScreenRT();

		{
			CTRect<float> rectReg;
			NGfx::GetRegisterSize( &rectReg );
			const int nWidth = rectReg.Width();
			const int nHeight = rectReg.Height();
			CTRect<float> rectRegDS( 0, 0, nWidth, nHeight );

			std::vector<CPtr<NGfx::I2DEffect> > filters;
			filters.push_back( new CDepthOfFieldEffect( NGfx::GetRegisterTexture( 0 ), NGfx::GetRegisterTexture( 1 ), nWidth, nHeight ) );

			CTRect<float> rectSrc( rectReg ), rectDst( rectRegDS );
			NGfx::CopyTexture( rc, CVec2( rectReg.Width(), rectReg.Height() ), rectDst, NGfx::GetRegisterTexture( 0 ), rectSrc, CVec4( 1, 1, 1, 1 ), filters[0] );
		}
	}
}

} // namespace NGScene


