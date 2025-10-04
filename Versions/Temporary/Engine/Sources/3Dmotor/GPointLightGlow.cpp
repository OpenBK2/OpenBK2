#include "stdafx.h"
#include "GParticleInfo.h"
#include "GPointLightGlow.h"
#include "GScene.h"
#include "RandomGen.h"
#include "4dCalcs.h"

namespace NGScene
{

class CPointGlowEffect : public CParticleEffect
{
	OBJECT_BASIC_METHODS(CPointGlowEffect);
public:
	CVec3 vPos;
	float fSize;
	int nAlpha;
	CPtr<CPointGlowAnimator> pParent;

	void AddParticles( IParticleOutput *pRender );
};

const float F_NEAR_CLIP = 1;//0.2f;
void CPointGlowEffect::AddParticles( IParticleOutput *pRender )
{
	pParent->CalcSize();

	const SParticleOrientationInfo &or = pRender->GetOrientationInfo();
	CVec3 vRes[4];
	CVec3 vNearPos( vPos - or.vBasic[3] );
	float fLeng = vNearPos * or.vBasic[2];
	if ( fLeng <= F_NEAR_CLIP )
		return;
	vNearPos = vPos;//or.vBasic[3] + vNearPos * ( F_NEAR_CLIP / fLeng );
	float fS = fSize;// * F_NEAR_CLIP / fLeng;
	vRes[0] = vNearPos - or.vBasic[0] * fS - or.vBasic[1] * fS;
	vRes[1] = vNearPos + or.vBasic[0] * fS - or.vBasic[1] * fS;
	vRes[2] = vNearPos + or.vBasic[0] * fS + or.vBasic[1] * fS;
	vRes[3] = vNearPos - or.vBasic[0] * fS + or.vBasic[1] * fS;
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( textures[0] );
	if ( !IsValid( pTex ) )
		return;
	pTex.Refresh();
	STransparentTexturePlace tPlace;
	GetTransparentTexturePlace( &tPlace, pTex->GetValue() );
	pRender->AddParticle( vRes, 0xffffff | (nAlpha<<24), tPlace, F_PARTICLE_OVER );//or.vDepth * or.vBasic[3] );
}

// CPointGlowAnimator

CPointGlowAnimator::CPointGlowAnimator( IGScene *_pScene, CFuncBase<STime> *_pTime, CFuncBase<CVec3> *_pPlace, 
	CPtrFuncBase<NGfx::CTexture> *_pTexture, float _fLightSize, float _fOnTime, float _fOffTime )
	: pScene(_pScene), pTime(_pTime), pPlacement(_pPlace), pTexture(_pTexture), fLightSize(_fLightSize),
	expFader(_fOnTime,_fOffTime, 0), pCamera( pScene->GetCamera() ), nLastMaskAny(0), tNextCheck(0), bIsVisible(false)
{
}

void CPointGlowAnimator::CalcSize()
{
	bool bShortInterval = pCamera.Refresh();
	SGroupSelect mask( pScene->GetLastMask() );
	if ( nLastMaskAny != mask.nMaskAny )
	{
		bShortInterval = true;
		nLastMaskAny = mask.nMaskAny;
	}
	STime tCur = pTime->GetValue();
	if ( bShortInterval )
		tNextCheck = Min( tNextCheck, tCur + 150 );
	if ( tCur > tNextCheck )
	{
		CVec4 vCamera4 = pCamera->GetValue();
		CRay r;
		CVec3 vCamera = SafeUnhomogen( vCamera4 );
		r.ptOrigin = vCamera;
		r.ptDir = pPlacement->GetValue() - vCamera;
		float f;
		CVec3 vNormal;
		mask.nMaskEvery |= N_MASK_CAST_SHADOW;
		if ( pScene->TraceScene( mask, r, &f, &vNormal, SPS_STATIC ) )
			bIsVisible = f > 1;
		else
			bIsVisible = true;
		if ( bShortInterval )
			tNextCheck = tCur + random.Get( 80, 150 );
		else
			tNextCheck = tCur + random.Get( 500, 1000 );
	}
	expFader.Update( tCur, bIsVisible ? 1 : 0 );

	CDynamicCast<CPointGlowEffect> pRealValue(pValue);
	CPointGlowEffect &value = *pRealValue;
	value.fSize = fLightSize * expFader.GetSize();
	value.nAlpha = Float2Int( expFader.GetSize() * 255 );
}

void CPointGlowAnimator::Recalc()
{
	if ( !IsValid( pValue ) )
	{
		pValue = new CPointGlowEffect;
		pValue->textures.resize(1);
		pValue->textures[0] = pTexture;
	}
	CDynamicCast<CPointGlowEffect> pRealValue(pValue);
	CPointGlowEffect &value = *pRealValue;
	value.bEnd = false;
	value.vPos = pPlacement->GetValue();
	value.pParent = this;
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x114a2180, CPointGlowEffect )
REGISTER_SAVELOAD_CLASS( 0x114a2181, CPointGlowAnimator )

