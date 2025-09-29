#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../Common_RTS_AI/Collision.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											Коллизии для самолётов											*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CPlaneCollision : public ICollision
{
	OBJECT_BASIC_METHODS( CPlaneCollision );
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }
public:	
	void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority ) {}

	const int GetPriority() const { return 0; }
	CBasePathUnit* GetPushUnit() const { return 0; }

	const bool IsSolved() const { return false; }
	void Segment( const NTimer::STime timeDiff ) {}
	void FindCandidates( ICollisionsCollector *pCollisionCollector ) {}

	const NCollision::ECollisionName GetName() const { return NCollision::ECN_FREE; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
