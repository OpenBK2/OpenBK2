#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// для определения комбатной стуации
class CCombatEstimator
{
	struct SShellInfo
	{
		NTimer::STime time;
		float fDamage;
		//
		SShellInfo() {}
		SShellInfo( NTimer::STime time, float fDamage )
			:time( time ), fDamage( fDamage ) { }
	};

	typedef det_set<int> CRegisteredUnits;
	typedef std::list<SShellInfo> CShellTimes;
ZDATA
	float fDamage;
	CRegisteredUnits registeredMechUnits;			// вражескте юниты (не пехота)с ненулевой текущей скоростью
	CRegisteredUnits registeredInfantry;			// вражескте юниты (пехота)с ненулевой текущей скоростью

	CShellTimes shellTimes;								// время выстрела
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&fDamage); f.Add(3,&registeredMechUnits); f.Add(4,&registeredInfantry); f.Add(5,&shellTimes); return 0; }
public:
	CCombatEstimator();

	void Clear();
	void Segment();

	bool IsCombatSituation() const ;

	void AddShell( NTimer::STime time, float fDamage );
	
	void AddUnit( CAIUnit *pUnit );
	void DelUnit( CAIUnit *pUnit );
	                                                                                                                          
};


