#pragma once

#include "Manuver.h"
#include "PlanePathFraction.h"

// move to separate file
struct SPlanesConsts
{
	static float MIN_HEIGHT;
	static float MAX_HEIGHT;
	static float MIN_HEIGHT_TEMP;
	static float GetMinHeight()
	{
		if ( MIN_HEIGHT_TEMP == 0.0f )
			return MIN_HEIGHT;
		return MIN_HEIGHT_TEMP;
	}
};

class CPlanesFormation;
interface IPathFraction;

/**
 * в маневре в зависимости от участка траектории задается крен самолета (либо от вертикали, либо от направления
 * к центру кривизны.
 */

class CManuver : public IManuver 
{

	ZDATA
	CPtr<CPlanesFormation> pPlane;
	CObj<IPathFraction> pPath;
	CPathList simplePaths;

	float fProgress;															// path progress
	float fSpeed;
	
	CVec3 vCenter;
	CVec3 vSpeed;
	CVec3 vNormal;			

	bool bUsed;

	// nomrale acceleration
	float fCurTiltSpeed;
	float fDistToGo;							// distance that other path must go to ensure smoothness.
	bool bToHorisontal;

	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPlane); f.Add(3,&pPath); f.Add(4,&simplePaths); f.Add(5,&fProgress); f.Add(6,&fSpeed); f.Add(7,&vCenter); f.Add(8,&vSpeed); f.Add(9,&vNormal); f.Add(10,&bUsed); f.Add(11,&fCurTiltSpeed); f.Add(12,&fDistToGo); f.Add(13,&bToHorisontal); return 0; }
	void InitInternal( interface IPathFraction *_pPath, CPlanesFormation *_pPlane, const float fSpeed, const CVec3 &vNormale, const bool _bToHorisontal );
	void CheckToHorisontal();
protected:

	void InitCommon( interface IPathFraction *_pPath, CPlanesFormation *_pPlane, const bool _bToHorisontal );
	bool AdvanceCommon( const NTimer::STime timeDiff );

private:
	void CalcSpeed();
	void CalcPoint();
	void CalcNormale( const NTimer::STime timeDiff );
	
	void AdjustNormale( const NTimer::STime timeDiff, const CVec3 &vDesiredNormale, CVec3 *vNormal, int *pnRotation, float *pfTiltToGo ) const;

	// height difference needed to start horisontal move
	const float CalcHeightToEnterHorisontal( const CVec3 &_vSpeed, const class CPlanePreferences &pref ) const;
	bool IsChangeHeightPossible( const CVec3 &_vSpeed, const CVec3 &vPos, const class CPlanePreferences &pref, CVec3 *pvNewTarget ) const;

public:

	bool IsToHorisontal() const { return bToHorisontal; }
	void SetPlane( CPlanesFormation *_pPlane ) { pPlane = _pPlane; }

	// to init next manuver with this params - will be smooth transition between manuvers
	void GetManuverParams( struct SPrevPathParams *pParams ) const;
	
	CManuver() : bUsed( false ), fCurTiltSpeed( 0 ), bToHorisontal( 0 ) {  }
	//CRAP{ FOR TEST
	// preferred top direction
	CVec3 GetTopDirection() const
	{
		return CVec3(0,0,1.0f);
	}
	virtual const CVec3 GetEndPoint() const { return pPath->GetEndPoint(); }
	//CRAP}

	CVec3 GetPos() const { return vCenter; }
	CVec3 GetSpeed() const { return vSpeed; }
	CVec3 GetNormale() const;

	CVec3 GetProspectivePoint( const NTimer::STime nTime ) const;
	CVec3 GetProspectiveSpeed( NTimer::STime nTime ) const;
	
	// helper functions
	static const CVec3 CalcPredictedPoint( class CPlanesFormation *pPos, class CPlanesFormation *pEnemy );
	static const CVec3 CalcPredictedSpeed( class CPlanesFormation *pPos, class CPlanesFormation *pEnemy );
};

//	CManuverGeneric

class CManuverGeneric : public CManuver 
{
	OBJECT_BASIC_METHODS( CManuverGeneric );
	ZDATA_(CManuver)
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CManuver*)this); return 0; }
public:

	void Init( CPlanesFormation *pPos, const CVec3 &vPos );
	virtual bool Advance( const NTimer::STime timeDiff );
};

class CManuverToHorisontal : public CManuver
{
	OBJECT_BASIC_METHODS( CManuverToHorisontal )
	ZDATA_(CManuver)
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CManuver*)this); return 0; }

public:

	void Init( CPlanesFormation *pPos, const CVec3 &vPos );
	virtual bool Advance( const NTimer::STime timeDiff );
};

class CManuverPrepareGroundAttack : public CManuver
{
	OBJECT_BASIC_METHODS( CManuverPrepareGroundAttack )
	ZDATA_(CManuver)
public: 
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CManuver*)this); return 0; }
public:
	void Init( CPlanesFormation *pPos, const CVec3 &vPos, const CVec3 &vDirection );
	virtual bool Advance( const NTimer::STime timeDiff );
};

//	CManuverSteepClimb

// 30-45 degrees gorka
//
class CManuverSteepClimb : public CManuver 
{
	OBJECT_BASIC_METHODS( CManuverSteepClimb );
	ZDATA_(CManuver)
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CManuver*)this); return 0; }

public:
	CManuverSteepClimb() {  }
	
	void Init( CPlanesFormation *pPos );
	virtual bool Advance( const NTimer::STime timeDiff );
	virtual void Init( const NDb::EManuverDestination dest, CPlanesFormation *pPlane, CPlanesFormation *pEnemy = 0 );
};
