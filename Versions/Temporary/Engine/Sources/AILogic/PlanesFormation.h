#pragma once

#include "Common_RTS_AI/BasePathUnit.h"
#include "IPlane.h"
#include "Manuver.h"
#include "PlanePreferences.h"

#include <cstdint>

class CPlaneManuverHistory
{
	typedef det_map<CVec3, SFormationMemberInfo, SVec3Hash> CMemberCache;
	ZDATA
	std::list<CObj<IManuver>> pathHistory;
	CMemberCache memberCache;
	SFormationMemberInfo curPos;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bFinished = {};

public:
	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &pathHistory );
		f.Add( 3, &memberCache );
		f.Add( 4, &curPos );
		f.Add( 5, &bFinished );
		return 0;
	}

public:
	bool IsEmpty() const
	{
		return pathHistory.empty();
	}

	bool IsManuverFinished() const;
	void StartManuver( IManuver *pManuver );
	IManuver *GetCurManuver() const;
	void ClearUnised();
	const SFormationMemberInfo &GetValues( const CVec3 &vOffset );
	void Advance( const NTimer::STime timeDiff );
};

// class to extend if unit to go by CPlaneSmoothPath

class CPlanesFormation : public CAIObjectBase, public IPlane, public CBasePathUnit
{
	OBJECT_NOCOPY_METHODS( CPlanesFormation );

	static int nIDSoFar;
	static det_map<int, bool> existence;
	ZDATA_( CBasePathUnit )
	CPlaneManuverHistory pathHistory;
	CPlanePreferences preferences;

	CVec3 vPos;
	CVec3 vSpeed;
	CVec3 vNormal;

	CVec3 vNewPos;
	CVec3 vNewSpeed;
	CVec3 vNewNormal;

	// for member counting
	int nProcessed = {};
	int nAlive = {};

	float fBombPointOffset = {};
	int nID = {};

public:
	ZEND int operator&( IBinSaver &f ) override;

protected:
	void NullSegmTime() override {}

	void CheckForDestroyedObjects( const CVec2 &vCenter ) const override {}

public:
	CPlanesFormation()
		: nID( -1 )
	{
	}

	CPlanesFormation( int )
	{
		nID = nIDSoFar++;
		existence[nID] = true;
	}

	~CPlanesFormation() override
	{
		existence.erase( nID );
	}

	static void Clear();
	static void SerializeIDs( const int nChunk, IBinSaver &saver );
	static bool IsFormaionExists( int nID );

	int GetFormationID() const
	{
		return nID;
	}

	//{ PlaneFormation interface
	bool IsManuverFinished() const;
	void CreateManuver( class CPlanesFormation *pEnemy );
	void CreateManuver( const CVec3 &vCenter, int nUniqueID );
	void CreatePointManuver( const CVec3 &vPos, const bool bToHorisontal );
	void Advance( const NTimer::STime timeDiff );

	//}

	//{ IPlane interface
	// current speed
	CVec3 GetSpeedB2() const override
	{
		return vSpeed;
	}

	// current position
	CVec3 GetPosB2() const override
	{
		return vPos;
	}

	// current vertical direction
	CVec3 GetNormalB2() const override
	{
		return vNormal;
	}

	const struct IManuver *GetManuver() const override
	{
		return pathHistory.GetCurManuver();
	}

	// next calculated position
	CVec3 GetPosNext() const override
	{
		return vNewPos;
	}

	CVec3 GetSpeedNext() const override
	{
		return vNewSpeed;
	}

	CVec3 GetNormalNext() const override
	{
		return vNewNormal;
	}

	//CRAP{
	void SetB2( const CVec3 &vPos, const CVec3 &vSpeed, const CVec3 &vNormal ) override {}

	//CRAP}

	// текущий самолет уже находится под атакой.
	bool IsBeingAttackedB2() override
	{
		return false;
	}

	// когда враг уже готовится стрелять в самолет. нужно для того, чтобы за 1 самолетом не гонялось несколько.
	void NotifyAttackedB2( IPlane &attacker, bool bAttack ) override {}

	// access to plane's preferences
	const class CPlanePreferences &GetPreferencesB2() const override
	{
		return preferences;
	}

	//}

	void SetNewPos( const CVec3 &vCenter );
	const CVec3 &GetPoint( const CVec3 &vFormationOffset );
	const CVec3 &GetSpeed( const CVec3 &vFormationOffset );
	const CVec3 &GetNormale( const CVec3 &vFormationOffset );

	const float GetBombPointOffset() const
	{
		return fBombPointOffset;
	}

	void AddProcessed();
	void AddAlive();
	bool IsAllProcessed() const;
	void SecondSegment( const NTimer::STime timeDiff ) override;

	void Init( const NDb::SMechUnitRPGStats *pStats,
	           const CVec3 &vCenter,
	           const float fTurnRadiusMin,
	           const float fTurnRadiusMax,
	           const CVec3 &vSpeed,
	           const CVec3 &_vNormale,
	           const float _fBombPointOffset,
	           bool bCanViolateHeghtLimits );
	void SetCanViolateHeghtLimits();

