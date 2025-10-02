#pragma once

#include "ActionNotify.h"
#include "Actions.h"
#include "CameraRunTypes.h"

namespace NDb
{
	struct SVisObj;
}

//*******************************************************************
//*                     UpdatableClient                             *
//*******************************************************************

struct IUpdatableClient
{
	virtual void ProcessUpdate( const struct SAIPointLightUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SAIHeadLightUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SAIToggleDayNightWindowsUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SAIBreakWindowUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SAINewProjectileUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SAIDeadProjectileUpdate *pUpdate ) = 0;
//	virtual void ProcessUpdate( const struct SAINewProjectileM1 *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SExplodeProjectileUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SScriptCameraRunUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SScriptCameraResetUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SScriptCameraStartMovieUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SScriptCameraStopMovieUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SClientUpdateButtonsUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SClientUpdateSingleUnitUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SWeatherChangedUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SLaserMarkUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SChatMessageUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SWinLoseUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SStartStopSequenceUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SMSChangePlaylistUpdate *pUpdate ) = 0;
//	virtual void ProcessUpdate( const struct SMSPlayVoiceUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SMSSetVolumeUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SMSPauseMusicUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SMoneyChangedUpdate *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SObjectiveChanged *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SEnableAirStrike *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SMemCameraPos *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SSetCameraToMemPos *pUpdate ) = 0;
	virtual void ProcessUpdate( const struct SChatClear *pUpdate ) = 0;
};

#define REGISTER_TO_UPDATES																					\
virtual void ProcessClient( IUpdatableClient *pClient ) const				\
{																																		\
	pClient->ProcessUpdate( this );																		\
}

//*******************************************************************
//*                   Updates                                       *
//*******************************************************************

// basic class
struct SAIBasicUpdate : public CObjectBase
{
	ZDATA
		EActionNotify eUpdateType;
		NTimer::STime nUpdateTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eUpdateType); f.Add(3,&nUpdateTime); return 0; }

public:
	SAIBasicUpdate() : eUpdateType( ACTION_NOTIFY_NONE ), nUpdateTime( 0 ) {}
	SAIBasicUpdate( const EActionNotify _eUpdateType, const NTimer::STime _nUpdateTime )
		: eUpdateType( _eUpdateType ), nUpdateTime( _nUpdateTime ) { }
	// skip unknown updates, for start word as soon as possible
	virtual void ProcessClient( IUpdatableClient *pClient ) const
	{
		//		DebugTrace( "Update not found for type %s", typeid(*this).name() );
	}
};

struct SAIActionUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIActionUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nObjUniqueID;
		int nParam;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&nParam); return 0; }

	SAIActionUpdate() { }
	SAIActionUpdate( const int _nObjUniqueID, const EActionNotify eUpdateType, const int _nParam, const NTimer::STime nUpdateTime )
		: SAIBasicUpdate( eUpdateType, nUpdateTime ), nObjUniqueID( _nObjUniqueID ), nParam( _nParam ) { }
};

struct SAIPlacementUpdateBase : public SAIBasicUpdate
{
	ZDATA_(SAIBasicUpdate)
	SAINotifyPlacement info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIPlacementUpdate : public SAIPlacementUpdateBase 
{
private:
	OBJECT_BASIC_METHODS( SAIPlacementUpdate )
	ZDATA_( SAIPlacementUpdateBase )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIPlacementUpdateBase *)this); return 0; }
public:
};

struct SAIChangeVisibilityUpdate : public SAIPlacementUpdateBase
{
private:
	OBJECT_BASIC_METHODS( SAIChangeVisibilityUpdate )
public:
	ZDATA_(SAIPlacementUpdateBase)
	bool bVisible;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIPlacementUpdateBase*)this); f.Add(2,&bVisible); return 0; }
};

