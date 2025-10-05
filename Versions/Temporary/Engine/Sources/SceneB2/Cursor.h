#pragma once

#include "SceneB2_export.h"


struct ICursor : public CObjectBase
{
	enum { tidTypeID = 0x3014EC00 };

	virtual bool Init() = 0;
	// cursor mode
	virtual void RegisterMode( const int nMode, const std::string &szFileName ) = 0;
	virtual bool SetMode( const int nMode ) = 0;
	virtual void OnSetCursor() = 0;
	// show/hide cursor
	virtual void Show( const bool bShow ) = 0;
	// set movement bounds
	virtual void SetBounds( const int x1, const int y1, const int x2, const int y2 ) = 0;
	// acquire control over cursor
	virtual void Acquire( const bool bAcquire ) = 0;
	// direct set cursor position
	virtual void SetPos( const int nX, const int nY ) = 0;
	// get current (!) cursor position
	virtual const CVec2 GetPos() const = 0;

	virtual void CanShow( const bool bCanShow ) = 0;
};

SCENEB2_EXPORT ICursor *CreateCursor();
inline ICursor* Cursor() { return Singleton<ICursor>(); }


