#pragma once
#include "GfxUtils.h"

namespace NGfx
{

class CMonochromeEffect: public I2DEffect
{
	OBJECT_NOCOPY_METHODS(CMonochromeEffect);
public:
	virtual void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV );
};

class CTwilightEffect: public I2DEffect
{
	OBJECT_NOCOPY_METHODS(CTwilightEffect);
private:
	float fTime;
	float fCoeff;
	NGfx::CTexture *pGlow;
	NGfx::CTexture *pMask;
	NGfx::CTexture *pNoise;

public:
	CTwilightEffect() {}
	CTwilightEffect( float _fCoeff, float _fTime, NGfx::CTexture *_pNoise, NGfx::CTexture *_pGlow, NGfx::CTexture *_pMask ): fCoeff(_fCoeff), fTime(_fTime), pNoise(_pNoise), pGlow(_pGlow), pMask(_pMask) {}
	void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV );
};

class CGausianBlurEffect: public I2DEffect
{
	OBJECT_NOCOPY_METHODS(CGausianBlurEffect);
private:
	bool bVert;
public:
	CGausianBlurEffect( bool _bVert = false ): bVert( _bVert ) {}
	void SetEffect( NGfx::CRenderContext *pRC, NGfx::CTexture *pTex, float fScaleU, float fScaleV );
};

void ApplyFilters( int nDownSample, const vector<CPtr<I2DEffect> > &filters );
void ApplyFilters( NGfx::CTexture *pTarget, const CTRect<float> &targetRect, int nDownSample, const vector<CPtr<I2DEffect> > &filters );

}


