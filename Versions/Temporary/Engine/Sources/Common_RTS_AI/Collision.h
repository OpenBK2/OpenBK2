#pragma once

#include "Common_RTS_AI_export.h"


class CAIMap;
class CBasePathUnit;

namespace NCollision
{
	enum ECollisionName
	{
		ECN_FREE								= 0x31223AC0,
		ECN_GIVE_PLACE					= 0x31223AC1,
		ECN_GIVE_PLACE_ROTATE		= 0x31223AC2,
		ECN_WAIT								= 0x31223AC3,
		ECN_OVERTAKE_FORWARD		= 0x31223AC4,
		ECN_OVERTAKE_BACK				= 0x31223AC5,
		ECN_STOP								= 0x31223AC6,
	};

	enum ECollideType
	{
		ECT_NONE 							= 0x00000000, // no collisions
		ECT_FIRST_WAIT 				= 0x00000101, // first unit waits for second
		ECT_SECOND_WAIT 			= 0x00000102, // second unit waits for first
		ECT_FIRST_GIVEPLACE 	= 0x00000201, // first unit gives place for second
		ECT_SECOND_GIVEPLACE 	= 0x00000202, // second unit gives place for first
		ECT_FIRST_HARD 				= 0x00000401, // first unit gives place for second (units too close to each other)
		ECT_SECOND_HARD 			= 0x00000402, // second unit gives place for first (units too close to each other)

		ECT_FIRST							= 0x00000001,
		ECT_SECOND						= 0x00000002,

		ECT_WAIT							= 0x00000100,
		ECT_GIVEPLACE					= 0x00000200,
		ECT_HARD							= 0x00000400,

		ECT_UNIT_MASK					= 0x000000FF,
		ECT_TYPE_MASK					= 0x0000FF00,
	};
}

interface ICollisionOld : public CAIObjectBase
{
	virtual const int GetPriority() const = 0;
	virtual CBasePathUnit* GetPushUnit() const = 0;

	virtual int FindCandidates( class CCollisionsCollector *pCollCollector ) = 0;
	virtual bool IsSolved() = 0;

	virtual NCollision::ECollisionName GetName() const = 0;

	virtual void Segment( const NTimer::STime timeDiff ) = 0;
};

interface ICollisionsCollector : public CAIObjectBase
{
	virtual void AddCollision( class CBasePathUnit *pUnit1, class CBasePathUnit *pUnit2, const float fDistance, const NCollision::ECollideType eCollideType ) = 0;
	virtual void HandOutCollisions( CAIMap *pAIMap ) = 0;
};

interface ICollision : public CAIObjectBase
{
	virtual void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority ) = 0;

	virtual const int GetPriority() const = 0;
	virtual CBasePathUnit* GetPushUnit() const = 0;

	virtual const bool IsSolved() const = 0;
	virtual void Segment( const NTimer::STime timeDiff ) = 0;
	virtual void FindCandidates( ICollisionsCollector *pCollisionCollector ) = 0;

	virtual const NCollision::ECollisionName GetName() const = 0;
};

COMMON_RTS_AI_EXPORT ICollisionsCollector *CreateCollisionsCollector();
COMMON_RTS_AI_EXPORT ICollision *CreateCollision( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const NCollision::ECollisionName eName );

