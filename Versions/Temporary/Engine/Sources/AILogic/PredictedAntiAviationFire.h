#pragma once

#include <cstdint>

class CAviation;
class CAIUnit;

class CPredictedAntiAviationFire
{
	class SPredict
	{
		uint16_t wHor, wVer;
		CVec3 vPt;
		float fRange;
		NTimer::STime timeToFire;
	public:
		SPredict() {  }
		SPredict( const CVec3 &pt, const float _fRange, const NTimer::STime _timeToFire, CAIUnit *pOwner );
		uint16_t GetHor()const { return wHor; }
		uint16_t GetVer()const { return wVer; }
		float GetRange() const { return fRange; }
		const CVec3 GetPt() const { return vPt; }
		const NTimer::STime GetFireTime() const { return timeToFire; }
	};
	enum ESoldierAttackAviationState
	{
		SAAS_ESITMATING,

		SAAS_START_TRASING,										// для стрельбы сопроводительным огнем
		SAAS_TRASING,
		SAAS_FIRING,

		SAAS_START_AIMING_TO_PREDICTED_POINT,	// для стрельбы заградительным огнем
		SAAS_AIM_TO_PREDICTED_POINT,					
		SAAS_START_FIRE_TO_PREDICTED_POINT,
		SAAS_FIRING_TO_PREDICTED_POINT,

		SAAS_FINISH,
		SAAS_WAIT_FOR_END_OF_BURST,
		SAAS_FINISHED_TASK,
	};
	class CAIUnit *pUnit;
	typedef std::list<int> Guns;

	ZDATA
		ZSKIP
		ZONSERIALIZE
	ESoldierAttackAviationState eState;

	CPtr<CAviation> pPlane;
	bool bAttacking;											// true when desided to aim and shoot

	SPredict aimPoint;		// точка прицеливания при стрельбе заградительным огнем
	NTimer::STime timeOfStartBurst;
	NTimer::STime timeLastAimUpdate;
	Guns nGuns;
public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&eState); f.Add(4,&pPlane); f.Add(5,&bAttacking); f.Add(6,&aimPoint); f.Add(7,&timeOfStartBurst); f.Add(8,&timeLastAimUpdate); f.Add(9,&nGuns); return 0; }
		void OnSerialize( IBinSaver &saver );
private:
	bool CanFireNow() const;
	void FireNow();
	void Aim();
	bool CalcAimPoint();
	bool IsFinishedFire();
	void StopFire();

public:
	
	CPredictedAntiAviationFire( CAIUnit *_pOwner, CAviation *_pPlane )
		: pUnit( _pOwner ), pPlane( _pPlane ), eState( SAAS_ESITMATING ) {  }

	void AddGunNumber( const int _nGun )
	{
		nGuns.push_back( _nGun );
	}
	void SetTarget( CAviation *pPlane );
	CAviation * GetPlane() const { return pPlane; }
	CAIUnit * GetUnit() const { return pUnit; }
	void Segment();
	bool IsFinishedTask() const { return eState == SAAS_FINISHED_TASK; }
	void Stop();
};


