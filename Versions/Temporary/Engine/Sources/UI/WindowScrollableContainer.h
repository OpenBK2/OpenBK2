#pragma once
#include "WindowScrollableContainerBase.h"

// may contain any number of windows, arrange them in column with given idention.
// allow to scroll to view all items
class CWindowScrollableContainer : public CWindowScrollableContainerBase
{
	OBJECT_BASIC_METHODS( CWindowScrollableContainer )
	CPtr<NDb::SWindowScrollableContainer> pInstance;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	int operator&( IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;
	void Select( IWindow *pElement ) override;
};
