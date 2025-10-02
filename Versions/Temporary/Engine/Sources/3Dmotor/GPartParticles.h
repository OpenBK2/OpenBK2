#pragma once
#include "GTransparent.h"
#include "GScene.h"

namespace NGScene
{

class CVolumeNode;
class CParticles: public IParticles
{
	OBJECT_NOCOPY_METHODS(CParticles);
	ZDATA
	SBound bound;
	CDGPtr< CPtrFuncBase<CParticleEffect> > pParticles;
	CDGPtr< CFuncBase<SFBTransform> > pPlacement;
	CPtr<CVolumeNode> pNode;
	SGroupInfo groupInfo;
	int nFlags;
	SBound transformedBound;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bound); f.Add(3,&pParticles); f.Add(4,&pPlacement); f.Add(5,&pNode); f.Add(6,&groupInfo); f.Add(7,&nFlags); f.Add(8,&transformedBound); return 0; }
public:
	CParticles() {}
	CParticles( CPtrFuncBase<CParticleEffect> *_pParticles, CFuncBase<SFBTransform> *_pPlacement, 
		const SBound &_bound, const SGroupInfo &_g, int _nFlags )
		: pParticles(_pParticles), pPlacement(_pPlacement), bound(_bound), groupInfo(_g), nFlags(_nFlags) {}
	const SGroupInfo& GetGroup() const { return groupInfo; }
	const SBound& GetBound() const { return transformedBound; }
	CParticleEffect* GetEffect();
	void Unlink();
	void SetFade( float fVal );
	bool Update( CVolumeNode *pVolume );
	bool IsLit() const { return ( nFlags & PF_LIT ) != 0; }
	bool IsDynamic() const { return ( nFlags & PF_DYNAMIC ) != 0; }
	CPtrFuncBase<CParticleEffect> *GetAnimator() const { return pParticles; }
};
}
