#pragma once

#include "../system/time.h"
#include "UpdatableObject.h"

#include "../Misc/2Darray.h"
#include "../Stats_B2_M1/FeedBackUpdates.h"


class CEventUpdater
{
public:
	//basic structure for storing updates
	class CUpdateData : public CObjectBase
	{
		OBJECT_BASIC_METHODS( CUpdateData )


////Update transformers
	public:
		interface IUpdateTransformer : public CObjectBase
		{
			virtual SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime ) = 0;
		};
		class CIdleTrenchUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CIdleTrenchUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CParadropStartedTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CParadropStartedTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CChangeDBIDUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CChangeDBIDUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CPlacementUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CPlacementUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CRPGUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CRPGUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CHitUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CHitUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CHTurretTurnUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CHTurretTurnUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CVTurretTurnUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CVTurretTurnUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CEntranceUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CEntranceUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CModifyEntranceUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CModifyEntranceUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CDiplomacyUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CDiplomacyUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CShootAreaUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CShootAreaUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CRangeAreaUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CRangeAreaUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CNewProjectileUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CNewProjectileUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CDeadUnitUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CDeadUnitUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CDisappearUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CDisappearUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CNewUnitUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CNewUnitUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CNewEntrenchmentUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CNewEntrenchmentUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CNewFormationUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CNewFormationUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CRevealArtilleryUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CRevealArtilleryUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CStructCopierUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CStructCopierUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CObjectsUnderConstructionTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CObjectsUnderConstructionTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CKeyBuildingUpdateTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CKeyBuildingUpdateTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CScriptCameraRunTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CScriptCameraRunTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CScriptCameraResetTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CScriptCameraResetTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CScriptCameraStartMovieTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CScriptCameraStartMovieTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CScriptCameraStopMovieTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CScriptCameraStopMovieTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CWeatherChangedTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CWeatherChangedTransformer );
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CChangeVisibilityTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CChangeVisibilityTransformer )
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CPlayEffectTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CPlayEffectTransformer )
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CUpdateStatusTransformer : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CUpdateStatusTransformer )
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CUpdateSuperWeaponControlTransform : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CUpdateSuperWeaponControlTransform )
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
		class CUpdateSuperWeaponRecycleTransform : public IUpdateTransformer
		{
			OBJECT_BASIC_METHODS( CUpdateSuperWeaponRecycleTransform )
		public:
			SAIBasicUpdate* Transform( CUpdateData *pUpdate, int nReturnTime );
		};
	private:
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		static hash_map< int, CPtr<IUpdateTransformer> > clientTransformers;

	protected:
		CUpdateData() {}
	public:
		//update time
		NTimer::STime nUpdateTime;
		//incoming order
		int nOrder;
		//update data
		EActionNotify eUpdateType;
		CPtr<SAIBasicUpdate> pData;
		CObj<CUpdatableObj> pObj;
		int nParam;
		//for removing sent updates
		bool bValid;
		
		CUpdateData( const NTimer::STime &nTime, int nCounter, EActionNotify eType, CUpdatableObj *_pObj, int _nParam ) :
			nUpdateTime( nTime ),
			nOrder( nCounter ),
			eUpdateType( eType ),
			pObj( _pObj ),
			nParam( _nParam ), 
			bValid( true ),
			pData( 0 ) {}
		
		CUpdateData( const CUpdateData &data ) : 
			nUpdateTime( data.nUpdateTime ),
			nOrder( data.nOrder ),
			eUpdateType( data.eUpdateType ),
			pObj( data.pObj ),
			nParam( data.nParam ),
			bValid( data.bValid ),
			pData( data.pData ) {}
		
		static void Init();
		
		SAIBasicUpdate* GetClientStruct( int nReturnTime ); 
		
		int operator&( IBinSaver &saver );
	};
	struct SUpdateDataLessCompare
	{
		bool operator()( const CUpdateData *data1, const CUpdateData *data2 ) const
		{
			return data1->nUpdateTime < data2->nUpdateTime || ( data1->nUpdateTime == data2->nUpdateTime && data1->nOrder < data2->nOrder );
		}
	};
private:
	typedef list<CPtr<CUpdateData> > TUpdatesList;
	typedef CArray2D<TUpdatesList> TUpdatesByCells;

	TUpdatesByCells suspended;
	void InsertSuspendedUpdate( CUpdateData* pUpdate, const SVector &_vPosition );
	//ready-to-go updates
	typedef list< CPtr< CUpdateData > > CUpdateList;
	CUpdateList pendingUpdates;
	CUpdateList pendingSuspendableUpdates;
	CUpdateList interpolatableUpdates;
	CUpdateList::iterator pendingIt;
	//basic updates catalog
	typedef hash_map< CPtr<CUpdatableObj>, CUpdateList, SDefaultPtrHash > CUpdateMap;
	CUpdateMap updatesHash;
	//current segment time
	NTimer::STime nTime;
	//current game time
	NTimer::STime nReturnTime;
	//incoming update counter
	int nCounter;
	//player's party
	int nMyParty;
	//visible tiles registrator
	hash_set<SVector, STilesHash> visibleTiles;
	//legacy
	bool bShowAreas;
	EActionNotify eAreaType;
	list< CPtr<CUpdatableObj> > shootGroupUnits;

	hash_set<int> updatedPlacements;
	NTimer::STime lastTimeUpTo;

	void DestroyContents();	
	
	//get update from queue
	CUpdateData* PopUpdate();
	//updating suspended updates visibility
	void PumpUpdates();
	//add visible suspended updates to queue
	void CollectUpdates( NTimer::STime nUpTo = 0 );
	
	CUpdateData* CreateAnimationUpdate( CUpdatableObj *pObj, int nAnimation );
	bool IsInterpolatableEvent( EActionNotify eUpdateType ) const;
	bool IsOneCopyEvent( EActionNotify eUpdateType ) const;
public:
	CEventUpdater();
	~CEventUpdater() {}

	void ClearInterpolatable();
	void Init( const int nStaticMapSizeX, const int nStaticMapSizeY );
	void Clear() { DestroyContents(); }
	
	//send update
	void AddUpdate( EFeedBack eFeedBack, int nParam = -1, CObjectBase *pParam = 0 );
	void AddUpdate( SAIBasicUpdate *pUpdate, EActionNotify eUpdateType, CUpdatableObj *pObj, int nParam );
	//inform on visible tiles in WarFog's coordinates !!!!!! (not AI tiles but vis one)
	void TileBecameVisibleFromWarFog( const SVector &vPos, const int nParty );
	//inform on object deletion
	void ClearUpdates( CUpdatableObj *pObj, EActionNotify eUpdateType = ACTION_NOTIFY_NONE );
	//update current segment time
	void UpdateTime( const NTimer::STime &nNewTime );
	
	void UpdateAreasGroup( const bool bShow, EActionNotify eType );

	bool IsPlacementUpdated( CUpdatableObj *pObj ) const;
	void ClearPlacementUpdates();

	void PrepareUpdates();
	SAIBasicUpdate* GetUpdate();
	void RegisterShootAreaUnit( CUpdatableObj *pObj ) { shootGroupUnits.push_back( pObj ); }

	void DumpSizes();

	int operator&( IBinSaver &saver );
};