struct SAIRPGUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIRPGUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyRPGStats info;	
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SParentOfAtomObjectUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SParentOfAtomObjectUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nAtomObjectID;
		int nParentID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nAtomObjectID); f.Add(3,&nParentID); return 0; }

	SParentOfAtomObjectUpdate() : nAtomObjectID( -1 ), nParentID( -1 ) { }
	SParentOfAtomObjectUpdate( int _nAtomObjectID, int _nParentID )
		: nAtomObjectID( _nAtomObjectID ), nParentID( _nParentID ) { eUpdateType = ACTION_NOTIFY_PARENT_OF_ATOM_OBJ; }
};

struct SAIDamageUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIDamageUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nObjUniqueID;
		int nProjectileUniqueID;
		float fDamage;
		CVec3 vExplosionPos;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&nProjectileUniqueID); f.Add(4,&fDamage); f.Add(5,&vExplosionPos); return 0; }
public:
	SAIDamageUpdate() : nObjUniqueID( -1 ), nProjectileUniqueID( -1 ), fDamage( 0.0f ) {}
	SAIDamageUpdate( const int _nObjUniqueID, const int _nProjectileUniqueID, const float _fDamage, const CVec3 &vExplosionPos )
		: SAIBasicUpdate( ACTION_NOTIFY_DAMAGE, 0 ), nObjUniqueID( _nObjUniqueID ), nProjectileUniqueID( _nProjectileUniqueID ), fDamage( _fDamage ) {}
};

struct SAIMechShotUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIMechShotUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyMechShot info;		
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIInfantryShotUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIInfantryShotUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyInfantryShot info;		
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIHitUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIHitUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyHitInfo info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAITurretUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAITurretUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyTurretTurn info;			
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIEntranceUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIEntranceUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyEntranceState info;				
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIDiplomacyUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIDiplomacyUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyDiplomacy info;					
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIKeyBuildingUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIKeyBuildingUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyKeyBuilding info;					
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIKeyBuildingCaptureUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIKeyBuildingCaptureUpdate )
public:
	ZDATA_(SAIBasicUpdate)
	int nObjUniqueID;
	int nOldSide;
	int nNewSide;
	float fProgress;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&nOldSide); f.Add(4,&nNewSide); f.Add(5,&fProgress); return 0; }
};

struct SAIShootAreaUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIShootAreaUpdate )
public:
	ZDATA_(SAIBasicUpdate)	
		vector<SShootAreas> info;		
		int nObjID;				
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); f.Add(3,&nObjID); return 0; }
};

struct SAINewUnitUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAINewUnitUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SNewUnitInfo info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
};

struct SAIDeadUnitUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIDeadUnitUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyAction dieAnimation;
		NTimer::STime dieTime;
		EActionNotify dieAction;
		int nFatality;
		int nDeadObj;
		SAINotifyPlacement placement;
		bool bVisibleWhenDie;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&dieAnimation); f.Add(3,&dieTime); f.Add(4,&dieAction); f.Add(5,&nFatality); f.Add(6,&nDeadObj); f.Add(7,&placement); f.Add(8,&bVisibleWhenDie); return 0; }
};

struct SAIDissapearObjUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIDissapearObjUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nDissapearObjID;
		bool bShowEffects;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nDissapearObjID); f.Add(3,&bShowEffects); return 0; }

	SAIDissapearObjUpdate() : nDissapearObjID( -1 ), bShowEffects( true ) { }
};

struct SAITrenchUpdate : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAITrenchUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SSegment2Trench info;
		bool						bLast;
		bool						bDigBySegment;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); f.Add(3,&bLast); f.Add(4,&bDigBySegment); return 0; }
public:
	SAITrenchUpdate(): bLast( false ) { }
};

struct SAIFormationUpdate : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAIFormationUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SSoldier2Formation info;						
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
public:
	SAIFormationUpdate() { }
};

struct SAICircleUpdate : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAICircleUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		CCircle info;
		int			nParam;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); f.Add(3,&nParam); return 0; }
};

struct SAIChangeDBIDUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAIChangeDBIDUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SChangeDBIDUpdate info; 
	int nObjUniqueID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); f.Add(3,&nObjUniqueID); return 0; }
};

