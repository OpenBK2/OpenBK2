#pragma once

class CCommonUnit;

class CEnemyRememberer : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CEnemyRememberer );

	ZDATA
	CVec2 vPosition;
	NTimer::STime timeLastSeen;
	int timeBeforeForget;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPosition); f.Add(3,&timeLastSeen); f.Add(4,&timeBeforeForget); return 0; }
public:
	CEnemyRememberer() {  }
	CEnemyRememberer( const int timeBeforeForget );
	void SetVisible( const class CCommonUnit *pUnit, const bool bVisible );
	const CVec2 GetPos( const class CCommonUnit *pUnit ) const;
	const NTimer::STime GetLastSeen() const { return timeLastSeen; }
	const bool IsTimeToForget() const;
};



