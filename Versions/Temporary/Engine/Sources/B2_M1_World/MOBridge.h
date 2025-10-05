#pragma once

#include "MapObj.h"

class CMOBridge : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CMOBridge );
	
	CDBPtr<NDb::SBridgeRPGStats> pStats;
	int nFrameIndex;
	int nRandomSpan;
	bool bDestroyed;			// once destroyed, bridge is not repeared untill full HP
	const NDb::SBridgeRPGStats::SElementRPGStats& GetElement() const;
	int GetDamagedState( float fHP ) const;
	const NDb::SVisObj* GetVisObjForHP( float fHP, int *pnDamagedState );
	void PlayDeathAnimation( const NTimer::STime timeStart, const bool bInstant );
public:
	bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void GetStatus( SObjectStatus *pStatus ) const;
	//void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason ) {}
	IClientUpdatableProcess* AIUpdateRPGStats( const SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const;
	bool IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const;
	virtual void GetPassangers( std::vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }

	//
	void FinalizeDeath( NDb::ESeason eSeason );
	int operator&( IBinSaver &saver );
};


