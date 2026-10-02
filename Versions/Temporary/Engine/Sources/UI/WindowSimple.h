#pragma once
#include "Window.h"


// provides cliping rect to inner elements
class CWindowSimple : public CWindow
{
	OBJECT_BASIC_METHODS(CWindowSimple)
	CPtr<NDb::SWindowSimple> pInstance;
	bool bScrollViewport = false;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	// The scroll border clips rendering intentionally, not the text layout.
	void SetScrollViewport() { bScrollViewport = true; }
	virtual int operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CWindow*>( this ) );
		saver.Add( 2, &pInstance );
		saver.Add( 3, &bScrollViewport );
		return 0;
	}
	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc )
	{
		pInstance = checked_cast<const NDb::SWindowSimple*>( pDesc )->Duplicate();
		CWindow::InitByDesc( pDesc );
	}
};


