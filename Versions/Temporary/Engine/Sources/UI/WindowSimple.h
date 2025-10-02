#pragma once
#include "window.h"


// provides cliping rect to inner elements
class CWindowSimple : public CWindow
{
	OBJECT_BASIC_METHODS(CWindowSimple)
	CPtr<NDb::SWindowSimple> pInstance;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	virtual int operator&( IBinSaver &saver )
	{
		saver.Add( 1, static_cast<CWindow*>( this ) );
		saver.Add( 2, &pInstance );
		return 0;
	}
	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc )
	{
		pInstance = checked_cast<const NDb::SWindowSimple*>( pDesc )->Duplicate();
		CWindow::InitByDesc( pDesc );
	}
};


