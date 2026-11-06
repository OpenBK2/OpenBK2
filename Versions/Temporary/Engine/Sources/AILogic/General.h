#pragma once

#include "System/Basic.h"
#include "System/det_map.h"
#include "System/det_set.h"

#include "AILogic_export.h"

#include <cstdint>
#include <list>

class CGeneral;
class CVec2;
class IBinSaver;
struct IGeneralDelayedTask;

namespace NDb
{
struct SMapInfo;
}

enum EResupplyType
{
	ERT_REPAIR = 0,
	ERT_RESUPPLY = 1,
	ERT_HUMAN_RESUPPLY = 2,
	ERT_MORALE = 3,
	ERT_MEDICINE = 4,

	_ERT_COUNT = 5,
};

// главная логика
class CSupremeBeing
{
public:
	AILOGIC_EXPORT int operator&( IBinSaver &saver );

private:
	;
	typedef det_map<int, CObj<CGeneral>> Generals;
	Generals generals;
	typedef std::list<CPtr<IGeneralDelayedTask>> DelayedTasks;
	DelayedTasks delayedTasks;

	det_set<int /*Link ID*/> ironmans;

public:
	AILOGIC_EXPORT void Segment();
	AILOGIC_EXPORT void Clear();

	void SetUnitVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible );

	// когда зенитка начинает атаку нашего самолета
	void SetAAVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible );

	// creates number of generals
	// каждый генерал знает о юнитах, которые являются мобильным резервом
	AILOGIC_EXPORT void Init( const NDb::SMapInfo *pMapInfo );
	// раздать юниты генералам
	AILOGIC_EXPORT void GiveNewUnitsToGenerals( const std::list<class CCommonUnit *> &pUnits,
	                                            bool bFromReinforcement = false );

	bool IsMobileReinforcement( int nPlayer, int nGroup ) const;
	void AddReinforcement( class CAIUnit *pUnit );
	AILOGIC_EXPORT struct IEnemyContainer *GetEnemyConatiner( int nPlayer );

	// Strategic work runs in single player and COOP, never in CLASSIC MP.
	bool IsActive() const
	{
		return !generals.empty();
	}

	bool MustShootToObstacles( const int nPlayer );
	void RegisterDelayedTask( struct IGeneralDelayedTask *pTask );

	// для очагов сопротивления
	void UpdateEnemyUnitInfo( class CAIUnitInfoForGeneral *pInfo,
	                          const uint32_t lastVisibleTimeDelta,
	                          const CVec2 &vLastVisiblePos,
	                          const uint32_t lastAntiArtTimeDelta,
	                          const CVec2 &vLastVisibleAntiArtCenter,
	                          const float fDistToLastVisibleAntiArt );
	void UnitChangedParty( class CAIUnit *pUnit, const int nNewParty );
	void UnitDied( class CAIUnitInfoForGeneral *pInfo );
	void UnitDied( class CCommonUnit *pUnit );

	// when some unit changed position.
	void UnitChangedPosition( class CCommonUnit *pUnit, const CVec2 &vNewPos );
	void UnitAskedForResupply( class CCommonUnit *pUnit, const EResupplyType eType, const bool bSet );

	void AddIronman( const int nScriptGroup );
	bool IsIronman( const int nScriptGroup ) const;

	bool IsInResistanceCircle( const CVec2 &vPoint, const int nGeneralParty );
};
