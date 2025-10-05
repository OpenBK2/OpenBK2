#pragma once

#include "MapObj.h"

class CMOEntrenchment;

class CMOEntrenchmentPart : public CMOSelectable
{
	OBJECT_NOCOPY_METHODS( CMOEntrenchmentPart );
	
	CPtr<CMOEntrenchment> pParent;
	// Cached transformation
	struct SCachedInfo 
	{
		//Indentifying parameters
		CVec3			vPos;
		WORD			wDir;
		//Derived parameters
		CQuat			qRot;
		SHMatrix	mPlace;
		//Terrain hole params
		CVec2			vHoleStart;
		CVec2			vHoleEnd;
		float			fHoleWidth;
	};

	SCachedInfo cached;
	int nFrameIndex;

	void DeriveTransform( const CVec3 &_vPos, const WORD _wDir );
public:
	CMOEntrenchmentPart() : pParent( 0 )  , nFrameIndex( 0 )
	{
		cached.vPos.Set( 0, 0, 0 );
		cached.wDir = 0;
		Identity( &cached.mPlace );
		cached.qRot = QNULL;
		cached.vHoleStart.Set( 0, 0 );
		cached.vHoleEnd.Set( 0, 0 );
		cached.fHoleWidth = 0;
	};
	
	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor );
	void GetStatus( SObjectStatus *pStatus ) const;
	virtual void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason );

	IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason ) { return 0; }
	//
	void SetParent( CMOEntrenchment *pTrench ) { pParent = pTrench; }
	CMOEntrenchment* GetParent() { return pParent; }
	//
	void GetHoleParams( CVec2 *pStart, CVec2 *pEnd, float *pWidth );
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	//
	virtual bool IsSelected() const { return false; }
	virtual void Select( bool bSelect ) { NI_ASSERT( !bSelect, "Selecting entrenchment!" ); }
	virtual bool CanSelect() const { return false; }
	virtual void SetCanSelect( bool _bCanSelect ) { NI_ASSERT( !_bCanSelect, "Selecting entrenchment!" ); }
	virtual void GetPassangers( std::vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }
	//
	int operator&( IBinSaver &saver );
};

class CMOEntrenchment : public CMOSelectable
{
	OBJECT_NOCOPY_METHODS( CMOEntrenchment );
	
	typedef std::list< CPtr<CMOEntrenchmentPart> > CPartsList;
	CPartsList parts;
public:
	CMOEntrenchment() {};
	~CMOEntrenchment();

	void SetEntrenchmentStats( const NDb::SEntrenchmentRPGStats *pStats ) { SetStats(pStats); }

	virtual void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, NDb::ESeason eSeason ) { }
	virtual void AIUpdatePlacement( const struct SAINotifyPlacement &placement, struct IScene *pScene, struct ISoundScene *pSoundScene, NDb::ESeason eSeason ) { }
	
	virtual IClientUpdatableProcess* AIUpdateRPGStats( const struct SAINotifyRPGStats &stats, struct IClientAckManager *pAckManager, NDb::ESeason eSeason ) { return 0; }

	virtual bool Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason, const NDb::EDayNight eDayTime, bool bInEditor );
	bool CreateSceneObject( const int nUniqueID, const SAINewUnitUpdate *pUpdate, NDb::ESeason eSeason, bool bInEditor ) { return true; }
	//
	void AddPart( CMOEntrenchmentPart *pPart, const bool bLast = false, const bool bDigBySegment = false );
	//
	void GetStatus( SObjectStatus *pStatus ) const;
	//
	void GetActions( CUserActions *pActions, EActionsType eActions ) const;
	void GetDisabledActions( CUserActions *pActions, EActionsType eActions ) const;
	NDb::EUserAction GetBestAutoAction( const CUserActions &actionsBy, CUserActions *pActionsWith, bool bAltMode ) const;
	//
	virtual bool IsSelected() const { return false; }
	virtual void Select( bool bSelect ) { NI_ASSERT( !bSelect, "Selecting entrenchment!" ); }
	virtual bool CanSelect() const { return false; }
	virtual void SetCanSelect( bool _bCanSelect ) { NI_ASSERT( !_bCanSelect, "Selecting entrenchment!" ); }

	bool IsPlaceMapCommandAck( NDb::EUserAction eUserAction ) const;
	virtual void GetPassangers( std::vector<IB2MapObj*> *pPassangers ) const { }
	virtual int GetPassangersCount() const { return 0; }
	//
	int operator&( IBinSaver &saver );
	//
};


