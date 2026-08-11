#pragma once

#include "AITypes.h"

namespace NDb
{
	struct SWeaponRPGStats;
	struct SHPObjectRPGStats;
	enum EReinforcementType;
}

// ************************************************************************************************************************ //
// **
// ** notify structures
// **
// **
// **
// ************************************************************************************************************************ //

enum EMovingType
{
	MOVE_TYPE_MOVE = 0,										// на самом деле либо Move либо Turn
	MOVE_TYPE_DIVE = 1,										//for dive bombers
};

struct SSuspendedUpdate
{
	ZDATA
		int nObjUniqueID;	// subject to change
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nObjUniqueID); return 0; }
public:
	SSuspendedUpdate() : nObjUniqueID( 0 ) { }
	SSuspendedUpdate( const int _nObjUniqueID ) : nObjUniqueID( _nObjUniqueID ) { }
};

// for chage unit's visual representation
struct SChangeDBIDUpdate : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
		ZSKIP
		ZSKIP
		CDBPtr<NDb::SHPObjectRPGStats> pStats;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(4,&pStats); return 0; }
public:
};

struct SAINotifyAction : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
		WORD typeID;															// action type
		int nParam;
		NTimer::STime time;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&typeID); f.Add(3,&nParam); f.Add(4,&time); return 0; }
public:
	//
	SAINotifyAction() { }
	SAINotifyAction( const BYTE _typeID, const int nObjUniqueID ) 
		: SSuspendedUpdate( nObjUniqueID ), typeID( _typeID ), nParam( -1 ) { }
};

struct SAINotifyDeadAtAll : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
		bool bRot;															// true - если потом придёт update на исчезновение
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&bRot); return 0; }
public:
};

// RPG stats (hit points for now) update
struct SAINotifyRPGStats : public SSuspendedUpdate
{
	struct SWeaponAmmo
	{
		ZDATA
		CDBPtr<NDb::SWeaponRPGStats> pStats;
		int nAmmo;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pStats); f.Add(3,&nAmmo); return 0; }
	};
	ZDATA_( SSuspendedUpdate )
		float fHitPoints;											// hit points
		float fFuel;
		ZSKIP//int nMainAmmo
		ZSKIP//int nSecondaryAmmo;				// патроны главной пушки и всего остального
		NTimer::STime time;
		std::vector<SWeaponAmmo> ammo;
		int nSupply;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&fHitPoints); f.Add(3,&fFuel); f.Add(6,&time); f.Add(7,&ammo); f.Add(8,&nSupply); return 0; }
public:
	//
	SAINotifyRPGStats() { }
	SAINotifyRPGStats( const int nObjUniqueID, const float _fHitPoints ) 
		: SSuspendedUpdate( nObjUniqueID ), fHitPoints( _fHitPoints ) { }
};

struct SAINotifyDiplomacy : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
		EDiplomacyInfo eDiplomacy;
		int nPlayer;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&eDiplomacy); f.Add(3,&nPlayer); return 0; }
public:
	SAINotifyDiplomacy() { }
	SAINotifyDiplomacy( EDiplomacyInfo _eDiplomacy, const int nObjUniqueID, const int _nPlayer ) 
		: SSuspendedUpdate( nObjUniqueID ), eDiplomacy( _eDiplomacy ), nPlayer( _nPlayer ) { }
};

struct SAINotifyKeyBuilding : public SSuspendedUpdate
{
	ZDATA_( SSuspendedUpdate )
	int nPlayer;
	int nPrevPlayer;
	bool bStorage;
	bool bFriendLost;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SSuspendedUpdate *)this); f.Add(2,&nPlayer); f.Add(3,&nPrevPlayer); f.Add(4,&bStorage); return 0; }
public:
	SAINotifyKeyBuilding() { }
	SAINotifyKeyBuilding( const int nObjUniqueID, bool _bStorage, const int _nPlayer, const int _nPrevPlayer ) 
		: SSuspendedUpdate( nObjUniqueID ), nPlayer( _nPlayer ), nPrevPlayer( _nPrevPlayer ), bStorage( _bStorage ), bFriendLost( false ) { }
};

// update на вход/выход, юниты сначала входят не в стрелковую ячейку
struct SAINotifyEntranceState
{
	ZDATA
		int nInfantryUniqueID;											// кто входит
		int nTargetUniqueID;												// куда входит
		bool bEnter;																// true - входит, false - выходит
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nInfantryUniqueID); f.Add(3,&nTargetUniqueID); f.Add(4,&bEnter); return 0; }
public:
	SAINotifyEntranceState() : nInfantryUniqueID( 0 ), nTargetUniqueID( 0 ) { }
	SAINotifyEntranceState( const int _nInfantryUniqueID, const int _nTargetUniqueID, const bool _bEnter )
		: nInfantryUniqueID( _nInfantryUniqueID ), nTargetUniqueID( _nTargetUniqueID ), bEnter( _bEnter ) { }
};
// placement update
struct SAINotifyPlacement : public SSuspendedUpdate
{
	bool bNewFormat;											// when true OLD is ignored
	
