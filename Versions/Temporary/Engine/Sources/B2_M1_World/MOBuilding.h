#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "MapObj.h"

#include "B2_M1_World_export.h"

#include <cstdint>

class B2_M1_WORLD_EXPORT CMOBuilding : public IMOContainer
{
	OBJECT_NOCOPY_METHODS( CMOBuilding );

	std::vector<CPtr<CMOSelectable>> vPassangers;

	enum EWindowState
	{
		EWS_NONE,
		EWS_DAY,
		EWS_NIGHT,
		EWS_DESTROYED
	};

	std::vector<EWindowState> attachedWindows;
	typedef std::list<std::pair<int, int>> CAttachedObjIDs;
	std::vector<CAttachedObjIDs> attachedObjects;
	std::vector<float> attachedObjectsHP;
	float fMaxDistance = 0.0f;
	float fBuildingHP = 0.0f;
	uint16_t wAmbientSound = 0;
	uint16_t wCycledSound = 0;
	//uint16_t wAmbientSoundTimed;
	uint16_t wCycledSoundTimed = 0;
	bool bStorage = false;

	std::unordered_set<int> projectilesAlreadyHit;
	int nOldModelStage = 0;
	int nCurrentAmmo = 0;
	float fCapturingProgress = 0.0f;
	int nCapturingColorIndex = 0;
	bool bCanEnter = false;

private:
	bool IsInside( const int nID );
	const NDb::SBuildingRPGStats *GetStats() const override;

	const NDb::SVisObj *GetWindowObj( const int nWindow, const EWindowState eState );
	void DetachWindow( const int nWindow, const NDb::ESeason eSeason );
	const int AttachWindow( const int nWindow, const NDb::ESeason eSeason, const EWindowState eState );
	void AddDynamicDebris( const NDb::SModel *pDeadModel, const CVec3 &vPos, const CQuat &qRot, NDb::ESeason eSeason );
	void AttachEffectToSlot( const int nSlot,
	                         const ESceneSubObjType eType,
	                         const NDb::SEffect *pEffect,
	                         const NTimer::STime _time,
	                         const bool bTurnWithWind,
	                         const bool bRemoveObject );
	void AttachComplexEffectToSlot( const int nSlot,
	                                const ESceneSubObjType eType,
	                                const NDb::SComplexEffect *pEffect,
	                                const NTimer::STime _time,
	                                const bool bRemoveObject );
	const int AttachSubObjectToSlot( const int nSlot, const ESceneSubObjType eType, const NDb::SModel *pSubModel );
	void RemoveSubObjectsFromSlot( const int nSlot, const ESceneSubObjType eType );
	const int GetSlotHPIndex( const int nWindow, const EWindowState eState ) const;
	bool IsHitbarVisible() const;

protected:
	void FillIconsInfo( SSceneObjIconInfo &iconInfo ) override;
	NDb::EUserAction
	GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const override;

public:
	CMOBuilding();
	//
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
	IClientUpdatableProcess *AIUpdateRPGStats( const SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override;

	IClientUpdatableProcess *AIUpdateDamage( int nProjectileID,
	                                         float fDamage,
	                                         const std::list<int> &probableHitAttached,
	                                         struct IScene *pScene,
	                                         NDb::ESeason eSeason,
	                                         bool bFromAIUpdate ) override;
	void AIUpdateKeyObject( const struct SAINotifyKeyBuilding &update ) override;
	void AIUpdateKeyObjectCaptureProgress( float fProgress, int nColorIndex ) override;

	// Create day/night dependent stuff (windows)
	void SwitchLightFX( const bool bNewState,
	                    const NDb::ESeason eSeason,
	                    const bool bIsNight,
	                    const bool bInEditor = false ) override;

	// break window
	const int BreakWindow( const int nWindow, const NDb::ESeason eSeason );
	void BreakAllWindows( const NDb::ESeason eSeason );
	// on/of night windows
	void ToggleNightWindows( const bool bNightOn, const NDb::ESeason eSeason );

	//
	bool Load( struct IMOUnit *pMO, bool bEnter ) override
	{
		return false;
	}

	bool LoadSquad( struct IMOSquad *pSquad, bool bEnter ) override;

	void UpdatePassangers() override {}

	void GetPassangers( std::vector<CMOSelectable *> *pBuffer ) const override;

	int GetPassangersCount() const override
	{
		return vPassangers.size();
	}

	int GetFreePlaces() const override;

	int GetFreeMechPlaces() const override
	{
		return 0;
	}

	void AIUpdateShot( const struct SAINotifyBaseShot &shot,
	                   const NTimer::STime &currTime,
	                   IScene *pScene,
	                   NDb::ESeason eSeason ) override;
	//
	bool NeedShowInterrior() const override;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;
	//void GetSpecialAbilities( CSpecialAbilities *pAbilities ) const {}
	//
	void Select( bool bSelect ) override;
	void SetCanSelect( bool bCanSelect ) override;

	void AIUpdateModifyEntranceState( bool _bOpen ) override
	{
		bCanEnter = _bOpen;
	}

	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override;

	bool IsOpen() const override
	{
		return true;
	}

	bool IsStorage() const
	{
		return bStorage;
	}

	virtual bool IsBuilding() const
	{
		return true;
	}

	//
	int operator&( IBinSaver &saver ) override;
};
