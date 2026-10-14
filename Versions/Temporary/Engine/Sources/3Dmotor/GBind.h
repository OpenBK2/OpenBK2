#pragma once
#include "GSkeleton.h"

namespace NGScene
{

class CBind : public CFuncBase<SSkeletonMatrices>
{
	OBJECT_NOCOPY_METHODS( CBind );

	ZDATA
	CDGPtr<CFuncBase<NAnimation::SSkeletonPose>> pAnimation;
	NAnimation::SSkeletonHandle skeletonH;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pAnimation );
		f.Add( 3, &skeletonH );
		return 0;
	}

protected:
	bool NeedUpdate() override
	{
		return pAnimation.Refresh();
	}

	void Recalc() override;
	~CBind() override;

public:
	CBind() {}

	CBind( CFuncBase<NAnimation::SSkeletonPose> *_pAnimation, const NAnimation::SSkeletonHandle &_skeletonH );
	CBind( CFuncBase<NAnimation::SSkeletonPose> *_pAnimation, const NDb::SSkeleton *pSkeleton, int _nModelInFile );
};

class CAnimatedBound : public CFuncBase<SBound>
{
	OBJECT_NOCOPY_METHODS( CAnimatedBound );
	ZDATA
	SBound bv;
	CDGPtr<CFuncBase<NAnimation::SSkeletonPose>> pAnimation;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &bv );
		f.Add( 3, &pAnimation );
		return 0;
	}

	SSphere prevValue;

protected:
	bool NeedUpdate() override;
	void Recalc() override;

public:
	CAnimatedBound()
	{
		Zero( value );
		Zero( prevValue );
	}

	CAnimatedBound( const SBound &_bv, CFuncBase<NAnimation::SSkeletonPose> *_pAnimation );
};

void DiscretisizeBoundSphere( SSphere *pResult, const CVec3 &ptCenter, const float fRadius, const float fDiscrStep );

} // namespace NGScene
