#pragma once

#include "System/DG.h"
namespace NGfx
{
	class CTexture;
}

namespace NGScene
{

struct IGScene;
class CParticleEffect;

class CSpriteAnimator: public CPtrFuncBase<CParticleEffect>
{
	OBJECT_BASIC_METHODS(CSpriteAnimator);
private:	
	ZDATA
	CVec3 sDir;
	ZSKIP;
	CPtr<IGScene> pScene;
	CDGPtr<CFuncBase<CVec3> > pDir;
	CDGPtr<CFuncBase<CVec4> > pCamera;
	CDGPtr<CFuncBase<CVec3> > pPosition;
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture;
	CDGPtr<CFuncBase<CVec2> > pSize;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&sDir); f.Add(4,&pScene); f.Add(5,&pDir); f.Add(6,&pCamera); f.Add(7,&pPosition); f.Add(8,&pTexture); f.Add(9,&pSize); return 0; }

protected:
	virtual void Recalc();
	virtual bool NeedUpdate() { return pCamera.Refresh() || pPosition.Refresh() || pDir.Refresh() || pSize.Refresh(); }

public:
	CSpriteAnimator() {}
	CSpriteAnimator( IGScene *pScene, CFuncBase<CVec3> *pPosition, CFuncBase<CVec3> *pDir, CPtrFuncBase<NGfx::CTexture> *pTexture, CFuncBase<CVec2> *pSize );
};

} // NAMESPACE


