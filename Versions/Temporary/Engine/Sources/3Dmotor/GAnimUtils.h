#pragma once
#include "GSkeleton.h"

struct granny_world_pose;
namespace NAnimation
{
class CGrannyFileInfo;

// filter for additional bones
class CAddBoneFilter : public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS(CAddBoneFilter);

private:
	ZDATA
	CDGPtr< CFuncBase<SGrannySkeletonPose> > pAnimation;
	NAnimation::SGrannySkeletonHandle skelHandle;
	int nAddBone;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimation); f.Add(3,&skelHandle); f.Add(4,&nAddBone); return 0; }
	granny_world_pose *pGlobal;
	CDGPtr<CPtrFuncBase<CGrannyFileInfo> > pSkeletonFileLoader;

protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh(); }
	virtual void Recalc();
public:
	CAddBoneFilter() : nAddBone(0), pGlobal(0) {}
	CAddBoneFilter( CFuncBase<SGrannySkeletonPose> *_pAnim, const NAnimation::SGrannySkeletonHandle &_skel, int _nAddBone ) 
		: pAnimation(_pAnim), skelHandle(_skel), nAddBone(_nAddBone), pGlobal(0)
	{}
	~CAddBoneFilter();
};
}