struct SParadropStartFinishUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SParadropStartFinishUpdate )
public:
	ZDATA_(SAIBasicUpdate)
	bool bStart;
	ZSKIP
	ZSKIP
	int nObjUniqueID;				// soldier ID 
	CDBPtr<NDb::SVisObj> pNewSoldierVisObj;
	CDBPtr<NDb::SVisObj> pParachuteVisObj;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&bStart); f.Add(5,&nObjUniqueID); f.Add(6,&pNewSoldierVisObj); f.Add(7,&pParachuteVisObj); return 0; }
};

struct SAITreeBrokenUpdate : public SAIBasicUpdate
{
private:
	OBJECT_BASIC_METHODS( SAITreeBrokenUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		CVec2 vDir;
	float fTg;
	int nObjUniqueID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&vDir); f.Add(3,&fTg); f.Add(4,&nObjUniqueID); return 0; }
};

struct SAIModifyEntranceStateUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIModifyEntranceStateUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nObjUniqueID;
		bool bOpen;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&bOpen); return 0; }
public:
	SAIModifyEntranceStateUpdate() :
		nObjUniqueID( 0 ), bOpen( true ) {}
	SAIModifyEntranceStateUpdate( const int _nObjUniqueID, const bool _bOpen ) :
		nObjUniqueID( _nObjUniqueID ), bOpen( _bOpen ) {}
};

struct SAIPointLightUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIPointLightUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nObjUniqueID;
	int nPointLight;
	bool bLight;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&nPointLight); f.Add(4,&bLight); return 0; }
public:
	SAIPointLightUpdate() : nPointLight( 0 ), bLight( false ) { }
	SAIPointLightUpdate( const int _nObjUniqueID, const int _nPointLight, const bool _bLight )
		: nObjUniqueID( _nObjUniqueID ), nPointLight( _nPointLight ), bLight( _bLight ) { }

		REGISTER_TO_UPDATES
};

struct SAIHeadLightUpdate : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAIHeadLightUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		int nObjUniqueID;
		int nHeadLight;
		bool bLight;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&nObjUniqueID); f.Add(3,&nHeadLight); f.Add(4,&bLight); return 0; }
public:
	SAIHeadLightUpdate() : nObjUniqueID( 0 ), nHeadLight( 0 ), bLight( false ) { }
	SAIHeadLightUpdate( const int _nObjUniqueID, const int _nHeadLight, const bool _bLight )
		: nObjUniqueID( _nObjUniqueID ), nHeadLight( _nHeadLight ), bLight( _bLight ) { }

	REGISTER_TO_UPDATES
};

struct SAIToggleDayNightWindowsUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIToggleDayNightWindowsUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		int nObjUniqueID;
		bool bNightOn;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nObjUniqueID); f.Add(3,&bNightOn); return 0; }
public:
	SAIToggleDayNightWindowsUpdate() : nObjUniqueID( 0 ), bNightOn( false ) { }
	SAIToggleDayNightWindowsUpdate( const int _nObjUniqueID, const bool _bNightOn )
		: nObjUniqueID( _nObjUniqueID ), bNightOn( _bNightOn ) { }

	REGISTER_TO_UPDATES
};

struct SAIBreakWindowUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIBreakWindowUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		int nObjUniqueID;
		int nWindow;							// if -1, when break all windows of the object
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nObjUniqueID); f.Add(3,&nWindow); return 0; }
public:
	SAIBreakWindowUpdate() : nObjUniqueID( 0 ), nWindow( -1 ) { }
	SAIBreakWindowUpdate( const int _nObjUniqueID, const int _nWindow )
		: nObjUniqueID( _nObjUniqueID ), nWindow( _nWindow ) { }

	REGISTER_TO_UPDATES
};

struct SAINewProjectileUpdate : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAINewProjectileUpdate )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyNewProjectile info;						
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); return 0; }
public:
	REGISTER_TO_UPDATES
};

namespace NDb
{
	struct SWeaponRPGStats;
}

