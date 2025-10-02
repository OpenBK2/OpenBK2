#ifndef __GENERAL__
#define __GENERAL__

class CGeneral;
interface IGeneralDelayedTask;
namespace NDb
{
	struct SMapInfo; 
}

enum EResupplyType
{
	ERT_REPAIR						= 0,
	ERT_RESUPPLY					= 1,
	ERT_HUMAN_RESUPPLY		= 2,
	ERT_MORALE						= 3,
	ERT_MEDICINE					= 4,
	
	_ERT_COUNT						= 5,
};

// главная логика
class CSupremeBeing
{
	public: int operator&( IBinSaver &saver ); private:;
	typedef hash_map<int, CObj<CGeneral> > Generals;
	Generals generals;
	typedef list< CPtr<IGeneralDelayedTask> > DelayedTasks;
	DelayedTasks delayedTasks;

	hash_set<int/*Link ID*/> ironmans;

public:
	void Segment();
	void Clear();

	void SetUnitVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible );
	
	// когда зенитка начинает атаку нашего самолета
	void SetAAVisible( class CAIUnit *pUnit, const int nGeneralParty, const bool bVisible );
	
	// creates number of generals
	// каждый генерал знает о юнитах, которые являются мобильным резервом
	void Init( const NDb::SMapInfo* pMapInfo );
	// раздать юниты генералам
	void GiveNewUnitsToGenerals( const list<class CCommonUnit*> &pUnits, bool bFromReinforcement = false );

	bool IsMobileReinforcement( int nParty, int nGroup ) const;
	void AddReinforcement( class CAIUnit *pUnit );
	interface IEnemyContainer* GetEnemyConatiner( int nParty );
	
	bool MustShootToObstacles( const int nPlayer );
	void RegisterDelayedTask( interface IGeneralDelayedTask *pTask );
	
	// для очагов сопротивления
	void UpdateEnemyUnitInfo( class CAIUnitInfoForGeneral *pInfo,
		const NTimer::STime lastVisibleTimeDelta, const CVec2 &vLastVisiblePos,
		const NTimer::STime lastAntiArtTimeDelta, const CVec2 &vLastVisibleAntiArtCenter, const float fDistToLastVisibleAntiArt );
	void UnitChangedParty( class CAIUnit *pUnit, const int nNewParty );
	void UnitDied( class CAIUnitInfoForGeneral *pInfo );
	void UnitDied( class CCommonUnit * pUnit );
	
	// when some unit changed position.
	void UnitChangedPosition( class CCommonUnit * pUnit, const CVec2 &vNewPos );
	void UnitAskedForResupply( class CCommonUnit * pUnit, const EResupplyType eType, const bool bSet );

	void AddIronman( const int nScriptGroup );
	bool IsIronman( const int nScriptGroup ) const;

	bool IsInResistanceCircle( const CVec2 &vPoint, const int nGeneralParty );
};

#endif // __GENERAL__
