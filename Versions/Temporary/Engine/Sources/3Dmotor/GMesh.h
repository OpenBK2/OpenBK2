#pragma once
#include "GSkeleton.h"

namespace NGScene
{

class CMeshBound : public CFuncBase<SBound>
{
	OBJECT_BASIC_METHODS( CMeshBound );
	CDGPtr<CFuncBase<SSkeletonMatrices>> pAnimation;

protected:
	bool NeedUpdate() override
	{
		return pAnimation.Refresh();
	}

	void Recalc() override;

public:
	CMeshBound( CFuncBase<SSkeletonMatrices> *_pAnimation = 0 )
		: pAnimation( _pAnimation )
	{
	}

	int operator&( CStructureSaver &f ) override;
};

} // namespace NGScene
