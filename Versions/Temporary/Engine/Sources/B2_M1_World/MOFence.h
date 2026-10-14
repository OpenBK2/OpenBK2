#pragma once

// Defensive member defaults; explicit constructor values still take precedence.
#include "MapObj.h"

class CMOFence : public CMapObj
{
	OBJECT_NOCOPY_METHODS( CMOFence );
	int nSceneID = 0; // object ID in scene

	//
	const NDb::SFenceRPGStats *GetStatsLocal() const
	{
		return checked_cast<const NDb::SFenceRPGStats *>( GetStats() );
	}

public:
	CMOFence()
		: nSceneID( -1 )
	{
	}

	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;
	bool CreateSceneObject( const int nUniqueID,
	                        const SAINewUnitUpdate *pUpdate,
	                        NDb::ESeason eSeason,
	                        bool bInEditor ) override;
	void GetStatus( SObjectStatus *pStatus ) const override;
	IClientUpdatableProcess *AIUpdateRPGStats( const struct SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;
	NDb::EUserAction
	GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const override;

	virtual void GetPassangers( std::vector<IB2MapObj *> *pPassangers ) const {}

	virtual int GetPassangersCount() const
	{
		return 0;
	}

	//
	int operator&( IBinSaver &saver ) override;
};