	//OLD{
	CVec2 center;													// (x, y)
	float z;															// height (mostly for planes)
	WORD dir;															// direction [0..65535) => [0..2pi), only for units
	DWORD dwNormal;												// нормаль
	//OLD}

	//NEW{
	CVec3 vPlacement;											// unit center 
	CQuat rotation;												// unit orientation (not sexual, in world)
	//NEW}

	float fSpeed;
	float fWaterCoeff;											// 0 - dry/shore, 1 - full amphibian water offset
	BYTE cSoil;														// параметры почвы: дым из-под колёс, следы и т.д.

	SAINotifyPlacement() : bNewFormat( false ), fWaterCoeff( 0.0f ), cSoil( 0 ) { }
	SAINotifyPlacement(	const int nObjUniqueID, const CVec2 &_center, const short _z, const WORD _dir, const float _fSpeed )
		: SSuspendedUpdate( nObjUniqueID ), bNewFormat( false ), center( _center ), z( _z ), dir( _dir ), fSpeed( _fSpeed ), fWaterCoeff( 0.0f ), cSoil( 0 ) { }

	// для использования в AILogic
	virtual int operator&( IBinSaver &saver )
	{
		saver.Add( 1, (SSuspendedUpdate*)(this) );

		saver.Add( 2, &bNewFormat );
		if ( bNewFormat )
		{
			saver.Add( 3, &rotation );
			saver.Add( 4, &vPlacement );
		}
		else
		{
			saver.Add( 5, &center );
			saver.Add( 6, &z );
			saver.Add( 7, &dir );
			saver.Add( 8, &dwNormal );
		}
		saver.Add( 9, &fSpeed );
		saver.Add( 10, &cSoil );
		saver.Add( 11, &fWaterCoeff );
		
		return 0;
	}
};
// information about new unit
struct SNewUnitInfo : public SAINotifyPlacement
{
	ZDATA_( SAINotifyPlacement )
		// placement of a unit
		float fResize;												//for resizing the object
		float fHitPoints;
		float fFuel;
		ZSKIP
		ZSKIP
		EDiplomacyInfo eDipl;									// diplomacy settings
		int nPlayer;
		int nFrameIndex;											// frame index for static objects
		int nExpLevel;												// XP level of current unit
		NDb::EReinforcementType eReinfType;
		CDBPtr<NDb::SHPObjectRPGStats> pStats;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAINotifyPlacement *)this); f.Add(2,&fResize); f.Add(3,&fHitPoints); f.Add(4,&fFuel); f.Add(7,&eDipl); f.Add(8,&nPlayer); f.Add(9,&nFrameIndex); f.Add(10,&nExpLevel); f.Add(11,&eReinfType); f.Add(12,&pStats); return 0; }
public:
	SNewUnitInfo() {  }
};

// hit update
struct SAINotifyHitInfo
{
	enum EHitType { EHT_NONE, EHT_HIT, EHT_MISS, EHT_REFLECT, EHT_GROUND, EHT_WATER, EHT_AIR, EHT_AIR_WITH_CRATER };

	ZDATA
		CDBPtr<NDb::SWeaponRPGStats> pWeapon;					// weapon shell was fired
		WORD wShell;														// shell index in the weapon
		WORD wDir;															// direction hit was from
		EHitType eHitType;											// тип попадани
		int nVictimUniqueID;
		CVec3 explCoord;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pWeapon); f.Add(3,&wShell); f.Add(4,&wDir); f.Add(5,&eHitType); f.Add(6,&nVictimUniqueID); f.Add(7,&explCoord); return 0; }
public:
	SAINotifyHitInfo() : pWeapon( 0 ) { }
	SAINotifyHitInfo( const NDb::SWeaponRPGStats *_pWeapon, const WORD _wShell, const WORD &_wDir, const int _nVictimUniqueID, const CVec3 &_explCoord )
		: pWeapon( _pWeapon ), wShell( _wShell ), wDir( _wDir ), nVictimUniqueID( _nVictimUniqueID ), explCoord( _explCoord ) { }

	SAINotifyHitInfo( const NDb::SWeaponRPGStats *_pWeapon, const WORD _wShell, const WORD &_wDir, const CVec3 &_explCoord )
		: pWeapon( _pWeapon ), wShell( _wShell ), wDir( _wDir ), nVictimUniqueID( 0 ), explCoord( _explCoord ) { }
};
// aiming: turret turning update
struct SAINotifyTurretTurn
{
	ZDATA
		int nObjUniqueID;											// object turret belong to
		int nPlantform;												// turned platform
		WORD wAngle;													// final angle
		NTimer::STime endTime;								// final time
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nObjUniqueID); f.Add(3,&nPlantform); f.Add(4,&wAngle); f.Add(5,&endTime); return 0; }
public:
	SAINotifyTurretTurn() : nObjUniqueID( 0 ) { }
	SAINotifyTurretTurn( const int _nObjUniqueID, const int &_nPlantform, const WORD &_wAngle, const NTimer::STime &_endTime )
		: nObjUniqueID( _nObjUniqueID ), nPlantform( _nPlantform ), wAngle( _wAngle ), endTime( _endTime ) { }
};

