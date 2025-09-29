#ifndef __AI_UNIT_INFO_FOR_GENERAL_H__
#define __AI_UNIT_INFO_FOR_GENERAL_H__
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma ONCE
class CAIUnit;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnitInfoForGeneral : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CAIUnitInfoForGeneral );

	ZDATA
	CPtr<CAIUnit> pOwner;
	NTimer::STime lastVisibleTime;
	CVec2 vLastVisiblePosition;

	NTimer::STime lastVisibleAntiArtTime;
	CVec2 vLastVisibleAntiArtCenter;
	float fDistToLastVisibleAntiArt;

	// когда в следующий раз говорить об изменении в состоянии генералу
	NTimer::STime nextTimeToReportGeneral;

	float fWeight;
	CVec2 vLastRegisteredGeneralPos;
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&lastVisibleTime); f.Add(4,&vLastVisiblePosition); f.Add(5,&lastVisibleAntiArtTime); f.Add(6,&vLastVisibleAntiArtCenter); f.Add(7,&fDistToLastVisibleAntiArt); f.Add(8,&nextTimeToReportGeneral); f.Add(9,&fWeight); f.Add(10,&vLastRegisteredGeneralPos); return 0; }

public:
	CAIUnitInfoForGeneral() : pOwner( 0 ), fWeight( 0 ), vLastRegisteredGeneralPos( VNULL2 ) { }
	CAIUnitInfoForGeneral( class CAIUnit *pOwner );

	void Segment();

	void UpdateVisibility( bool bVisible );
	void UpdateAntiArtFire( const NTimer::STime lastHeardTime, const CVec2 &vAntiArtCenter );

	void Die();

	class CAIUnit* GetOwner() const { return pOwner; }

	void SetWeight( const float _fWeight ) { fWeight = int(_fWeight * 100) / 100.0f; }
	const float GetWeight() const { return fWeight; }
	void SetRegisteredPos( const CVec2 &vPos ) { vLastRegisteredGeneralPos = vPos; }
	const CVec2& GetRegisteredPos() const { return vLastRegisteredGeneralPos; }

	const NTimer::STime GetLastTimeOfVisibility() const { return Max( lastVisibleAntiArtTime, lastVisibleTime ); }
	void SetLastVisibleTime( const NTimer::STime time ) { lastVisibleTime = time; lastVisibleAntiArtTime = time; }
	
	bool IsLastVisibleAntiArt() const { return lastVisibleAntiArtTime > lastVisibleTime; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __AI_UNIT_INFO_FOR_GENERAL_H__
