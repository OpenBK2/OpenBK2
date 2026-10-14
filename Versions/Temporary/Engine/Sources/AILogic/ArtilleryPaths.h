#pragma once

#include "Common_RTS_AI/Path.h"

// специфический путь для артиллеристов - напролом сквозь все.
// этот путь включается только когда артиллеристы уже у пушки.
class CArtilleryCrewPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CArtilleryCrewPath );

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	CBasePathUnit *pUnit = {};

	ZDATA
	ZSKIP
	ZONSERIALIZE

	CVec2 vCurPoint;
	CVec2 vEndPoint;
	float fSpeedLen = {};
	bool bSelfSpeed = {};
	bool bNotInitialized = {};
	CVec3 vSpeed3;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		OnSerialize( f );
		f.Add( 3, &vCurPoint );
		f.Add( 4, &vEndPoint );
		f.Add( 5, &fSpeedLen );
		f.Add( 6, &bSelfSpeed );
		f.Add( 7, &bNotInitialized );
		f.Add( 8, &vSpeed3 );
		return 0;
	}

	void OnSerialize( IBinSaver &f );

public:
	CArtilleryCrewPath()
		: pUnit( 0 )
		, vCurPoint( VNULL2 )
		, vEndPoint( VNULL2 )
		, fSpeedLen( 0.0f )
		, bSelfSpeed( false )
		, bNotInitialized( true )
		, vSpeed3( VNULL3 )
	{
	}

	CArtilleryCrewPath( CBasePathUnit *pUnit,
	                    const CVec2 &vStartPoint,
	                    const CVec2 &vEndPoint,
	                    const float fMaxSpeed = 0.0f );

	void SetParams( const CVec2 &vEndPoint, const float fMaxSpeed );
	void SetParams( const CVec2 &_vEndPoint, const float fMaxSpeed, const CVec2 &_vSpeed2 );

	bool IsFinished() const override;
	void Segment( const NTimer::STime timeDiff ) override;

	virtual const CVec2 &GetFinishPoint() const
	{
		return vEndPoint;
	}

	//ненужные функции
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override;

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return true;
	}

	bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override;

	void Stop() override {}

	const float GetSpeed() const override
	{
		return fSpeedLen;
	}

	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}

	//	virtual void SlowDown() {}
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
		return true;
	}
};

// путь для артиллерии, которую буксируют
class CArtilleryBeingTowedPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CArtilleryBeingTowedPath );
	ZDATA
	float fSpeedLen = {};
	CVec3 vCurPoint;
	CVec2 vCurPoint2D;
	CVec2 vSpeed;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &fSpeedLen );
		f.Add( 3, &vCurPoint );
		f.Add( 4, &vCurPoint2D );
		f.Add( 5, &vSpeed );
		return 0;
	}

public:
	CArtilleryBeingTowedPath()
		: fSpeedLen( 0.0f )
		, vCurPoint( VNULL3 )
		, vCurPoint2D( VNULL2 )
		, vSpeed( VNULL2 )
	{
	}

	CArtilleryBeingTowedPath( const float fSpeedLen, const CVec2 &vCurPoint, const CVec2 &vSpeed );
	bool Init( float fSpeedLen, const class CVec2 &vCurPoint, const CVec2 &vSpeed );

	virtual const CVec2 &GetFinishPoint() const
	{
		return vCurPoint2D;
	}

	bool IsFinished() const override
	{
		return false;
	}

	void Segment( const NTimer::STime timeDiff ) override {}

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

	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) {}

	const bool CanGoBackward() const override
	{
		return true;
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

	virtual void GetSpeed3( CVec3 *pvSpeed ) const
	{
		*pvSpeed = CVec3( vSpeed, 0.0f );
	}
};