struct SAINotifyBaseShot
{
	ZDATA
		int typeID;														// shot type
		int nObjUniqueID;											// юнит, который стрелял либо объект, из которого он стрелял
		BYTE cShell;													// shell number
		NTimer::STime time;										// time, this shot was...
		CVec3 vDestPos;												// destination point of this shot
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&typeID); f.Add(3,&nObjUniqueID); f.Add(4,&cShell); f.Add(5,&time); f.Add(6,&vDestPos); return 0; }
public:
	SAINotifyBaseShot() : nObjUniqueID( 0 ) {  }
	SAINotifyBaseShot( const BYTE _typeID, const int _nObjUniqueID, const BYTE _cShell, const NTimer::STime &_time, const CVec3 &_vDestPos )
		: typeID( _typeID ), nObjUniqueID( _nObjUniqueID ), cShell( _cShell ), time( _time ), vDestPos( _vDestPos ) {  }
};

struct SAINotifyMechShot : public SAINotifyBaseShot
{
	ZDATA_( SAINotifyBaseShot )
		BYTE cGun;														// gun number
		BYTE cPlatform;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( SAINotifyBaseShot *)this); f.Add(2,&cGun); f.Add(3,&cPlatform); return 0; }
public:
	SAINotifyMechShot() { }
	SAINotifyMechShot( const BYTE _typeID, const int _nObjUniqueID, const BYTE _cGun, const BYTE _cShell, const NTimer::STime &_time, const CVec3 &_vDestPos )
		: SAINotifyBaseShot( _typeID, _nObjUniqueID, _cShell, _time, _vDestPos ), cGun( _cGun ) {  }
	
};

struct SAINotifyInfantryShot : public SAINotifyBaseShot
{
	ZDATA_(SAINotifyBaseShot)
		CDBPtr<NDb::SWeaponRPGStats> pWeapon;				// оружие
		// если nSlot >= 0, то стрельбы из объекта, nSlot == -1, то стрельба в открытую
		int nSlot;											// номер слота
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(SAINotifyBaseShot*)this); f.Add(2,&pWeapon); f.Add(3,&nSlot); return 0; }
public:
// CRAP{
// Эти два параметра нужны только нам в M1
// Потому что мы не умеем другими способами передавать pWeapon
	int nPlatform;
	int nGun;
// CRAP}
	SAINotifyInfantryShot() : pWeapon( 0 ), nSlot( -1 ), nPlatform( 0 ), nGun( 0 ) {  }
	SAINotifyInfantryShot( const BYTE _typeID, const int nObjUniqueID, const int _nSlot, const BYTE _cShell, const NTimer::STime &_time, const CVec3 &_vDestPos )
		: SAINotifyBaseShot( _typeID, nObjUniqueID, _cShell, _time, _vDestPos ), nSlot( _nSlot ), nPlatform( 0 ), nGun( 0 ) {  }
};

struct SAINotifySelections
{
	ZDATA
		int nObjUniqueID;
		bool bSelectable;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nObjUniqueID); f.Add(3,&bSelectable); return 0; }
public:
};

struct SAINotifyNewProjectile
{
	ZDATA
		int nObjUniqueID;
		int nSourceUniqueID;

		int nGun;
		int nPlatform;
		int nShell;

		CVec3 vAIStartPos;
		NTimer::STime timeToEqualizePos;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nObjUniqueID); f.Add(3,&nSourceUniqueID); f.Add(4,&nGun); f.Add(5,&nPlatform); f.Add(6,&nShell); f.Add(7,&vAIStartPos); f.Add(8,&timeToEqualizePos); return 0; }
public:
	SAINotifyNewProjectile() : nObjUniqueID( 0 ) { }
	SAINotifyNewProjectile( const int _nObjUniqueID, const int _nSourceUniqueID, const int _nPlatform, const int _nGun, const int _nShell, const CVec3 &_vAIStartPos, const NTimer::STime _timeToEqualizePos )
		: nObjUniqueID( _nObjUniqueID ), nSourceUniqueID( _nSourceUniqueID ), nPlatform( _nPlatform ), nGun( _nGun ), nShell( _nShell ), vAIStartPos( _vAIStartPos ), timeToEqualizePos( _timeToEqualizePos ) { }
};

struct SAINotifyUpdateLaseMark
{
	ZDATA
		int nLaserMarkID;
		int nUnitID;

		int nPlatform;
		int nGun;
		
		CVec3 vTarget;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLaserMarkID); f.Add(3,&nUnitID); f.Add(4,&nPlatform); f.Add(5,&nGun); f.Add(6,&vTarget); return 0; }
public:
	SAINotifyUpdateLaseMark() : nLaserMarkID( -1 ), nUnitID( -1 ), nPlatform( -1 ), nGun( -1 ), vTarget( VNULL3 ) {}
	SAINotifyUpdateLaseMark( const int _nLaserMarkID, const int _nUnitID, const int _nPlatform, const int _nGun, const CVec3 &_vTarget )
		: nLaserMarkID( _nLaserMarkID ), nUnitID( _nUnitID ), nPlatform( _nPlatform ), nGun( _nGun ), vTarget( _vTarget ) {}
};


