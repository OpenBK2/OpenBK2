#include "stdafx.h"

#include "PathFinder.h"
#include "StandartDirPath.h"
#include "..\Common_RTS_AI\AIMap.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IStaticPath* CreateStaticPathToPoint( const CVec2 &_finishPoint, const CVec2 &vShift, CBasePathUnit *pUnit, const bool bCanGoOutOfRadius, CAIMap *pAIMap )
{
	CVec2 finishPoint( _finishPoint + vShift );
	if ( !pAIMap->IsPointInside( finishPoint ) )
		finishPoint = _finishPoint;
	if ( !bCanGoOutOfRadius && !pUnit->CanGoToPoint( _finishPoint ) )
		return 0;
	
	return 
		pUnit->CreateBigStaticPath( pUnit->GetCenterPlain(), finishPoint, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IStaticPath* CreateStaticPathToPoint( const CVec2 &_startPoint, const CVec2 &_finishPoint, const CVec2 &vShift, CBasePathUnit *pUnit, const bool bCanGoOutOfRadius, CAIMap *pAIMap )
{
	CVec2 finishPoint( _finishPoint + vShift );
	if ( !pAIMap->IsPointInside( finishPoint ) )
		finishPoint = _finishPoint;
	if ( !bCanGoOutOfRadius && !pUnit->CanGoToPoint( finishPoint ) )
		return 0;

	return 
		pUnit->CreateBigStaticPath( _startPoint, finishPoint, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPath* CreatePathByDirection( const CVec2 &startPoint, const CVec2 &dir, const CVec2 &finishPoint, CAIMap *pAIMap )
{
	return new CStandartDirPath( startPoint, dir, finishPoint, pAIMap->GetTileSize() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
