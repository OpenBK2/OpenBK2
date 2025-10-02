#pragma once

#include "Stats_B2_M1/StatusUpdates.h"

class CUpdatableObj;

class CStatusUpdatesHelper
{
	ZDATA
private:
		EUnitStatus __eStatus;
		CPtr<CUpdatableObj> __pUnit;
		float __fRadius;
		bool __bInited;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&__eStatus); f.Add(3,&__pUnit); f.Add(4,&__fRadius); f.Add(5,&__bInited); return 0; }
protected:
	void SetUnit( CUpdatableObj *_pUnit ) { __pUnit = _pUnit; }
	void SetStatus( const EUnitStatus _eStatus );
public:

	CStatusUpdatesHelper() : __eStatus( EUS_UNDEFINED ), __pUnit( 0 ), __fRadius( 0.0f ), __bInited( false ) {}
	
	CStatusUpdatesHelper( const EUnitStatus _eStatus );
	CStatusUpdatesHelper( const EUnitStatus _eStatus, CUpdatableObj *_pUnit );
	CStatusUpdatesHelper( const EUnitStatus _eStatus, CUpdatableObj *_pUnit, const float _fRadius );

	~CStatusUpdatesHelper();

	void InitStatus();
};


