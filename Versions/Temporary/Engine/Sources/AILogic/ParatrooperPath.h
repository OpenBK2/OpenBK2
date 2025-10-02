#pragma once

#include "..\Common_RTS_AI\Path.h"
class CAIUnit;

class CParatrooperPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CParatrooperPath );
	ZDATA
	NTimer::STime lastPathUpdateTime;

	CVec3 vStartPoint;
	CVec3 vFinishPoint;
	CVec2 vFinishPoint2D;

	CVec3 vCurPoint;
	float fSpeedLen;
	
	CVec3 vHorSpeed;//horisontal speed of parachute
	CPtr<CAIUnit> pUnit;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&lastPathUpdateTime); f.Add(3,&vStartPoint); f.Add(4,&vFinishPoint); f.Add(5,&vFinishPoint2D); f.Add(6,&vCurPoint); f.Add(7,&fSpeedLen); f.Add(8,&vHorSpeed); f.Add(9,&pUnit); return 0; }
	void FindFreeTile();
	void Init();

public:
	CParatrooperPath() { };
	CParatrooperPath( const CVec3 &startPoint, CAIUnit *pUnit );
	virtual bool IsFinished() const;
	virtual void Segment( const NTimer::STime timeDiff );

	virtual void GetSpeed3( CVec3 *vSpeed ) const;

	virtual const CVec2& GetFinishPoint() const { return vFinishPoint2D; }
//ненужные функции
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) { CPtr<IPath> p = pPath; return true; }
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { return true; }
	virtual bool Init( interface IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) { CPtr<IMemento> p = pMemento; return true; }

	virtual void Stop() {}
	virtual float& GetSpeedLen() { return fSpeedLen; }
	virtual const float GetSpeed() const { return fSpeedLen; }
	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}
	virtual const bool CanGoBackward() const { return false; }
	virtual const bool CanGoForward() const { return true; }
	virtual void GetNextTiles( list<SVector> *pTiles ) {}
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { return CVec2( 0, 0 ); }
	virtual IMemento* CreateMemento() const { return 0; }
	virtual float GetCurvatureRadius() const { return 0.0f; }

	virtual bool IsWithFormation() const { return true; }

	static float CalcFallTime( const float fZ );
};

