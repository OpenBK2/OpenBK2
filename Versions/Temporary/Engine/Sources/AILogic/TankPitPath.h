#pragma once

#include "..\Common_RTS_AI\Path.h"

class CBasePathUnit;
// специфический путь для юнита, когда он выезжает из TankPit.
// идя по этому пути танк не поворачивает. если он на что-либо натыкается, то он должен 
// остановиться.
class CTankPitPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTankPitPath );
	public: int operator&( IBinSaver &saver ); private:;

	CVec2 vCurPoint;
	CVec2 vEndPoint;
	float fSpeedLen;

	CBasePathUnit *pUnit;
public:
	CTankPitPath() : pUnit( 0 ) { }
	CTankPitPath( CBasePathUnit *pUnit, const class CVec2 &vStartPoint, const class CVec2 &vEndPoint );
	virtual bool IsFinished() const;
	virtual void Segment( const NTimer::STime timeDiff );
	
	virtual const CVec2& GetFinishPoint() const { return vEndPoint; }
//ненужные функции
	virtual bool Init( CBasePathUnit *_pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap )
	{
		pUnit = _pUnit;
		CPtr<IPath> p = pPath;
		return true;
	}
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit  ) { return true; }
	virtual bool Init( interface IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap )
	{
		pUnit = _pUnit;
		CPtr<IMemento> p = pMemento;
		return true;
	}
	virtual void Stop() {}
	virtual float& GetSpeedLen() { return fSpeedLen; }
	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}
	//virtual void SlowDown() {}
	virtual const bool CanGoBackward() const { return false; }
	virtual const bool CanGoForward() const { return true; }
	virtual void GetNextTiles( list<SVector> *pTiles ) {}
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return VNULL2; };
	virtual IMemento* CreateMemento() const { return 0; }
	virtual float GetCurvatureRadius() const { return 0.0f; }

	virtual bool IsWithFormation() const { return false; }
};


