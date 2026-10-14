#pragma once

#include "Common_RTS_AI/Path.h"

#include "AILogic_export.h"

#include <cstdint>

struct IPath;

class AILOGIC_EXPORT CPresizePath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CPresizePath );

	enum EPresizePathState
	{
		EPPS_WAIT_FOR_INIT,
		EPPS_APPROACH_BY_STANDART,
		EPPS_TURN_TO_DESIRED_POINT,
		EPPS_APPROACH_DESIRED_POINT,
		EPPS_TURN_TO_DESIRED_DIR,
		EPPS_FINISHED,
	};

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	CBasePathUnit *pUnit = {};
	ZDATA
	ZSKIP
	ZONSERIALIZE
	EPresizePathState eState = {};
	CVec2 vEndPoint;
	CVec2 vEndDir;
	uint16_t wDesiredDir = {};

	CPtr<ISmoothPath> pPathStandart;
	CPtr<ISmoothPath> pPathCheat;

	float fSpeedLen = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		OnSerialize( f );
		f.Add( 3, &eState );
		f.Add( 4, &vEndPoint );
		f.Add( 5, &vEndDir );
		f.Add( 6, &wDesiredDir );
		f.Add( 7, &pPathStandart );
		f.Add( 8, &pPathCheat );
		f.Add( 9, &fSpeedLen );
		return 0;
	}

	void OnSerialize( IBinSaver &f );

public:
	CPresizePath()
		: pUnit( 0 )
		, fSpeedLen( 0.0f )
		, wDesiredDir( 0 )
		, vEndDir( VNULL2 )
		, vEndPoint( VNULL2 )
		, eState( EPPS_WAIT_FOR_INIT )
	{
	}

	CPresizePath( CBasePathUnit *pUnit,
	              const class CVec2 &vEndPoint,
	              const class CVec2 &vEndDir,
	              ISmoothPath *pApproachPath );

	virtual const CVec2 &GetFinishPoint() const
	{
		return vEndPoint;
	}

	bool IsFinished() const override;
	void Segment( const NTimer::STime timeDiff ) override;

	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override;
	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override;
	bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override;
	void Stop() override;
	const bool CanGoBackward() const override;
	const bool CanGoForward() const override;
	const CVec2 PeekPathPoint( const int nToShift ) const override;
	IMemento *CreateMemento() const override;

	virtual float GetCurvatureRadius() const
	{
		return 0.0f;
	}

	virtual bool IsWithFormation() const
	{
		return false;
	}
};

class CMilitaryCar;

class CMechUnitRestOnBoardPath : public ISmoothPath
{
	OBJECT_BASIC_METHODS( CMechUnitRestOnBoardPath );
	CBasePathUnit *pUnit = {};

	ZDATA
	ZSKIP
	ZONSERIALIZE
	CPtr<CMilitaryCar> pTransport;
	float fSpeedLen = {};
	CVec3 vFormerPlacement;
	CVec3 vCurrentPlacement;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		OnSerialize( f );
		f.Add( 3, &pTransport );
		f.Add( 4, &fSpeedLen );
		f.Add( 5, &vFormerPlacement );
		f.Add( 6, &vCurrentPlacement );
		return 0;
	}

	void OnSerialize( IBinSaver &saver );

public:
	CMechUnitRestOnBoardPath()
		: pUnit( 0 )
	{
	}

	CMechUnitRestOnBoardPath( CBasePathUnit *pUnit, CMilitaryCar *pTransport );

	virtual const CVec3 GetPoint( NTimer::STime timeDiff );
	void Segment( const NTimer::STime timeDiff ) override;

	virtual void GetSpeed3( CVec3 *vSpeed ) const;
	void Advance();

	virtual const CVec2 &GetFinishPoint() const
	{
		static CVec2 vFinish( VNULL2 );
		return vFinish;
	}

	bool IsFinished() const override
	{
		return false;
	}

	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		return false;
	}

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		return false;
	}

	bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override
	{
		return false;
	}

	void Stop() override {}

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
