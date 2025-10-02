#pragma once

#include "SceneB2_export.h"

#include "../3DLib/GGeometry.h"
#include "../System/DG.h"

class SCENEB2_EXPORT CLaserMarkTrace :	public CPtrFuncBase<NGScene::CObjectInfo>
{
	OBJECT_NOCOPY_METHODS( CLaserMarkTrace )

	ZDATA
		CVec3 vStart;
		CVec3 vEnd;
		CVec3 vNorm;
		CVec3 vNorm2;
		bool bNeedUpdate;
		NTimer::STime timeStart;
		CDGPtr< CFuncBase<STime> > pTimer;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vStart); f.Add(3,&vEnd); f.Add(4,&vNorm); f.Add(5,&bNeedUpdate); return 0; }

protected:
	void Recalc();
	bool NeedUpdate() { return pTimer != 0 ? pTimer.Refresh() : false; }

public:
	CLaserMarkTrace() {}
	CLaserMarkTrace( const CVec3 &vStart, const CVec3 &vEnd, CFuncBase<STime> *_pTimer )
		: timeStart( 0 ),
		pTimer( _pTimer )
	{
		timeStart = pTimer->GetValue();
		UpdatePoints( vStart, vEnd );
	}

	void UpdatePoints( const CVec3 &vStart, const CVec3 &vEnd );
};


