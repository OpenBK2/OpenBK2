#pragma once
#include "../System/DG.h"
#include "../3dlib/ExpFader.h"

namespace NGfx
{
	class CTexture;
}

namespace NGScene
{
class CParticleEffect;
interface IGScene;

class CPointGlowAnimator : public CPtrFuncBase<CParticleEffect>
{
	OBJECT_BASIC_METHODS(CPointGlowAnimator );
protected:
	virtual bool NeedUpdate() { return pTime.Refresh() | pPlacement.Refresh(); }
	virtual void Recalc();
private:	
	ZDATA
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr< CFuncBase<CVec3> > pPlacement;
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture;
	CPtr<IGScene> pScene;
	SExpFader expFader;
	float fLightSize;
	CDGPtr<CFuncBase<CVec4> > pCamera;
	int nLastMaskAny;
	STime tNextCheck;
	bool bIsVisible;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTime); f.Add(3,&pPlacement); f.Add(4,&pTexture); f.Add(5,&pScene); f.Add(6,&expFader); f.Add(7,&fLightSize); f.Add(8,&pCamera); f.Add(9,&nLastMaskAny); f.Add(10,&tNextCheck); f.Add(11,&bIsVisible); return 0; }
	void CalcSize();
public:
	CPointGlowAnimator() {}
	CPointGlowAnimator( IGScene *_pScene, CFuncBase<STime> *_pTime, CFuncBase<CVec3> *pPlace, 
		CPtrFuncBase<NGfx::CTexture> *pTexture, float _fLightSize, float fOnTime, float fOffTime );
	friend class CPointGlowEffect;
};
}


