#ifndef __RND_RUN_UP_TO_ENEMY__
#define __RND_RUN_UP_TO_ENEMY__
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldier;
class CAIUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRndRunUpToEnemy
{

	CSoldier *pOwner;
	ZDATA
		ZSKIP
		ZONSERIALIZE
	CPtr<CAIUnit> pEnemy;

	CVec2 vLastOwnerPos;
	NTimer::STime checkTime;
	bool bRunningToEnemy;
	bool bForceStaying;
	bool bCheck;
	public: ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&pEnemy); f.Add(4,&vLastOwnerPos); f.Add(5,&checkTime); f.Add(6,&bRunningToEnemy); f.Add(7,&bForceStaying); f.Add(8,&bCheck); return 0; }
	void OnSerialize( IBinSaver &saver );
	//
	void SendOwnerToRandomRun();
public:
	CRndRunUpToEnemy() : pOwner( 0 ) { }
	CRndRunUpToEnemy( CAIUnit *pOwner, CAIUnit *pEnemy, bool bCanMove );
	void Init( CAIUnit *pOwner, CAIUnit *pEnemy, bool bCanMove );

	bool IsRunningToEnemy() const { return bRunningToEnemy; }
	void Segment();

	void Finish();
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif __RND_RUN_UP_TO_ENEMY__

