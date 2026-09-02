#pragma once

#include "Stats_B2_M1/RPGStats.h"

struct SPointListDialogData
{
	int nInstanceID;

	int nNumPoints;
	int nSelectedPoint;
	bool bChkPassability;
	bool bChkPropmask;
	NDb::ESeason eSeason;
	
	SPointListDialogData() :
		nInstanceID(-1),
		nNumPoints(0),
		nSelectedPoint(-1),
		bChkPassability(false),
		bChkPropmask(false),
		eSeason(NDb::ESeason(-1))
	{
	}
};

struct SFormationWindowDialogData
{
	NDb::SSquadRPGStats::SFormation::EFormationMoveType eSelectedFormation;
	std::vector<NDb::SSquadRPGStats::SFormation::EFormationMoveType> squadFormations;
	bool bChkPropmask;
    
	SFormationWindowDialogData() :
		eSelectedFormation(NDb::SSquadRPGStats::SFormation::EFormationMoveType(-1)),
		bChkPropmask(false)
	{
	}

	void AddFormation( NDb::SSquadRPGStats::SFormation::EFormationMoveType e )
	{
		for ( int k = 0; k < squadFormations.size(); ++k )
		{
			if ( squadFormations[k] == e )
				return;
		}
		squadFormations.push_back(e);
	}
};


