#pragma once

#include "MapObj.h"

class CMOObject : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CMOObject );
	bool bAnimateOnDeath;
	WORD wAmbientSound;
	WORD wCycledSound;
	//WORD wAmbientSoundTimed;
	WORD wCycledSoundTimed;
	//
	const NDb::SObjectRPGStats* GetStatsLocal() const { return checked_cast<const NDb::SObjectRPGStats*>( GetStats() ); }
public:
	CMOObject(): wAmbientSound(0), wCycledSound(0), wCycledSoundTimed(0) {}
	//
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void GetStatus( SObjectStatus *pStatus ) const;
	IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, interface IClientAckManager *pAckManager, NDb::ESeason eSeason );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	void AIUpdateFall( const SAITreeBrokenUpdate *pUpdate );

	virtual bool IsObject() const { return true; }
	virtual void GetPassangers( vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }
	//
	int operator&( IBinSaver &saver );
};

