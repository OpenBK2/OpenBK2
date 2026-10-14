#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "MapObj.h"

#include <cstdint>

class CMOEntrenchment;

class CMOEntrenchmentPart : public CMOSelectable
{
	OBJECT_NOCOPY_METHODS( CMOEntrenchmentPart );

	CPtr<CMOEntrenchment> pParent;

	// Cached transformation
	struct SCachedInfo
	{
		//Indentifying parameters
		CVec3 vPos;
		uint16_t wDir = 0;
		//Derived parameters
		CQuat qRot;
		SHMatrix mPlace;
		//Terrain hole params
		CVec2 vHoleStart;
		CVec2 vHoleEnd;
		float fHoleWidth = 0.0f;
	};

	SCachedInfo cached;
	int nFrameIndex = 0;

	void DeriveTransform( const CVec3 &_vPos, const uint16_t _wDir );

public:
	CMOEntrenchmentPart()
		: pParent( 0 )
		, nFrameIndex( 0 )
	{
		cached.vPos.Set( 0, 0, 0 );
		cached.wDir = 0;
		Identity( &cached.mPlace );
		cached.qRot = QNULL;
		cached.vHoleStart.Set( 0, 0 );
		cached.vHoleEnd.Set( 0, 0 );
		cached.fHoleWidth = 0;
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
	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override;

	IClientUpdatableProcess *AIUpdateRPGStats( const struct SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override
	{
		return 0;
	}

	//
	void SetParent( CMOEntrenchment *pTrench )
	{
		pParent = pTrench;
	}

	CMOEntrenchment *GetParent()
	{
		return pParent;
	}

	//
	void GetHoleParams( CVec2 *pStart, CVec2 *pEnd, float *pWidth );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;

	//
	bool IsSelected() const override
	{
		return false;
	}

	void Select( bool bSelect ) override
	{
		NI_ASSERT( !bSelect, "Selecting entrenchment!" );
	}

	bool CanSelect() const override
	{
		return false;
	}

	void SetCanSelect( bool _bCanSelect ) override
	{
		NI_ASSERT( !_bCanSelect, "Selecting entrenchment!" );
	}

	virtual void GetPassangers( std::vector<IB2MapObj *> *pPassangers ) const {}

	virtual int GetPassangersCount() const
	{
		return 0;
	}

	//
	int operator&( IBinSaver &saver ) override;
};

class CMOEntrenchment : public CMOSelectable
{
	OBJECT_NOCOPY_METHODS( CMOEntrenchment );

	typedef std::list<CPtr<CMOEntrenchmentPart>> CPartsList;
	CPartsList parts;

public:
	CMOEntrenchment() {}

	~CMOEntrenchment() override;

	void SetEntrenchmentStats( const NDb::SEntrenchmentRPGStats *pStats )
	{
		SetStats( pStats );
	}

	virtual void
	AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, NDb::ESeason eSeason )
	{
	}

	void AIUpdatePlacement( const struct SAINotifyPlacement &placement,
	                        struct IScene *pScene,
	                        struct ISoundScene *pSoundScene,
	                        NDb::ESeason eSeason ) override
	{
	}

	IClientUpdatableProcess *AIUpdateRPGStats( const struct SAINotifyRPGStats &stats,
	                                           struct IClientAckManager *pAckManager,
	                                           NDb::ESeason eSeason ) override
	{
		return 0;
	}

	bool Create( const int nUniqueID,
	             const SAIBasicUpdate *_pUpdate,
	             NDb::ESeason eSeason,
	             const NDb::EDayNight eDayTime,
	             bool bInEditor ) override;

	bool CreateSceneObject( const int nUniqueID,
	                        const SAINewUnitUpdate *pUpdate,
	                        NDb::ESeason eSeason,
	                        bool bInEditor ) override
	{
		return true;
	}

	//
	void AddPart( CMOEntrenchmentPart *pPart, const bool bLast = false, const bool bDigBySegment = false );
	//
	void GetStatus( SObjectStatus *pStatus ) const override;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const override;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const override;
	NDb::EUserAction
	GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const override;

	//
	bool IsSelected() const override
	{
		return false;
	}

	void Select( bool bSelect ) override
	{
		NI_ASSERT( !bSelect, "Selecting entrenchment!" );
	}

	bool CanSelect() const override
	{
		return false;
	}

	void SetCanSelect( bool _bCanSelect ) override
	{
		NI_ASSERT( !_bCanSelect, "Selecting entrenchment!" );
	}

	bool IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const override;

	virtual void GetPassangers( std::vector<IB2MapObj *> *pPassangers ) const {}

	virtual int GetPassangersCount() const
	{
		return 0;
	}

	//
	int operator&( IBinSaver &saver ) override;
	//
};
