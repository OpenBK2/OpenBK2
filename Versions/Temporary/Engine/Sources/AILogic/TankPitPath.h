#pragma once

#include "Common_RTS_AI/Path.h"

class CBasePathUnit;

// специфический путь для юнита, когда он выезжает из TankPit.
// идя по этому пути танк не поворачивает. если он на что-либо натыкается, то он должен
// остановиться.
class CTankPitPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CTankPitPath );

public:
	int operator&( IBinSaver &saver ) override;

private:
	;

	CVec2 vCurPoint;
	CVec2 vEndPoint;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fSpeedLen = {};

	CBasePathUnit *pUnit = {};

public:
	CTankPitPath()
		: pUnit( 0 )
	{
	}

	CTankPitPath( CBasePathUnit *pUnit, const class CVec2 &vStartPoint, const class CVec2 &vEndPoint );
	bool IsFinished() const override;
	void Segment( const NTimer::STime timeDiff ) override;

	virtual const CVec2 &GetFinishPoint() const
	{
		return vEndPoint;
	}

	//ненужные функции
	bool Init( CBasePathUnit *_pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		pUnit = _pUnit;
		CPtr<IPath> p = pPath;
		return true;
	}

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return true;
	}

	bool Init( struct IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap ) override
	{
		pUnit = _pUnit;
		CPtr<IMemento> p = pMemento;
		return true;
	}

	void Stop() override {}

	virtual float &GetSpeedLen()
	{
		return fSpeedLen;
	}

	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}

	//virtual void SlowDown() {}
	const bool CanGoBackward() const override
	{
		return false;
	}

	const bool CanGoForward() const override
	{
		return true;
	}

	virtual void GetNextTiles( std::list<SVector> *pTiles ) {}

	const CVec2 PeekPathPoint( const int nToShift ) const override
	{
		return VNULL2;
	}

	IMemento *CreateMemento() const override
	{
		return 0;
	}

	virtual float GetCurvatureRadius() const
	{
		return 0.0f;
	}

	virtual bool IsWithFormation() const
	{
		return false;
	}
};
