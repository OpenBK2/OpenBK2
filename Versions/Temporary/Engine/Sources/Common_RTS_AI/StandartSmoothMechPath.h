#pragma once

#include "StandartSmoothPath.h"

#include <cstdint>

#define STANDART_SMOOTH_MECH_PATH 0x311133C2

//! сглаженный путь для механических юнитов
class CStandartSmoothMechPath : public CStandartSmoothPathBasis
{
	OBJECT_BASIC_METHODS( CStandartSmoothMechPath );

	ZDATA_(CStandartSmoothPathBasis)
		// temporary variables
		CVec2 vLastValidatedPoint;

		std::list<CCirclePath> circles;

		bool bSmoothTurn;
		bool bSkipNextSegment;

		bool bCanGoForward, bCanGoBackward;
		bool bForceGoBackward;
		NTimer::STime lastCheckToRightTurn;
		SVector vLastStopPosition;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStandartSmoothPathBasis*)this); f.Add(2,&vLastValidatedPoint); f.Add(3,&circles); f.Add(4,&bSmoothTurn); f.Add(5,&bSkipNextSegment); f.Add(6,&bCanGoForward); f.Add(7,&bCanGoBackward); f.Add(8,&lastCheckToRightTurn); f.Add(9,&vLastStopPosition); f.Add(10,&bForceGoBackward); return 0; }
	//
	void AddSmoothTurn();

	// построить большой разворот
	bool BuildLargeTurn( const uint16_t wStartDir, const uint16_t wEndDir, const CVec2 &vFinishPoint );
	// построить серию маленьких разворотов (второй вариант, может дать более оптимальный разворот)
	bool BuildSmallTurns( const uint16_t wStartDir, const uint16_t wEndDir, const CVec2 &vFinishPoint, const bool bPrefereForward );
	// прсото сглаженно выйти на новый угол
	bool BuildSmoothTurn( const CVec2 &vUnitMoveDir, const bool bCheckThreshold );
	// отехать задом, чтобы было место для маневра
	bool RideAway();

	bool CheckTurn( const uint16_t wNewDir );
	uint16_t CheckArc( const CVec2 &vUnit, const uint16_t wStartAngle, const uint16_t wDiffAngle, const float fRadius, const bool bClockWise, const bool bForward );
	bool UpdateDirection();
protected:
	virtual const CVec2 MoveUnit( const NTimer::STime timeDiff, const float fSpeed );
	virtual const bool ValidateCurrentPath( const CVec2 &vCenter, const CVec2 &vNewPoint );
public:
	CStandartSmoothMechPath() : vLastValidatedPoint( VNULL2 ), bSmoothTurn( false ), bSkipNextSegment( false ),
		bCanGoForward( false ), bCanGoBackward( false ), bForceGoBackward( false ), lastCheckToRightTurn( 0 ), vLastStopPosition( -1, -1 ) {}

	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap );

	virtual void Stop();

	virtual const bool CanGoBackward() const;
	virtual const bool CanGoForward() const { return bCanGoForward; }
	virtual void SetForceGoBackward( const bool bForce );
};


