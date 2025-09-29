#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__

#pragma ONCE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCommonUnit;
class CAIUnit;
class CBasicGun;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// выбор наилучшего воздушного юнита ( для легких зениток ) - для стрельбы сопро-
//водительным огнем
class CShootEstimatorLighAA
{
	ZDATA
	BYTE party;
	
	CPtr<CCommonUnit> pUnit;
	CPtr<CAIUnit> pResult;
	CPtr<CBasicGun> pGun;
	float fWorstDamage;
	NTimer::STime bestTime;
	float fMinDistance;
	bool bCanShootNow;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&party); f.Add(3,&pUnit); f.Add(4,&pResult); f.Add(5,&pGun); f.Add(6,&fWorstDamage); f.Add(7,&bestTime); f.Add(8,&fMinDistance); f.Add(9,&bCanShootNow); return 0; }
public:
	CShootEstimatorLighAA() 
	: fWorstDamage( -1 ),
		bestTime( 100000 ),
		fMinDistance( 1e10 ),
		bCanShootNow( false ) { }

	void Init( class CCommonUnit *pUnit );
	// выбрать оптимальный юнит для данного gun, приоценки времени, чтобы пристрелить, 
	// учитывается только время поворота gun до этого юнита
	void Init( class CCommonUnit *pUnit, CBasicGun *pGun );
	void AddUnit( class CAIUnit *pTarget );
	class CAIUnit* GetBestUnit();
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStandartBehaviour
{
	//
	class CAIUnit* LookForTargetInFireRange( class CCommonUnit *pUnit );

	ZDATA
	NTimer::STime camouflateTime;

	NTimer::STime underFireAnalyzeTime;
	NTimer::STime lastTimeOfRotate;
	NTimer::STime fleeTraceEnemyTime;
	int nLastSign;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&camouflateTime); f.Add(3,&underFireAnalyzeTime); f.Add(4,&lastTimeOfRotate); f.Add(5,&fleeTraceEnemyTime); f.Add(6,&nLastSign); return 0; }
	// отслеживать врага, если нет никого, в кого возможно стрелять и нельзя двигаться, и есть turret
	bool TryToTraceEnemy( class CAIUnit *pUnit );
public:
	CStandartBehaviour() : camouflateTime( 0 ), underFireAnalyzeTime( 0 ), nLastSign( 1 ), lastTimeOfRotate( NTimer::STime(-1) ), fleeTraceEnemyTime( 0 ) { }

	void ResetTime( class CCommonUnit *pUnit );
	void UponFire( class CCommonUnit *pUnit, class CAIUnit *pWho, class CAICommand *pCommand );

	void AnalyzeUnderFire( class CAIUnit *pUnit );
	
//	void StartCamouflating();
//	void AnalyzeCamouflage( class CAIUnit *pUnit );
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __BEHAVIOUR_H__
