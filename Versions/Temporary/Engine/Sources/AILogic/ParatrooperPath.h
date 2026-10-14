#pragma once

#include "Common_RTS_AI/Path.h"
class CAIUnit;

class CParatrooperPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CParatrooperPath );
	ZDATA
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NTimer::STime lastPathUpdateTime = {};

	CVec3 vStartPoint;
	CVec3 vFinishPoint;
	CVec2 vFinishPoint2D;

	CVec3 vCurPoint;
	float fSpeedLen = {};

	CVec3 vHorSpeed; //horisontal speed of parachute
	CPtr<CAIUnit> pUnit;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &lastPathUpdateTime );
		f.Add( 3, &vStartPoint );
		f.Add( 4, &vFinishPoint );
		f.Add( 5, &vFinishPoint2D );
		f.Add( 6, &vCurPoint );
		f.Add( 7, &fSpeedLen );
		f.Add( 8, &vHorSpeed );
		f.Add( 9, &pUnit );
		return 0;
	}

	void FindFreeTile();
	void Init();

public:
	CParatrooperPath() {}

	CParatrooperPath( const CVec3 &startPoint, CAIUnit *pUnit );
	bool IsFinished() const override;
	void Segment( const NTimer::STime timeDiff ) override;

	virtual void GetSpeed3( CVec3 *vSpeed ) const;

	virtual const CVec2 &GetFinishPoint() const
	{
		return vFinishPoint2D;
	}

	//ненужные функции
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		CPtr<IPath> p = pPath;
		return true;
	}

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return true;
	}

	bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override
	{
		CPtr<IMemento> p = pMemento;
		return true;
	}

	void Stop() override {}

	virtual float &GetSpeedLen()
	{
		return fSpeedLen;
	}

	const float GetSpeed() const override
	{
		return fSpeedLen;
	}

	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}

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
		return CVec2( 0, 0 );
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
		return true;
	}

	static float CalcFallTime( const float fZ );
};