struct SAINewProjectileM1 : public SAIBasicUpdate
{
	OBJECT_BASIC_METHODS( SAINewProjectileM1 )
public:
	ZDATA_(SAIBasicUpdate)
		SAINotifyNewProjectile info;
		int nTargetID;
		float fDamage;
		bool bTraceTargetIntersection;
		int nWeaponID;
		CVec2 vDirection;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAIBasicUpdate*)this); f.Add(2,&info); f.Add(3,&nTargetID); f.Add(4,&fDamage); f.Add(5,&bTraceTargetIntersection); f.Add(6,&nWeaponID); f.Add(7,&vDirection); return 0; }
public:
	SAINewProjectileM1() : nTargetID( -1 ), fDamage( 0 ) { }
	SAINewProjectileM1( int _nTargetID, float _fDamage, bool _bTraceTargetIntersection, int _nWeaponID, const CVec2 &vDir, const SAINotifyNewProjectile &_info )
		: nTargetID( _nTargetID ), fDamage( _fDamage ), info( _info ), nWeaponID( _nWeaponID ), vDirection( vDir ), bTraceTargetIntersection( _bTraceTargetIntersection ) { }

//	REGISTER_TO_UPDATES
};

struct SAIDeadProjectileUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIDeadProjectileUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		int nProjectileUnqiueID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nProjectileUnqiueID); return 0; }
public:
	SAIDeadProjectileUpdate() : nProjectileUnqiueID( 0 ) { }
	SAIDeadProjectileUpdate( const int _nProjectileUnqiueID )
		: nProjectileUnqiueID( _nProjectileUnqiueID ) { }

	REGISTER_TO_UPDATES
};

struct SExplodeProjectileUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SExplodeProjectileUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		SAINotifyHitInfo::EHitType eHitType;
		CVec3 vExplCenter;
		CVec3 vExplDir;
		int nProjectileID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&eHitType); f.Add(3,&vExplCenter); f.Add(4,&vExplDir); f.Add(5,&nProjectileID); return 0; }
public:
	SExplodeProjectileUpdate() : vExplCenter( VNULL3 ), vExplDir( VNULL3 ), nProjectileID( -1 ) { }
	SExplodeProjectileUpdate( int _nProjectileID, const CVec3 &_vExplCenter, const CVec3 &_vExplDir, SAINotifyHitInfo::EHitType _eHitType )
		: nProjectileID( _nProjectileID ), vExplCenter( _vExplCenter ), vExplDir( _vExplDir ), eHitType( _eHitType ) { }

	REGISTER_TO_UPDATES
};

struct SScriptCameraRunUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SScriptCameraRunUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		string szStartCam;
		string szFinishCam;
		float fTime;
		float fLinSpeed;
		float fAngle;
		int nTargetID;
		float fSpline1;
		float fSpline2;
		NDb::EScriptCameraRunType eRunType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&szStartCam); f.Add(3,&szFinishCam); f.Add(4,&fTime); f.Add(5,&fLinSpeed); f.Add(6,&fAngle); f.Add(7,&nTargetID); f.Add(8,&fSpline1); f.Add(9,&fSpline2); f.Add(10,&eRunType); return 0; }
public:
	SScriptCameraRunUpdate() {}
	SScriptCameraRunUpdate( const string &_szStartCam, const string &_szFinshCam, float _fTime )
		: szStartCam( _szStartCam ),
		szFinishCam( _szFinshCam ),
		fTime( _fTime )
	{}

	REGISTER_TO_UPDATES
};

struct SScriptCameraResetUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SScriptCameraResetUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		NDb::EScriptCameraRunType eRunType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&eRunType); return 0; }
public:
	SScriptCameraResetUpdate() {}

	REGISTER_TO_UPDATES
};

struct SScriptCameraStartMovieUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SScriptCameraStartMovieUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		int nMovieIndex;
	bool bLoopPlayback;
	string szCallbackFuncName;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nMovieIndex); f.Add(3,&bLoopPlayback); f.Add(4,&szCallbackFuncName); return 0; }
public:
	SScriptCameraStartMovieUpdate() {}

	REGISTER_TO_UPDATES
};

struct SScriptCameraStopMovieUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SScriptCameraStopMovieUpdate )
public:
	SScriptCameraStopMovieUpdate() {}

	REGISTER_TO_UPDATES
};

