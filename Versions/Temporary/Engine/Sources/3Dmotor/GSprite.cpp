#include "stdafx.h"
#include "GScene.h"
#include "GParticleInfo.h"
#include "4dCalcs.h"

#include "GSprite.h"

namespace NGScene
{

// CSpriteEffect

class CSpriteEffect: public CParticleEffect
{
	OBJECT_BASIC_METHODS(CSpriteEffect);
public:
	CVec3 sPos;
	CVec3 sDir;
	CVec2 sSize;
	CVec3 sCamera;
	////
	void AddParticles( IParticleOutput *pRender );
};

void CSpriteEffect::AddParticles( IParticleOutput *pRender )
{
	const SParticleOrientationInfo &or = pRender->GetOrientationInfo();
	////
	Normalize( &sDir );
	CVec3 sNormal = sDir ^ or.vBasic[2];
	Normalize( &sNormal );
	sDir *= sSize.x;
	sNormal *= sSize.y;
	////
	CVec3 sRes[4];
	sRes[0] = sPos - sNormal;
	sRes[1] = sPos + sDir - sNormal;
	sRes[2] = sPos + sDir + sNormal;
	sRes[3] = sPos + sNormal;
	////
	CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( textures[0] );
	if ( !IsValid( pTex ) )
		return;
	pTex.Refresh();
	STransparentTexturePlace sTexPlace;
	GetTransparentTexturePlace( &sTexPlace, pTex->GetValue() );
	pRender->AddParticle( sRes, 0xFFFFFFFF, sTexPlace, or.vDepth * sPos );
}

// CSpriteAnimator

CSpriteAnimator::CSpriteAnimator( IGScene *_pScene, CFuncBase<CVec3> *_pPosition, CFuncBase<CVec3> *_pDir, CPtrFuncBase<NGfx::CTexture> *_pTexture, CFuncBase<CVec2> *_pSize ):
	pScene(_pScene), pPosition(_pPosition), pDir(_pDir), pTexture(_pTexture), pSize(_pSize)
{
	pCamera = pScene->GetCamera();
}

void CSpriteAnimator::Recalc()
{
	if ( !IsValid( pValue ) )
	{
		pValue = new CSpriteEffect;
		pValue->textures.resize( 1 );
		pValue->textures[0] = pTexture;
	}
	////
	pDir.Refresh();
	pCamera.Refresh();
	pPosition.Refresh();
	pSize.Refresh();
	////
	CDynamicCast<CSpriteEffect> pRealValue( pValue );
	CSpriteEffect &value = *pRealValue;
	value.bEnd = false;
	value.sDir = pDir->GetValue();
	value.sPos = pPosition->GetValue();
	value.sSize = pSize->GetValue();
	value.sCamera = SafeUnhomogen( pCamera->GetValue() );
}

} // NAMESPACE

using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0xB4414160, CSpriteEffect )
REGISTER_SAVELOAD_CLASS( 0xB4414161, CSpriteAnimator )

