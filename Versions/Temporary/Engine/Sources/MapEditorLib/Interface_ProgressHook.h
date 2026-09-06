#pragma once

#include "Interface_Controller.h"
#include "Interface_Widget.h"

#include <cstdint>

// NOTE: there is a second, unrelated `struct IProgressHook` in Misc/Progress.h,
// which is the game's. The two have never been the same type and only the two
// files under MapEditor/ include this one.
struct IProgressHook : public CObjectBase
{
	enum { tidTypeID = 0x1B24AB80 };

	//std::string szProgressName;

	// pParentWidget is the front-end window the progress display should belong
	// to, carried through as an opaque handle.
	virtual void Create( const std::string &rszName, IWidget *pParentWidget ) = 0;
	//virtual void GetName( std::string *pName ) const { (*pName) = szProgressName; }
	//
	virtual void SetCancel( bool bHasCancel ) = 0;
	virtual bool HasCancel() const = 0;
	virtual bool WasCancelled() const = 0;
	//
	virtual void SetProgressRange( int nStart, int nFinish ) = 0;
	virtual void SetProgressPosition( int nPosition ) = 0;
	virtual void IterateProgressPosition() = 0;
	//
	virtual void ClearLog() = 0;
	virtual void AddLog( const std::string &rzsLogMessage ) = 0;
};