	virtual void UpdateDirection( const CVec2 &newDir )
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void UpdateDirection( const uint16_t newDir )
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	const bool SendAlongPath( struct IPath *pPath ) override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual const uint16_t GetDir() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual const CVec2 &GetDirVector() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return VNULL2;
	}

	virtual const uint16_t GetFrontDir() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual const CVec2 &GetCenter() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return VNULL2;
	}

	virtual const float GetZ() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual const CVec2 &GetSpeed() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return VNULL2;
	}

	virtual struct ISmoothPath *GetCurPath() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	void UpdatePlacement( const CVec3 &vOldPosition, const uint16_t wOldDirection, const bool bNeedUpdate ) override
	{
		NI_ASSERT( false, "Illegal call of CBasePathUnit::UpdatePlacement" );
	}

	void UpdateTile() override
	{
		NI_ASSERT( false, "Illegal call of CBasePathUnit::UpdateTile" );
	}

	// ну и уродство
	virtual const int GetUniqueIDPath() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	const float GetTurnSpeed() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual const float GetMaxSpeedHere( const CVec2 &point, bool bAdjust ) const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	const float GetMaxPossibleSpeed() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	const float GetPassability() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	const bool CanMove() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return true;
	}

	const bool CanMoveCritical() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return true;
	}

	const int GetBoundTileRadius() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 1;
	}

	const CVec2 &GetAABBHalfSize() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		static CVec2 v;
		return v;
	}

	virtual void SetCoordWOUpdate( const CVec3 &newCenter )
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void SetNewCoordinates( const CVec3 &newCenter, bool bStopUnit = true )
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual const SRect GetUnitRectForLock() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return SRect();
	}

	const SRect &GetUnitRect() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		static SRect r;
		return r;
	}

	virtual bool TurnToDir( const uint16_t &newDir, const bool bCanBackward = true, const bool bForward = true )
	{
		NI_ASSERT( false, "WRONG CALL" );
		return true;
	}

	virtual bool TurnToUnit( const CVec2 &targCenter )
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	const bool IsIdle() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	const bool IsTurning() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	void Stop() override {}

	void StopTurning() override
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void ForceGoByRightDir()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	const bool SendAlongPath( struct IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn ) override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool CheckToTurn( const uint16_t wNewDir )
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual void LockTiles()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void UnlockTiles()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void FixUnlocking()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void UnfixUnlocking()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual bool CanTurnToFrontDir( const uint16_t wDir )
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool IsInFormation() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual class CFormation *GetFormation() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual const CVec2 GetUnitPointInFormation() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return VNULL2;
	}

	virtual const int GetFormationSlot() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	const bool CanGoToPoint( const CVec2 &point ) const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	const float GetSmoothTurnThreshold() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual void SetDesirableSpeed( const float fDesirableSpeed )
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual void UnsetDesirableSpeed()
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	virtual float GetDesirableSpeed() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual void AdjustWithDesirableSpeed( float *pfMaxSpeed ) const
	{
		NI_ASSERT( false, "WRONG CALL" );
	}

	const bool CanGoBackward() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	const bool CanTurnRound() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool IsLockingTiles() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool HasSuspendedPoint() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool CanRotateTo( SRect smallRect, const CVec2 &vNewDir, bool bWithUnits, bool bCanGoBackward = true ) const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return true;
	}

	IStaticPath *CreateBigStaticPath( const CVec2 &vStartPoint,
	                                  const CVec2 &vFinishPoint,
	                                  struct IPointChecking *pPointChecking ) override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0;
	}

	virtual bool IsInOneTrain( CBasePathUnit *pUnit ) const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual bool IsTrain() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return false;
	}

	virtual const SVector GetLastKnownGoodTile() const
	{
		NI_ASSERT( false, "WRONG CALL" );
		return SVector( 0, 0 );
	}

	const bool IsDangerousDirExist() const override
	{
		return false;
	}

	const uint16_t GetDangerousDir() const override
	{
		return 0;
	}

	const SUnitProfile &GetUnitProfile() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		static SUnitProfile unitProfile;
		return unitProfile;
	}

	const float GetTurnRadius() const override
	{
		NI_ASSERT( false, "WRONG CALL" );
		return 0.0f;
	}

	// for CBasePathUnit
	const int GetUniqueID() const override
	{
		return -1;
	}

	const EAIClasses GetAIPassabilityClass() const override
	{
		return EAC_ANY;
	}

	const CVec2 GetCenterShift() const override
	{
		return VNULL2;
	}

	const bool IsRound() const override
	{
		return true;
	}

	const bool IsInfantry() const override
	{
		return false;
	}

	void UnitTrampled( const CBasePathUnit * ) override {}

	const bool CanUnitTrampled( const CBasePathUnit * ) const override
	{
		return false;
	}

	const bool IterateUnits( const CVec2 &vCenter,
	                         const float fRadius,
	                         const bool bOnlyMech,
	                         const SIterateUnitsCallback &callback ) const override
	{
		return false;
	}

	const bool IsStaticUnit() const override
	{
		return false;
	}
};
