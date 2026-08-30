#pragma once
#include "GSkeleton.h"

namespace NAnimation
{

// filter for additional bones
class CAddBoneFilter : public CFuncBase<SFBTransform>
{
	OBJECT_NOCOPY_METHODS(CAddBoneFilter);

private:
	ZDATA
	CDGPtr< CFuncBase<SSkeletonPose> > pAnimation;
	NAnimation::SSkeletonHandle skelHandle;
	int nAddBone;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimation); f.Add(3,&skelHandle); f.Add(4,&nAddBone); return 0; }

protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh(); }
	virtual void Recalc();
public:
	CAddBoneFilter() : nAddBone(0) {}
	CAddBoneFilter( CFuncBase<SSkeletonPose> *_pAnim, const NAnimation::SSkeletonHandle &_skel, int _nAddBone )
		: pAnimation(_pAnim), skelHandle(_skel), nAddBone(_nAddBone)
	{}
	~CAddBoneFilter();
};
}

