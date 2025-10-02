
#pragma once


#include "UI_export.h"

// debug vars
extern int CHECK_DUPLICATE_CHILDREN;
EXTERNVAR UI_EXPORT int CHECK_UI_TEXTURES_INSTANT_LOAD;

UI_EXPORT void CheckInstantLoadTexture( const NDb::STexture *pTexture );

// Declares components used rare or mostly internally by UI

struct ISelectNotify : public virtual CObjectBase
{
	virtual void OnSelectData( CObjectBase *pData ) = 0;
};

struct IFocusNotify : public virtual CObjectBase
{
	virtual void OnFocus( const bool bFocus ) = 0;
};


