#pragma once

#include "Common_RTS_AI/Collision.h"

//*******************************************************************
//*											Коллизии для самолётов											*
//*******************************************************************

class CPlaneCollision : public ICollision
{
	OBJECT_BASIC_METHODS( CPlaneCollision );

	ZDATA
	ZEND int operator&( IBinSaver &f ) override
	{
		return 0;
	}

public:
	void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority ) override {}

	const int GetPriority() const override
	{
		return 0;
	}

	CBasePathUnit *GetPushUnit() const override
	{
		return 0;
	}

	const bool IsSolved() const override
	{
		return false;
	}

	void Segment( const NTimer::STime timeDiff ) override {}

	void FindCandidates( ICollisionsCollector *pCollisionCollector ) override {}

	const NCollision::ECollisionName GetName() const override
	{
		return NCollision::ECN_FREE;
	}
};
