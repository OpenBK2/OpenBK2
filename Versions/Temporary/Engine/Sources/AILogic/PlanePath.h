#pragma once

#include "..\Common_RTS_AI\Path.h"

interface IAviationUnit;

class CBasePlaneSmoothPath : public ISmoothPath
{
public:
	virtual void SetAviationUnit( IAviationUnit *_pPlane ) {  }
};
class CPlanesFormation;

// path for planes, that formation with horisontal shift.
class CPlaneInFormationSmoothPath : public CBasePlaneSmoothPath
{
	OBJECT_BASIC_METHODS( CPlaneInFormationSmoothPath );
	public: int operator&( IBinSaver &saver ); private:;

	CPlanesFormation * pFormation;
	class CAviation *pOwner;
	
public:
	CPlaneInFormationSmoothPath() : pOwner( 0 ) {  } 
	void Init( class CAviation *_pOwner ) ;

	virtual bool IsFinished() const;
	virtual void Segment( const NTimer::STime timeDiff );

	// need to correct work of CBasePathUnit
	virtual bool const CanGoBackward() const;

	// empty functions
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap ) { NI_ASSERT(false, "wrong call" ); return false; }
	virtual bool Init( interface IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap ) { NI_ASSERT(false, "wrong call" ); return false; }
	virtual bool InitByFormationPath( class CFormation *pFormation, CBasePathUnit *pUnit ) { NI_ASSERT(false, "wrong call" ); return false; }
	virtual void NotifyAboutClosestThreat( CBasePathUnit *pCollUnit, const float fDist ) { NI_ASSERT(false, "wrong call" ); }
	virtual bool const CanGoForward() const { NI_ASSERT(false, "wrong call" ); return true; }
	virtual void GetNextTiles( list<SVector> *pTiles ) { NI_ASSERT(false, "wrong call" ); }
	virtual const CVec2 PeekPathPoint( const int nToShift ) const { NI_ASSERT(false, "wrong call" ); return VNULL2; }
	virtual IMemento* CreateMemento() const { NI_ASSERT(false, "wrong call" ); return 0; }
	virtual bool IsWithFormation() const { NI_ASSERT(false, "wrong call" ); return true; }
	virtual void Stop() { NI_ASSERT(false, "wrong call" ); }
};


