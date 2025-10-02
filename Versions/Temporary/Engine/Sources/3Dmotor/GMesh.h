#ifndef __GMESH_H_
#define __GMESH_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GSkeleton.h"

namespace NGScene
{

class CMeshBound: public CFuncBase<SBound>
{
	OBJECT_BASIC_METHODS(CMeshBound);
	CDGPtr< CFuncBase<SSkeletonMatrices> > pAnimation;
protected:
	virtual bool NeedUpdate() { return pAnimation.Refresh(); }
	virtual void Recalc();
public:
	CMeshBound( CFuncBase<SSkeletonMatrices> *_pAnimation = 0 ): pAnimation(_pAnimation) {}
	int operator&( CStructureSaver &f );
};

} // namespace

#endif

