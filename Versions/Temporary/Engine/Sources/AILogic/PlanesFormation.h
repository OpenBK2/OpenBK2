#pragma once

#include "Manuver.h"
#include "IPlane.h"
#include "PlanePreferences.h"

#include "..\Common_RTS_AI\BasePathUnit.h"

class CPlaneManuverHistory
{
	typedef hash_map<CVec3, SFormationMemberInfo, SVec3Hash> CMemberCache;
	ZDATA
	list<CObj<IManuver> > pathHistory;
	CMemberCache memberCache;
	SFormationMemberInfo curPos;
	bool bFinished;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&pathHistory); f.Add(3,&memberCache); f.Add(4,&curPos); f.Add(5,&bFinished); return 0; }
public:
	bool IsEmpty() const { return pathHistory.empty(); }
	bool IsManuverFinished() const;
	void StartManuver( IManuver *pManuver );
	IManuver * GetCurManuver() const;
	void ClearUnised();
	const SFormationMemberInfo &GetValues( const CVec3 &vOffset );
	void Advance( const NTimer::STime timeDiff );
};

// class to extend if unit to go by CPlaneSmoothPath

class CPlanesFormation : public CAIObjectBase, public IPlane, public CBasePathUnit
{
	OBJECT_NOCOPY_METHODS( CPlanesFormation );

	static int nIDSoFar;
	static hash_map<int, bool> existence;
	ZDATA_(CBasePathUnit)
	CPlaneManuverHistory pathHistory;
	CPlanePreferences preferences;

	CVec3 vPos;
	CVec3 vSpeed;
	CVec3 vNormal;

	CVec3 vNewPos;
	CVec3 vNewSpeed;
	CVec3 vNewNormal;
	
	// for member counting
	int nProcessed;
	int nAlive;

	float fBombPointOffset;
	int nID;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBasePathUnit*)this); f.Add(2,&pathHistory); f.Add(3,&preferences); f.Add(4,&vPos); f.Add(5,&vSpeed); f.Add(6,&vNormal); f.Add(7,&vNewPos); f.Add(8,&vNewSpeed); f.Add(9,&vNewNormal); f.Add(10,&nProcessed); f.Add(11,&nAlive); f.Add(12,&fBombPointOffset); return 0; }
protected:
	virtual void NullSegmTime() {}
	virtual void CheckForDestroyedObjects( const CVec2 &vCenter ) const {}