struct SWeatherChangedUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SWeatherChangedUpdate )
public:
	ZDATA_( SAIBasicUpdate )
		bool bActive;
		int nTimeTo;		
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&bActive); f.Add(3,&nTimeTo); return 0; }
public:
	SWeatherChangedUpdate() {}

	REGISTER_TO_UPDATES
};

struct SPlaneReturnsUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SPlaneReturnsUpdate )
public:
	SPlaneReturnsUpdate() {}
};

struct SAIObjectsUnderConstructionUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SAIObjectsUnderConstructionUpdate );
	struct SObjectUnderConstruction
	{
		SNewUnitInfo info;
		bool bCanBuild;
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &info );
			saver.Add( 2, &bCanBuild );
			return 0;
		}
	};
	// if bSet == false - delete all objects under construction, all other fields are ignored.
public:
	bool bSet;															
	vector<SObjectUnderConstruction> objects;
	vector<CVec2> impossibleToBuildTiles;
	vector<CVec2> buildTiles;
	bool bCanBuild;
public: 
	int operator&( IBinSaver &f ) 
	{ 
		if ( !f.IsChecksum() )
		{
			f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&bSet); 
			f.Add(3,&objects); 
			f.Add(4,&impossibleToBuildTiles);
			f.Add(5,&buildTiles); 
			f.Add(6,&bCanBuild); 
		}
		return 0; 
	}
public:
	SAIObjectsUnderConstructionUpdate() : bCanBuild( true ) {  }
	SAIObjectsUnderConstructionUpdate( const bool _bSet ) : bSet( _bSet ), bCanBuild( true ) {  }
};

struct SClientUpdateButtonsUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SClientUpdateButtonsUpdate );
	ZDATA_( SAIBasicUpdate )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); return 0; }
public:
	REGISTER_TO_UPDATES	
};

struct SClientUpdateSingleUnitUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SClientUpdateSingleUnitUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		int nUnitID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nUnitID); return 0; }
	SClientUpdateSingleUnitUpdate() {}
	SClientUpdateSingleUnitUpdate( const int &_nUnitID ) : nUnitID( _nUnitID ) {}

	REGISTER_TO_UPDATES
};

namespace NDb
{
	struct SComplexEffect;
}

struct SPlayEffectUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SPlayEffectUpdate );
public:
	ZDATA_( SAIBasicUpdate )
	CVec3 vPos;
	CDBPtr<NDb::SComplexEffect> pEffect;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&vPos); f.Add(3,&pEffect); return 0; }
	SPlayEffectUpdate( const NDb::SComplexEffect *_pEffect, const CVec3 &_vPos ) : SAIBasicUpdate( ACTION_NOTIFY_PLAY_EFFECT, 0 ), pEffect( _pEffect ), vPos( _vPos ) { }
	SPlayEffectUpdate() { }
};

struct SLaserMarkUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SLaserMarkUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		SAINotifyUpdateLaseMark info;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&info); return 0; }
	SLaserMarkUpdate() : info() {}
	SLaserMarkUpdate( const int _nLaserMarkID, const int _nUnitID, const int _nPlatform, const int _nGun, const CVec3 &_vTarget )
		: info( _nLaserMarkID, _nUnitID, _nPlatform, _nGun, _vTarget ) {}

	REGISTER_TO_UPDATES
};

struct SChatMessageUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SChatMessageUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		wstring wszMessage;
		DWORD dwColor;
		bool bAnotherChat;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&wszMessage); f.Add(3,&dwColor); f.Add(4,&bAnotherChat); return 0; }
	SChatMessageUpdate() {}
	SChatMessageUpdate( const wstring &_wszMessage, const DWORD &_dwColor, const bool &_bAnotherChat ) 
		: wszMessage( _wszMessage ), dwColor( _dwColor ), bAnotherChat( _bAnotherChat ) {}

	REGISTER_TO_UPDATES
};

struct SWinLoseUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SWinLoseUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		bool bWin;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&bWin); return 0; }
	SWinLoseUpdate() {}
	SWinLoseUpdate( const bool &_bWin ) : bWin( _bWin ) {}

	REGISTER_TO_UPDATES
}; 

