#ifndef __OBSTACLE_H__
#define __OBSTACLE_H__

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// wrapper pattern
// оболочка для всех препятствий, которые нужно уничтожать на своем пути
interface IObstacle : public CAIObjectBase
{
	// для того, чтобы не стрелять вем по 1 препятствию
	virtual void UpdateTakenDamagePower( const float fUpdate ) = 0;
	virtual const float GetTakenDamagePower() const = 0;
	
	// выбирает пушку, из которой юнит лучше разрушит это препятствие
	virtual class CBasicGun *ChooseGunToShootToSelf( class CCommonUnit *pUnit, NTimer::STime *pTime ) = 0;

	virtual int GetPlayer() const = 0;
	virtual float GetHPPercent() const = 0;
	virtual const CVec3 GetCenter() const = 0;
	virtual bool IsAlive() const = 0;
	
	// помещает в начало очереди юнита команду на атаку этого обьекта
	virtual void IssueUnitAttackCommand( class CCommonUnit *pUnit ) = 0;
	
	// может ли юнит разрушить объект просто проехав по нему.
	virtual bool CanDeleteByMovingOver( class CAIUnit * pUnit ) = 0;
	virtual class CUpdatableObj *GetObject() const = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
interface IObstacleEnumerator
{
	//true if enough objects enumerated.
	virtual bool AddObstacle( IObstacle *pObstacle ) = 0;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __OBSTACLE_H__

