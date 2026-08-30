#include "stdafx.h"
#include "GAnimUtils.h"

#include "3Dmotor_export.h"

namespace NAnimation
{

// CAddBoneFilter

void CAddBoneFilter::Recalc()
{
	pAnimation.Refresh();
	const SSkeletonPose &pose = pAnimation->GetValue();
	if ( nAddBone < 0 || nAddBone >= static_cast<int>(pose.worldPose.size()) )
		return;
	value.forward = pose.worldPose[nAddBone];
	value.backward.HomogeneousInverse( value.forward );
}

CAddBoneFilter::~CAddBoneFilter()
{
}

}
using namespace NAnimation;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x10441190, CAddBoneFilter )

