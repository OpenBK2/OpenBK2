#pragma once

#include "Common_RTS_AI_export.h"


#include "BasePathUnit.h"

COMMON_RTS_AI_EXPORT interface IStaticPath* CreateStaticPathToPoint( const CVec2 &finishPoint, const CVec2 &vShift, CBasePathUnit *pUnit, const bool bCanGoOutOfRadius, CAIMap *pAIMap );
COMMON_RTS_AI_EXPORT interface IStaticPath* CreateStaticPathToPoint( const CVec2 &startPoint, const CVec2 &finishPoint, const CVec2 &vShift, CBasePathUnit *pUnit, const bool bCanGoOutOfRadius, CAIMap *pAIMap );
COMMON_RTS_AI_EXPORT interface IPath* CreatePathByDirection( const CVec2 &startPoint, const CVec2 &dir, const CVec2 &finishPoint, CAIMap *pAIMap );


