#pragma once

#include "3Dmotor/GView.h"
#include "Stats_B2_M1/RPGStats.h"
#include "Camera.h"

class CShotTraceObj :	public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CShotTraceObj )

	ZDATA
		CVec3 vStart, vEnd;
		float fSpeed;
		NTimer::STime timeStart;
		CDGPtr< CFuncBase<STime> > pTimer;
		float fHalfWidth;
		float fTimeTotal, fTimeQuant;
		const NDb::SWeaponRPGStats::SShell *pShellStats;
		CVec3 vDir;
		CVec3 vCameraAnchor, vCameraEye, vViewNormal;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vStart); f.Add(3,&vEnd); f.Add(4,&fSpeed); f.Add(5,&timeStart); f.Add(6,&pTimer); return 0; }

	void Recalc();
	bool NeedUpdate() { return pTimer != 0 ? pTimer.Refresh() : false; }

	bool WasCameraMoved() const
	{
		return ( (fabs(vCameraAnchor - Camera()->GetAnchor()) > FP_EPSILON) ||
						 (fabs(vCameraEye - Camera()->GetPos()) > FP_EPSILON) );
	}
	void UpdateViewNormal()
	{
		vCameraAnchor = Camera()->GetAnchor();
		vCameraEye = Camera()->GetPos();
		//
		CVec3 vViewDir = vCameraAnchor - vCameraEye;
		::Normalize( &vViewDir );
		//
		vViewNormal = vDir ^ vViewDir;
		::Normalize( &vViewNormal );
	}

public:
	CShotTraceObj()
		:pShellStats( 0 )
	{}
	CShotTraceObj(	const CVec3 &_vStart, const CVec3 &_vEnd, NTimer::STime _timeStart, const NDb::SWeaponRPGStats::SShell *pShell, CFuncBase<STime> *_pTimer )
		: vStart( _vStart ),
		vEnd( _vEnd ),
		pShellStats( pShell ),
		fSpeed( pShell->fSpeed * pShell->fTraceSpeedCoeff ),
		fHalfWidth( AI2Vis(pShell->fTraceWidth / 2.0f) ),
		timeStart( _timeStart ),
		pTimer( _pTimer )
	{
		fTimeTotal = Vis2AI( fabs(vEnd - vStart) ) / fSpeed;	// time, bullet flies from gun to target
		fTimeQuant = pShellStats->fTraceLength / fSpeed;			// trace quant pass time
		//
		vDir = vEnd - vStart;
		::Normalize( &vDir );
		//
		UpdateViewNormal();
	}
};