struct SMoneyChangedUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMoneyChangedUpdate );
public:
	ZDATA_( SAIBasicUpdate )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); return 0; }
	SMoneyChangedUpdate() {}

	REGISTER_TO_UPDATES
};

struct SStartStopSequenceUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SStartStopSequenceUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		bool bStart;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&bStart); return 0; }
	SStartStopSequenceUpdate() {}
	SStartStopSequenceUpdate( const bool &_bStart ) : bStart( _bStart ) {}

	REGISTER_TO_UPDATES
}; 

// "MS" means Music System

struct SMSPlayVoiceUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMSPlayVoiceUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		int nVoiceID;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nVoiceID); return 0; }
	SMSPlayVoiceUpdate() {}
	SMSPlayVoiceUpdate( const int &_nVoiceID ) : nVoiceID( _nVoiceID ) {}

//	REGISTER_TO_UPDATES
};

struct SMSChangePlaylistUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMSChangePlaylistUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		int nPlayList;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nPlayList); return 0; }
	SMSChangePlaylistUpdate() {}
	SMSChangePlaylistUpdate( const int &_nPlayList ) : nPlayList( _nPlayList ) {}

	REGISTER_TO_UPDATES
};

struct SMSSetVolumeUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMSSetVolumeUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		int nVolumeType;
		float fVolume;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nVolumeType); f.Add(3,&fVolume); return 0; }
	SMSSetVolumeUpdate() {}
	SMSSetVolumeUpdate( const int &_nVolumeType, const float &_fVolume ) : nVolumeType( _nVolumeType ), fVolume( _fVolume ) {}

	REGISTER_TO_UPDATES
};

struct SMSPauseMusicUpdate : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMSPauseMusicUpdate );
public:
	ZDATA_( SAIBasicUpdate )
		int nVolumeType;
		bool bPause;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nVolumeType); f.Add(3,&bPause); return 0; }
	SMSPauseMusicUpdate() {}
	SMSPauseMusicUpdate( const int &_nVolumeType, const bool &_bPause ) : nVolumeType( _nVolumeType ), bPause( _bPause ) {}

	REGISTER_TO_UPDATES
};

struct SObjectiveChanged : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SObjectiveChanged );
public:
	ZDATA_( SAIBasicUpdate )
		int nObjectiveNumber;
		int nStatus;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nObjectiveNumber); f.Add(3,&nStatus); return 0; }
	SObjectiveChanged() {}
	SObjectiveChanged( const int &_nObjectiveNumber, const int &_nStatus ) : nObjectiveNumber( _nObjectiveNumber ), nStatus( _nStatus ){}

	REGISTER_TO_UPDATES

};

struct SEnableAirStrike : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SEnableAirStrike );
public:
	ZDATA_( SAIBasicUpdate )
		bool bEnable;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&bEnable); return 0; }
	SEnableAirStrike() {}
	SEnableAirStrike( const bool &_bEnable ) : bEnable( _bEnable ) {}

	REGISTER_TO_UPDATES
};

struct SMemCameraPos : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SMemCameraPos )
public:
	ZDATA_( SAIBasicUpdate )
		int nPos;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nPos); return 0; }

	SMemCameraPos() : nPos( -1 ) { }
	SMemCameraPos( const int _nPos ) : nPos( _nPos ) { }

	REGISTER_TO_UPDATES
};

struct SSetCameraToMemPos : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SSetCameraToMemPos )
public:
	ZDATA_( SAIBasicUpdate )
		int nPos;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); f.Add(2,&nPos); return 0; }

	SSetCameraToMemPos() : nPos( -1 ) { }
	SSetCameraToMemPos( const int _nPos ) : nPos( _nPos ) { }

	REGISTER_TO_UPDATES
};

struct SChatClear : public SAIBasicUpdate
{
	OBJECT_NOCOPY_METHODS( SChatClear )
public:
	ZDATA_( SAIBasicUpdate )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAIBasicUpdate *)this); return 0; }

	SChatClear() {}
	REGISTER_TO_UPDATES
};


