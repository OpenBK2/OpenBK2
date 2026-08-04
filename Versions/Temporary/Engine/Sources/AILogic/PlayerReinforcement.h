#pragma once

#include "Stats_b2_m1/DBMapInfo.h"

class CCommonUnit;

// holds reinforcement points and avaliable player's reinforcements
// sends updates to client
class CPlayerReinforcement
{
	typedef det_map<int, std::pair<NDb::SReinforcementPosition,bool> > CPositions;
	typedef det_map<int/*EReinforcementType*/, CDBPtr<NDb::SReinforcement> > CInfos;
	typedef det_map<int/*EReinforcementType*/, NTimer::STime> CRecycleTimes;

	struct SCallReinforcementCommand
	{
		ZDATA
		NDb::EReinforcementType eType;
		CVec2 vPoint;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&eType); f.Add(3,&vPoint); return 0; }
		SCallReinforcementCommand() { }
		SCallReinforcementCommand( NDb::EReinforcementType _eType, const CVec2 &_vPoint )
			:eType( _eType ), vPoint( _vPoint ) { }
	};

	enum ESuperWeaponState
	{
		SWS_WAIT_FOR_CALL,
		SWS_WAIT_FOR_FLY,
		SWS_FLYING,
	};

	CArray2D<int>	unitAvailability;	// do not serialize in checksum
	ZDATA
		CPositions positions;
		
		// reinforcement info by type
		CInfos reinforcementInfos;
		NTimer::STime timeReinfButtonEnable;
		int nPlayer;

		CRecycleTimes recycleTimes;
		bool bReinfButtonEnabled;

		ZSKIP // for `unitAvailability'
		ZONSERIALIZE
		int nMapReinforcementBonus;
		float fRecycleTimeCoeff;
		std::vector<SCallReinforcementCommand> commands;
		NTimer::STime timeToCall;		// current full recycle time without coeff
		float fStoredProgress;			// for the case when progress stops and restarts
		NTimer::STime timeReinfIncrease;

		// for super weapon
		NDb::ESuperWeaponType superWeaponType;
		int nSuperWeaponShots;
		NTimer::STime timeSuperWeaponRecycleTime;
		NTimer::STime timeSuperWeaponFlyTime;

		ESuperWeaponState superWeaponState;
		NTimer::STime timeSuperWeaponFire;
		CVec2 vSuperWeaponCallPoint;
		CPtr<CCommonUnit> pSuperWeaponShell;
		int nSuperWeaponShotsLeft;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&positions); f.Add(3,&reinforcementInfos); f.Add(4,&timeReinfButtonEnable); f.Add(5,&nPlayer); f.Add(6,&recycleTimes); f.Add(7,&bReinfButtonEnabled); OnSerialize( f ); f.Add(9,&nMapReinforcementBonus); f.Add(10,&fRecycleTimeCoeff); f.Add(11,&commands); f.Add(12,&timeToCall); f.Add(13,&fStoredProgress); f.Add(14,&timeReinfIncrease); f.Add(15,&superWeaponType); f.Add(16,&nSuperWeaponShots); f.Add(17,&timeSuperWeaponRecycleTime); f.Add(18,&timeSuperWeaponFlyTime); f.Add(19,&superWeaponState); f.Add(20,&timeSuperWeaponFire); f.Add(21,&vSuperWeaponCallPoint); f.Add(22,&pSuperWeaponShell); f.Add(23,&nSuperWeaponShotsLeft); return 0; }
private:
	
private:
	void OnSerialize( IBinSaver &f ) { if ( !f.IsChecksum() ) f.Add(8,&unitAvailability); }

	// initial reinforcement fill
	void AddReinforcement( const NDb::SReinforcement *pReinforcement );
	void ModifyReinforcement( const NDb::SReinforcement *pReinforcement );

	// client connection functions
	void UpdateButtonsAfterCall( const NDb::EReinforcementType eType );
	void UpdateReinfButtonState( bool bEnable, NTimer::STime timeWhenEnabled, float fPercentComplete );
	void CheckReinfButton();
	void UpdateAddPosition( int nID, const NDb::SReinforcementPosition &point ) const;
	void UpdateDeletePosition( int nUniqueID ) const;
	void AddPosition( const NDb::SReinforcementPosition &point, int nID, bool bEnabled );
	
	void SendReinforcementToPoint( std::list< std::pair<int, CObjectBase*> > &objects, const NDb::EReinforcementType eType, const CVec2 &vPoint, const bool bIsParatroops, const float fCmdParam );
public:
	
	CPlayerReinforcement();

	void AddCallReinforcementCommand( NDb::EReinforcementType eType, const CVec2 &vPoint );
	static const NDb::SDeployTemplate * GetDeployTemplate( const NDb::SReinforcementPosition &positionToDeploy, NDb::EReinforcementType eType );
	void EnablePosition( int nPositionID, bool bEnable );
	const NDb::SReinforcementPosition * GetPosition( int nPositionID ) const;
	void InitPlayerReinforcement( int nPlayer, const NDb::SMapInfo * pMapInfo, const NDb::SAIGameConsts *_pConsts );
	void CallReinforcement( NDb::EReinforcementType eType, int nPointID, int nScriptID, std::list< std::pair<int, CObjectBase*> > *pObjects, const bool bOnWater = false, const CVec2 &vTarget = VNULL2 );
	void CallReinforcement( NDb::EReinforcementType eType, const CVec2 &vPoint, int nScriptID );
	void CallSuperWeapon();
	void ShotSuperWeapon();
	void Segment();
	
	bool CanCallNow() const;
	bool HasReinforcement( NDb::EReinforcementType eType ) const;
	NTimer::STime GetRecycleTimeLeft() const;
	const NDb::SHPObjectRPGStats * GetUnitSample( NDb::EReinforcementType eType, int nEntry ) const;
	int GetNEntries( NDb::EReinforcementType eType ) const;
	const int GetUnitAvailability( const NDb::EDBUnitRPGType eUnit, const NDb::EReinforcementType eReinf );
	int GetReinforcementCallsLeft() const;

	void SetRecycleCoeff( const float fNewCoeff );		// -1 == doesn't recycle
	
	int GetRandomPointID() const;		// randomize appear point (generally for general)
	CVec2 GetRandomPoint() const;		// randomize appear point (generally for general)

	bool HasGroundReinforcements() const;

	void PlaceInitialUnits();				// Cheating method for placing a reinforcement and not reporting it
	void GiveReinforcementCalls( int nCalls, bool bResetCounter );		// Adjust calls number (add/subtract) externally
};

class CPlayerReinforcementArray : public std::vector<CPlayerReinforcement>
{
public:
	void Segment();
	void InitPlayerReinforcementArray( const NDb::SMapInfo *pMapInfo, const NDb::SAIGameConsts *pConsts );
	void SetRecycleCoeff( const int nPlayer, const float fNewCoeff );		// -1 == doesn't recycle
	int operator&( IBinSaver &saver );

	void PlaceInitialUnits();				// Cheating method for placing a reinforcement and not reporting it
	void GiveReinforcementCalls( const int nPlayer, const int nCalls, const bool bResetCounter );		// Adjust calls number (add/subtract) externally
};