public:
	CPlanesFormation() : nID( -1 ) {}
	CPlanesFormation( int ) 
	{
		nID = nIDSoFar++;
		existence[nID] = true;
	}
	~CPlanesFormation() 
	{
		existence.erase( nID );
	}
	static void Clear();
	static bool IsFormaionExists( int nID );
	int GetFormationID() const { return nID; }
	//{ PlaneFormation interface
	bool IsManuverFinished() const;
	void CreateManuver( class CPlanesFormation *pEnemy );
	void CreateManuver( const CVec3 &vCenter, int nUniqueID );
	void CreatePointManuver( const CVec3 &vPos, const bool bToHorisontal );
	void Advance( const NTimer::STime timeDiff );
	//}
	
	//{ IPlane interface
	// current speed
	virtual CVec3 GetSpeedB2() const 
	{ 
		return vSpeed; 
	}
	// current position
	virtual CVec3 GetPosB2() const 
	{ 
		return vPos; 
	}
	// current vertical direction
	virtual CVec3 GetNormalB2() const 
	{ 
		return vNormal; 
	}

	virtual const interface IManuver * GetManuver() const 
	{ 
		return pathHistory.GetCurManuver(); 
	}

	// next calculated position
	virtual CVec3 GetPosNext() const { return vNewPos; }
	virtual CVec3 GetSpeedNext() const { return vNewSpeed; }
	virtual CVec3 GetNormalNext() const { return vNewNormal; }
	//CRAP{
	virtual void SetB2( const CVec3 &vPos, const CVec3 &vSpeed, const CVec3 &vNormal ) { }
	//CRAP}

	// текущий самолет уже находится под атакой.
	virtual bool IsBeingAttackedB2() { return false; }

	// когда враг уже готовится стрелять в самолет. нужно для того, чтобы за 1 самолетом не гонялось несколько.
	virtual void NotifyAttackedB2(IPlane & attacker, bool bAttack) { }

	// access to plane's preferences
	virtual const class CPlanePreferences & GetPreferencesB2() const 
	{ 
		return preferences; 
	}
	//}

	void SetNewPos( const CVec3 &vCenter );
	const CVec3 & GetPoint( const CVec3 &vFormationOffset );
	const CVec3 & GetSpeed( const CVec3 &vFormationOffset );
	const CVec3 & GetNormale( const CVec3 &vFormationOffset );

	const float GetBombPointOffset() const { return fBombPointOffset; }

	void AddProcessed();
	void AddAlive();
	bool IsAllProcessed() const;
	void SecondSegment( const NTimer::STime timeDiff );

	void Init( const NDb::SMechUnitRPGStats *pStats, const CVec3 &vCenter, const float fTurnRadiusMin, const float fTurnRadiusMax, const CVec3 &vSpeed, const CVec3 &_vNormale, const float _fBombPointOffset, bool bCanViolateHeghtLimits );
	void SetCanViolateHeghtLimits();

	virtual void UpdateDirection( const CVec2 &newDir ) { NI_ASSERT( false, "WRONG CALL"); }
	virtual void UpdateDirection( const WORD newDir ) { NI_ASSERT( false, "WRONG CALL"); }

	virtual const bool SendAlongPath( interface IPath *pPath ) { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const WORD GetDir() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const CVec2& GetDirVector() const { NI_ASSERT( false, "WRONG CALL"); return VNULL2; }
	virtual const WORD GetFrontDir() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const CVec2& GetCenter() const { NI_ASSERT( false, "WRONG CALL"); return VNULL2; }
	virtual const float GetZ() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const CVec2& GetSpeed() const { NI_ASSERT( false, "WRONG CALL"); return VNULL2; }
	virtual interface ISmoothPath* GetCurPath() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual void UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate )	{ NI_ASSERT( false, "Illegal call of CBasePathUnit::UpdatePlacement" ); }
	virtual void UpdateTile()	{ NI_ASSERT( false, "Illegal call of CBasePathUnit::UpdateTile" ); }

	// ну и уродство
	virtual const int GetUniqueIDPath() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const float GetTurnSpeed() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const float GetMaxSpeedHere( const CVec2 &point, bool bAdjust ) const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const float GetMaxPossibleSpeed() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const float GetPassability() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const bool CanMove() const { NI_ASSERT( false, "WRONG CALL"); return true; }
	virtual const bool CanMoveCritical() const { NI_ASSERT( false, "WRONG CALL"); return true; }
	virtual const int GetBoundTileRadius() const { NI_ASSERT( false, "WRONG CALL"); return 1; }
	virtual const CVec2 &GetAABBHalfSize() const { NI_ASSERT( false, "WRONG CALL"); static CVec2 v; return v; }
	virtual void SetCoordWOUpdate( const CVec3 &newCenter ) { NI_ASSERT( false, "WRONG CALL"); }
	virtual void SetNewCoordinates( const CVec3 &newCenter, bool bStopUnit = true ) { NI_ASSERT( false, "WRONG CALL"); }
	virtual const SRect GetUnitRectForLock() const { NI_ASSERT( false, "WRONG CALL"); return SRect(); }
	virtual const SRect & GetUnitRect() const { NI_ASSERT( false, "WRONG CALL"); static SRect r; return r; }
	virtual bool TurnToDir( const WORD &newDir, const bool bCanBackward = true, const bool bForward = true ) { NI_ASSERT( false, "WRONG CALL"); return true; }
	virtual bool TurnToUnit( const CVec2 &targCenter ) { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const bool IsIdle() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const bool IsTurning() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual void Stop() { }
	virtual void StopTurning() { NI_ASSERT( false, "WRONG CALL"); }
	virtual void ForceGoByRightDir() { NI_ASSERT( false, "WRONG CALL"); }
	virtual const bool SendAlongPath( interface IStaticPath *pStaticPath, const CVec2 &vShift, const bool bSmoothTurn ) { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool CheckToTurn( const WORD wNewDir ) { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual void LockTiles() { NI_ASSERT( false, "WRONG CALL"); }
	virtual void UnlockTiles() { NI_ASSERT( false, "WRONG CALL"); }
	virtual void FixUnlocking() { NI_ASSERT( false, "WRONG CALL"); }
	virtual void UnfixUnlocking() { NI_ASSERT( false, "WRONG CALL"); }
	virtual bool CanTurnToFrontDir( const WORD wDir ) { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool IsInFormation() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual class CFormation* GetFormation() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const CVec2 GetUnitPointInFormation() const { NI_ASSERT( false, "WRONG CALL"); return VNULL2; }
	virtual const int GetFormationSlot() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual const bool CanGoToPoint( const CVec2 &point ) const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const float GetSmoothTurnThreshold() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual void SetDesirableSpeed( const float fDesirableSpeed ) { NI_ASSERT( false, "WRONG CALL");  }
	virtual void UnsetDesirableSpeed() { NI_ASSERT( false, "WRONG CALL"); }
	virtual float GetDesirableSpeed() const { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual void AdjustWithDesirableSpeed( float *pfMaxSpeed ) const { NI_ASSERT( false, "WRONG CALL"); }
	virtual const bool CanGoBackward() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const bool CanTurnRound() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool IsLockingTiles() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool HasSuspendedPoint() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool CanRotateTo( SRect smallRect, const CVec2 &vNewDir, bool bWithUnits, bool bCanGoBackward = true ) const { NI_ASSERT( false, "WRONG CALL"); return true; }
	virtual IStaticPath* CreateBigStaticPath( const CVec2 &vStartPoint, const CVec2 &vFinishPoint, interface IPointChecking *pPointChecking ) { NI_ASSERT( false, "WRONG CALL"); return 0; }
	virtual bool IsInOneTrain( CBasePathUnit *pUnit ) const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual bool IsTrain() const { NI_ASSERT( false, "WRONG CALL"); return false; }
	virtual const SVector GetLastKnownGoodTile() const { NI_ASSERT( false, "WRONG CALL"); return SVector(0,0); }

	virtual const bool IsDangerousDirExist() const { return false; }
	virtual const WORD GetDangerousDir() const { return 0; }

	virtual const SUnitProfile &GetUnitProfile() const { NI_ASSERT( false, "WRONG CALL"); static SUnitProfile unitProfile; return unitProfile; }
	virtual const float GetTurnRadius() const { NI_ASSERT( false, "WRONG CALL"); return 0.0f;  }

	// for CBasePathUnit
	virtual const int GetUniqueID() const { return -1; }
	virtual const EAIClasses GetAIPassabilityClass() const { return EAC_ANY; }
	virtual const CVec2 GetCenterShift() const { return VNULL2; }
	virtual const bool IsRound() const { return true; }
	virtual const bool IsInfantry() const { return false; }
	virtual void UnitTrampled( const CBasePathUnit * ) {}
	virtual const bool CanUnitTrampled(const CBasePathUnit *) const { return false; }
	virtual const bool IterateUnits( const CVec2 &vCenter, const float fRadius,	const bool bOnlyMech, const SIterateUnitsCallback &callback ) const { return false; }
	const bool IsStaticUnit() const { return false; }
};

