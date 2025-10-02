#ifndef __UI_COMPONENTS_H__
#define __UI_COMPONENTS_H__

#pragma ONCE


#include "UI_export.h"

// debug vars
extern int CHECK_DUPLICATE_CHILDREN;
EXTERNVAR UI_EXPORT int CHECK_UI_TEXTURES_INSTANT_LOAD;

UI_EXPORT void CheckInstantLoadTexture( const NDb::STexture *pTexture );

// Declares components used rare or mostly internally by UI

interface ISelectNotify : public virtual CObjectBase
{
	virtual void OnSelectData( CObjectBase *pData ) = 0;
};

interface IFocusNotify : public virtual CObjectBase
{
	virtual void OnFocus( const bool bFocus ) = 0;
};

#endif //__UI_COMPONENTS_H__

