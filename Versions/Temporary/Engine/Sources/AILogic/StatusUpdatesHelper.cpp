#include "stdafx.h"

#include "NewUpdater.h"
#include "StatusUpdatesHelper.h"

extern CEventUpdater updater;

CStatusUpdatesHelper::CStatusUpdatesHelper( const EUnitStatus _eStatus )
: __eStatus( _eStatus ), __pUnit( 0 ), __fRadius( 0.0f ), __bInited( false )
{
}

CStatusUpdatesHelper::CStatusUpdatesHelper( const EUnitStatus _eStatus, CUpdatableObj *_pUnit )
: __eStatus( _eStatus ), __pUnit( _pUnit ), __fRadius( 0.0f ), __bInited( false )
{
	NI_ASSERT( IsValid( __pUnit ), "CStatusUpdatesHelper created with invalid unit" );
}

CStatusUpdatesHelper::CStatusUpdatesHelper( const EUnitStatus _eStatus, CUpdatableObj *_pUnit, const float _fRadius )
: __eStatus( _eStatus ), __pUnit( _pUnit ), __fRadius( _fRadius ), __bInited( false )
{
	NI_ASSERT( IsValid( __pUnit ), "CStatusUpdatesHelper created with invalid unit" );
}

void CStatusUpdatesHelper::InitStatus()
{
	if ( !__bInited && __eStatus != EUS_UNDEFINED && IsValid( __pUnit ) )
	{
		updater.AddUpdate( CreateStatusUpdate( __eStatus, true, __fRadius ), ACTION_NOTIFY_UPDATE_STATUS, __pUnit, -1 );
		__bInited = true;
	}
}

CStatusUpdatesHelper::~CStatusUpdatesHelper()
{
	if ( __bInited && __eStatus != EUS_UNDEFINED && IsValid( __pUnit ) )
		updater.AddUpdate( CreateStatusUpdate( __eStatus, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, __pUnit, -1 );
}

void CStatusUpdatesHelper::SetStatus( const EUnitStatus _eStatus )
{
	if ( !__bInited )
		__eStatus = _eStatus;
	else if ( __eStatus != __eStatus )
	{
		updater.AddUpdate( CreateStatusUpdate( __eStatus, false, 0.0f ), ACTION_NOTIFY_UPDATE_STATUS, __pUnit, -1 );
		__eStatus = __eStatus;
		updater.AddUpdate( CreateStatusUpdate( __eStatus, true, __fRadius ), ACTION_NOTIFY_UPDATE_STATUS, __pUnit, -1 );
	}
}


