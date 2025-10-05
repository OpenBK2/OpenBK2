#pragma once

#include "Common_RTS_AI/Path.h"

// специфический путь для артиллеристов - напролом сквозь все.
// этот путь включается только когда артиллеристы уже у пушки.
class CArtilleryCrewPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CArtilleryCrewPath );

	CBasePathUnit *pUnit;

	ZDATA
		ZSKIP
		ZONSERIALIZE

	CVec2 vCurPoint;
	CVec2 vEndPoint;
	float fSpeedLen;
	bool bSelfSpeed;
	bool bNotInitialized;
	CVec3 vSpeed3;
	public: ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&vCurPoint); f.Add(4,&vEndPoint); f.Add(5,&fSpeedLen); f.Add(6,&bSelfSpeed); f.Add(7,&bNotInitialized); f.Add(8,&vSpeed3); return 0; }
		void OnSerialize( IBinSaver &f );
public:
	CArtilleryCrewPath()
		: pUnit( 0 ), vCurPoint( VNULL2 ), vEndPoint( VNULL2 ), fSpeedLen( 0.0f ), bSelfSpeed( false ), bNotInitialized( true ), vSpeed3( VNULL3 ) { }
	CArtilleryCrewPath( CBasePathUnit *pUnit, const CVec2 &vStartPoint, const CVec2 &vEndPoint, const float fMaxSpeed = 0.0f );

	void SetParams( const CVec2 &vEndPoint, const float fMaxSpeed );
	void SetParams( const CVec2 &_vEndPoint, const float fMaxSpeed, const CVec2 &_vSpeed2 );
	
	virtual bool IsFinished() const;
	virtual void Segment( const NTimer::STime timeDiff );

	virtual const CVec2& GetFinishPoint() const { return vEndPoint; }
//ненужные функции
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap );
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit  ) { return true; }
	virtual bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap );
	virtual void Stop() {}
	virtual const float GetSpeed() const 
	{ 
		return fSpeedLen; 
	}
	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) { }
//	virtual void SlowDown() {}
	virtual const bool CanGoBackward() const { return false; }
	virtual const bool CanGoForward() const { return true; }
	virtual void GetNextTiles( std::list<SVector> *pTiles ) {}
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return VNULL2; };
	virtual IMemento* CreateMemento() const { return 0; }
	virtual float GetCurvatureRadius() const { return 0.0f; }
	virtual bool IsWithFormation() const { return true; }
};

// путь для артиллерии, которую буксируют
class CArtilleryBeingTowedPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CArtilleryBeingTowedPath );
	ZDATA
	float fSpeedLen;
	CVec3 vCurPoint;
	CVec2 vCurPoint2D;
	CVec2 vSpeed;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&fSpeedLen); f.Add(3,&vCurPoint); f.Add(4,&vCurPoint2D); f.Add(5,&vSpeed); return 0; }
public:
	CArtilleryBeingTowedPath() : fSpeedLen( 0.0f ), vCurPoint( VNULL3	), vCurPoint2D( VNULL2 ), vSpeed( VNULL2 ) { }
	CArtilleryBeingTowedPath( const float fSpeedLen, const CVec2 &vCurPoint, const CVec2 &vSpeed );
	bool Init( float fSpeedLen, const class CVec2 &vCurPoint, const CVec2 &vSpeed );

	virtual const CVec2& GetFinishPoint() const { return vCurPoint2D; }
	virtual bool IsFinished() const { return false; }
	virtual void Segment( const NTimer::STime timeDiff ) {}

//ненужные функции
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap )
	{
		CPtr<IPath> p = pPath;
		return true;
	}
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit  ) { return true; }
	virtual bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap )
	{
		CPtr<IMemento> p = pMemento;
		return true;
	}
	virtual void Stop() { }
	virtual float& GetSpeedLen() { return fSpeedLen; }
	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) { }
	virtual const bool CanGoBackward() const { return true; }
	virtual const bool CanGoForward() const { return true; }
	virtual void GetNextTiles( std::list<SVector> *pTiles ) { }
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return VNULL2; };
	virtual IMemento* CreateMemento() const { return 0; }
	virtual float GetCurvatureRadius() const { return 0.0f; }

	virtual bool IsWithFormation() const { return false; }
	
	virtual void GetSpeed3( CVec3 *pvSpeed ) const { *pvSpeed = CVec3( vSpeed, 0.0f ); }
};


