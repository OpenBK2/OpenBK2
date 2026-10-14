#pragma once
#include "Window.h"

// provides cliping rect to inner elements
class CWindowSimple : public CWindow
{
	OBJECT_BASIC_METHODS( CWindowSimple )
	CPtr<NDb::SWindowSimple> pInstance;
	bool bScrollViewport = false;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	// The scroll border clips rendering intentionally, not the text layout.
	void SetScrollViewport()
	{
		bScrollViewport = true;
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, static_cast<CWindow *>( this ) );
		saver.Add( 2, &pInstance );
		saver.Add( 3, &bScrollViewport );
		return 0;
	}

	void Visit( struct IUIVisitor *pVisitor ) override;

	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override
	{
		pInstance = checked_cast<const NDb::SWindowSimple *>( pDesc )->Duplicate();
		CWindow::InitByDesc( pDesc );
	}
};
