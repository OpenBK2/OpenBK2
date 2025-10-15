#include "stdafx.h"

#include "PlanePath.h"
#include "Aviation.h"
#include "PlanesFormation.h"

#include "AILogic_export.h"

#include <float.h>

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x1508D48F, CPlaneInFormationSmoothPath );
extern NTimer::STime curTime;

//*******************************************************************
//*												CPlaneInFormationSmoothPath*
//*******************************************************************

const bool CPlaneInFormationSmoothPath::CanGoBackward() const 
{ 
	return false; 
}

void CPlaneInFormationSmoothPath::Init( class CAviation *_pOwner ) 
{ 
	pOwner = _pOwner; 
	pFormation = pOwner->GetPlanesFormation(); 
}

bool CPlaneInFormationSmoothPath::IsFinished() const 
{ 
	return false; 
}

void CPlaneInFormationSmoothPath::Segment( NTimer::STime timeDiff ) 
{ 
	const CVec3 vNewPoint( pFormation->GetPosNext() );
	pOwner->SetCenter( vNewPoint );
}


