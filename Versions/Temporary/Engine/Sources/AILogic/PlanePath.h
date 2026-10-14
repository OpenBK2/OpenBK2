#pragma once

#include "Common_RTS_AI/Path.h"

struct IAviationUnit;

class CBasePlaneSmoothPath : public ISmoothPath
{
public:
	virtual void SetAviationUnit( IAviationUnit *_pPlane ) {}
};
class CPlanesFormation;

// path for planes, that formation with horisontal shift.
class CPlaneInFormationSmoothPath : public CBasePlaneSmoothPath
{
	OBJECT_BASIC_METHODS( CPlaneInFormationSmoothPath );

public:
	int operator&( IBinSaver &saver ) override;

private:
	;

	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	CPlanesFormation *pFormation = {};
	class CAviation *pOwner = {};

public:
	CPlaneInFormationSmoothPath()
		: pOwner( 0 )
	{
	}

	void Init( class CAviation *_pOwner );

	bool IsFinished() const override;
	void Segment( const NTimer::STime timeDiff ) override;

	// need to correct work of CBasePathUnit
	bool const CanGoBackward() const override;

	// empty functions
	bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) override
	{
		NI_ASSERT( false, "wrong call" );
		return false;
	}

	bool Init( struct IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) override
	{
		NI_ASSERT( false, "wrong call" );
		return false;
	}

	bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) override
	{
		NI_ASSERT( false, "wrong call" );
		return false;
	}

	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist )
	{
		NI_ASSERT( false, "wrong call" );
	}

	bool const CanGoForward() const override
	{
		NI_ASSERT( false, "wrong call" );
		return true;
	}

	virtual void GetNextTiles( std::list<SVector> *pTiles )
	{
		NI_ASSERT( false, "wrong call" );
	}

	const CVec2 PeekPathPoint( const int nToShift ) const override
	{
		NI_ASSERT( false, "wrong call" );
		return VNULL2;
	}

	IMemento *CreateMemento() const override
	{
		NI_ASSERT( false, "wrong call" );
		return 0;
	}

	virtual bool IsWithFormation() const
	{
		NI_ASSERT( false, "wrong call" );
		return true;
	}

	void Stop() override
	{
		NI_ASSERT( false, "wrong call" );
	}
};
