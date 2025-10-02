
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// для атаки второстепенными пушками.
// если турели, на которых расположены пушки не залоканы, то вращает турели.
class CFreeFireManager
{
	enum { TIME_TO_CHECK = 1500 };

	struct SShotInfo
	{
	public:
		ZDATA 
		CPtr<CAIUnit> pTarget;
		CVec2 shootingPos;
		SAIAngle unitDir;
		SAIAngle gunDir;

		public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTarget); f.Add(3,&shootingPos); f.Add(4,&unitDir); f.Add(5,&gunDir); return 0; }
		SShotInfo() : shootingPos( VNULL2 ), unitDir( 0 ), gunDir( 0 ) { }
		
		bool NeedAim( CAIUnit *pNewTarget, class CBasicGun *pGun ) const;
		void SetInfo( CAIUnit *pNewTarget, class CBasicGun *pGun );
	};

	ZDATA 
	vector<SShotInfo> shootInfo;

	NTimer::STime lastCheck;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&shootInfo); f.Add(3,&lastCheck); return 0; }
public:
	CFreeFireManager() : lastCheck( 0 ) { }
	CFreeFireManager( class CCommonUnit *pOwner );
	// считается, что base уже залокана pActiveGun-ом
	void Analyze( class CCommonUnit *pUnit, class CBasicGun *pActiveGun );
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

