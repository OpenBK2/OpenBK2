#pragma once
#include "..\System\DG.h"
#include "..\System\Time.hpp"

namespace NGfx
{
	class CRenderContext;
	class CTexture;
}
namespace NGScene
{
struct SPerspDirectionalDepthInfo;
void RenderClouds( NGfx::CRenderContext *pRC, const SPerspDirectionalDepthInfo &renderInfo, const SPerspDirectionalDepthInfo &dp, NGfx::CTexture *pCloud, const SHMatrix &proj );

class CCloudMover : public CFuncBase<SHMatrix>
{
	OBJECT_NOCOPY_METHODS(CCloudMover);
	ZDATA
	CDGPtr<CFuncBase<STime> > pTime;
	CVec2 vWrapSize;
	float fAngle, fSpeed; // speed in textures per second
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTime); f.Add(3,&vWrapSize); f.Add(4,&fAngle); f.Add(5,&fSpeed); return 0; }
	bool NeedUpdate() { return pTime.Refresh(); }
	void Recalc();
public:
	CCloudMover() {}
	CCloudMover( CFuncBase<STime> *_pTime, const CVec2 &_vWrapSize, float _fAngle, float _fSpeed ) 
		: pTime(_pTime), vWrapSize(_vWrapSize), fAngle(_fAngle), fSpeed(_fSpeed) {}
};
}
