#pragma once
#include "GSkeleton.h"

namespace granny {
	struct world_pose;
}

typedef granny::world_pose granny_world_pose;

namespace NGScene
{

class CBind : public CFuncBase<SSkeletonMatrices>
{
	OBJECT_NOCOPY_METHODS(CBind);

	ZDATA
	CDGPtr< CFuncBase<NAnimation::SGrannySkeletonPose> > pAnimation;
	NAnimation::SGrannySkeletonHandle skeletonH;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pAnimation); f.Add(3,&skeletonH); return 0; }

	granny_world_pose *pGlobal;
	CDGPtr<CPtrFuncBase<NAnimation::CGrannyFileInfo> > pSkeletonFileLoader;

protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh(); }
	virtual void Recalc();
	~CBind();
public:
	CBind() : pGlobal(0) {}
	CBind( CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation, const NAnimation::SGrannySkeletonHandle &_skeletonH );
	CBind( CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation, const NDb::SSkeleton *pSkeleton, int _nModelInFile );
};

class CAnimatedBound : public CFuncBase<SBound>
{
	OBJECT_NOCOPY_METHODS(CAnimatedBound);
	ZDATA
	SBound bv;
	CDGPtr< CFuncBase<NAnimation::SGrannySkeletonPose> > pAnimation;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bv); f.Add(3,&pAnimation); return 0; }
	SSphere prevValue;
protected:
	virtual bool NeedUpdate();
	virtual void Recalc();
public:
	CAnimatedBound() { Zero( value ); Zero( prevValue ); }
	CAnimatedBound( const SBound &_bv, CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation );
};

void DiscretisizeBoundSphere( SSphere *pResult, const CVec3 &ptCenter, const float fRadius, const float fDiscrStep );

} // namespace


