#include "stdafx.h"

#include "PlanePath.h"
#include "TankPitPath.h"
#include "PresizePath.h"
#include "ArtilleryPaths.h"
#include "SerializeOwner.h"
#include "../Common_RTS_AI/BasePathUnit.h"

int CPlaneInFormationSmoothPath::operator&( IBinSaver &saver )
{
	SerializeOwner( 1, &pFormation, &saver );
	SerializeOwner( 2, &pOwner, &saver );
	return 0;
}

void CArtilleryCrewPath::OnSerialize( IBinSaver &saver )
{
	SerializeBasePathUnit( saver, 2, &pUnit );
}

int CTankPitPath::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vCurPoint );
	saver.Add( 3, &vEndPoint );
	saver.Add( 4, &fSpeedLen );
  SerializeBasePathUnit( saver, 5, &pUnit );
	return 0;
}

void CPresizePath::OnSerialize( IBinSaver &saver )
{
  SerializeBasePathUnit( saver, 2, &pUnit );
}


