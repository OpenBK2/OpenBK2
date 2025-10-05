#pragma once
#include "System/time.hpp"
#include "GParticleInfo.h"
namespace NGScene
{

class IParticleFilter;
class CRainAnimator : public CPtrFuncBase<CParticleEffect>
{
	OBJECT_BASIC_METHODS(CRainAnimator);
protected:
	virtual bool NeedUpdate() { return pCamera.Refresh() | pTime.Refresh(); }
	virtual void Recalc();
private:	
	ZDATA
	CDGPtr< CFuncBase<STime> > pTime;
	CDGPtr<CFuncBase<CVec4> > pCamera;
	CObj<IParticleFilter> pFilter;
public:
	std::vector<CObj<CPtrFuncBase<NGfx::CTexture> > > textureIDs;
private:
	STime tStart;
public:
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pTime); f.Add(3,&pCamera); f.Add(4,&pFilter); f.Add(5,&textureIDs); f.Add(6,&tStart); return 0; }

	CRainAnimator() {}
	CRainAnimator( CFuncBase<STime> *_pTime, CFuncBase<CVec4> *_pCamera, IParticleFilter *_pFilter ) : pTime(_pTime), pCamera(_pCamera), pFilter(_pFilter), tStart(0) {}
};
}

