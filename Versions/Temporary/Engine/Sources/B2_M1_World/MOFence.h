#pragma once
#include "mapobj.h"


class CMOFence : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CMOFence );
	int nSceneID;	// object ID in scene
	//
	const NDb::SFenceRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SFenceRPGStats*>( GetStats() ); }
public:
	CMOFence() : nSceneID( -1 ) {  }
	bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void GetStatus( SObjectStatus *pStatus ) const;
	IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const;
	virtual void GetPassangers( vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }
	//
	int operator&( IBinSaver &saver );
};


