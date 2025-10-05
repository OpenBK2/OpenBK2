#pragma once

#include "Common_RTS_AI/Terrain.h"
#include "Misc/AreaMap.h"
#include <float.h>

class CExistingObject;
class CStaticObject;
class CBuilding;

namespace NDb
{
	struct SFenceRPGStats;
	struct SObjectBaseRPGStats; 
	struct SBuildingRPGStats;
	struct SStaticObjectRPGStats; 
	struct SEntrenchmentRPGStats;
	struct SBridgeRPGStats; 
	struct SMineRPGStats; 
	struct SMechUnitRPGStats; 
}
template<bool bOnlyContainers> class CStObjIter;
typedef std::list< CPtr<CBuilding> > CStoragesList;

//*******************************************************************
//*												  CStaticObjects													*
//*******************************************************************

struct IObstacle;
class CUpdatableObj;
class CBridgeSpan;
class CStaticObjects : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CStaticObjects );
	
	typedef CAreaMap<IObstacle, CPtr<IObstacle>, SVector, int > ObstacleAreaMap;
	typedef std::unordered_map< int, CPtr<IObstacle> > ObstacleObjectMap;
	typedef CAreaMap<CExistingObject, CObj<CExistingObject>, SVector, int> StaticObjectsAreaMap;
	struct SSegmentObjectsSort
	{
		bool operator()( const CPtr<CStaticObject> &segmObj1, const CPtr<CStaticObject> &segmObj2 ) const;
	};
public:
	// для перебора всех хранилищь
	struct IEnumStoragesPredicate
	{
		// перебирать только подсоединенные хранилища
		virtual bool OnlyConnected() const = 0;
		// true - закончить, то, что нужно уже нашлось
		// длина пути - в ТАЙЛАХ
		virtual bool AddStorage( class CBuilding * pStorage, const float fPathLenght ) = 0;
	};

	// для хранения информации о складах RU 
	class CStoragesContainer
	{
		typedef std::unordered_map< int, CObj<CBuilding> > CStorages;
		typedef std::list< CObj<CBuilding> > CStoragesList;

		ZDATA
		ZSKIP
		// пункты линии поддержки
		CStorages storages;									// for speed search storages
		public: ZEND int operator&( IBinSaver &f ) { f.Add(3,&storages); return 0; }
	public:
		CStoragesContainer();
		void UpdateForParty( const int nParty );
		//void Segment();
		void EnumStoragesForParty( const int nParty, IEnumStoragesPredicate * pPred );
		void Init() { Clear(); }
		void EnumStoragesInRange( const CVec2 &vCenter, 
			const int nParty, 
			const float fMaxPathLenght,
			const float fMaxOffset,
		class CCommonUnit * pUnitToFindPath, 
			IEnumStoragesPredicate * pPred );

		void AddStorage( class CBuilding *pNewStorage, const int nPlayer, int nLinkID );
		void RemoveStorage( class CBuilding * pNewStorage );
		void StorageChangedDiplomacy( class CBuilding *pNewStorage, const int nNewPlayer );
		void Clear();
	};

private:
	typedef std::unordered_map< int, CObj<CExistingObject> > CObjectsHashSet;

	ZDATA
	ObstacleAreaMap obstacles;
	ObstacleObjectMap obstacleObjects;

	StaticObjectsAreaMap areaMap;
	StaticObjectsAreaMap containersAreaMap;
	int nObjs;
	std::list<CPtr<CBridgeSpan> > bridges;
	std::list<CObj<CObjectBase> > entrenchments;

	// for iterators
	bool bIterCreated;
	CStoragesContainer storagesContainer;

	CObjectsHashSet terraObjs;
	CObjectsHashSet deletedObjects;

	std::unordered_set<int> burningObjects;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&obstacles); f.Add(3,&obstacleObjects); f.Add(4,&areaMap); f.Add(5,&containersAreaMap); f.Add(6,&nObjs); f.Add(7,&bridges); f.Add(8,&entrenchments); f.Add(9,&bIterCreated); f.Add(10,&storagesContainer); f.Add(11,&terraObjs); f.Add(12,&deletedObjects); f.Add(13,&burningObjects); return 0; }

	//
	void AddToAreaMap( CExistingObject *pObj );
	void AddObjectToAreaMapTile( CExistingObject *pObj, const SVector &tile );
	void RemoveFromAreaMap( CExistingObject *pObj );
	void RemoveObjectFromAreaMapTile( CExistingObject *pObj, const SVector &tile );

	StaticObjectsAreaMap& GetAreaMap() { return areaMap; }
	StaticObjectsAreaMap& GetContainersAreaMap() { return containersAreaMap; }
	void SetIterCreated( bool _bCreated ) { bIterCreated = _bCreated; }
	bool IsIterCreated() const { return bIterCreated; }

public:
	CStaticObjects()
		: areaMap( SConsts::STATIC_OBJ_CELL ), containersAreaMap( SConsts::STATIC_CONTAINER_OBJ_CELL ),
			obstacles( SConsts::STATIC_OBJ_CELL ), bIterCreated( false ) { }
	void Init( const int nMapTileSizeX, const int nMapTileSizeY );
	
	void PostAllObjectsInit();

	void AddObstacle( struct IObstacle *pObstacle );
	void RemoveObstacle( struct IObstacle *pObstacle );
	
	class CStaticObject* AddNewFenceObject( const SFenceRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nDiplomacy, const int nFrameIndex );
	void AddStaticObject( class CExistingObject* pObj, bool bAlreadyLocked );
	class CStaticObject* AddNewStaticObject( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex );
	class CStaticObject* AddNewTerraObj( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex );
	class CStaticObject* AddNewTerraMeshObj( const SObjectBaseRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex );
	class CStaticObject* AddNewBuilding( const SBuildingRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD wDir, const int nFrameIndex, int nPlayer, int nLinkID );
	void AddStorage( class CBuilding *pObj );
	class CStaticObject* AddNewEntrencmentPart( const SEntrenchmentRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD dir, const int nFrameIndex, int nPlayer, bool bPlayerCreates );
	void AddEntrencmentPart(  class CEntrenchmentPart *pObj, bool bLockedAlready );
	class CStaticObject* AddNewEntrencment( CObjectBase** segments, const int nLen, class CFullEntrenchment *pFullEntrenchment, const bool bPiecewise = false );
	class CStaticObject* AddNewBridgeSpan( const SBridgeRPGStats *pStats, const float fHPFactor, const CVec3 &center, const WORD ir, const int nFrameIndex );
	class CStaticObject* AddNewSmokeScreen( const CVec3 &vCenter, const float fR, const int nTransparency, const int nTime );

	class CMineStaticObject* AddNewMine( const SMineRPGStats *pStats, const float fHPFactor, const CVec3 &center, const int nFrameIndex, const int player );
	class CExistingObject* AddNewTankPit( const SMechUnitRPGStats *pStats, const CVec3 &center, const WORD dir, const int nFrameIndex, const class CVec2 &vHalfSize, const std::list<SObjTileInfo> &tilesToLock, class CAIUnit *pOwner );
	void GetNewStaticObjects( struct SNewUnitInfo **pObjects, int *pnLen );
	void GetDeletedStaticObjects( CObjectBase ***pObjects, int *pnLen );

	void RegisterSegment( class CStaticObject *pObj );
	void UnregisterSegment( class CStaticObject *pObj );

	void Segment();

	// вызываются только самими удаляемыми объектами
	void DeleteInternalObjectInfo( class CExistingObject *pObj );
	void DeleteInternalEntrenchmentInfo( class CEntrenchment *pEntrench );
	
	void StartBurning( class CExistingObject *pObj );
	void EndBurning( class CExistingObject *pObj );

	void StorageChangedDiplomacy( class CBuilding *pNewStorage, const int nNewPlayer );
	
	void UpdateAllObjectsPos();

	void EnumObstaclesInRange( const CVec2 &vCenter, const float fRadius, struct IObstacleEnumerator *f );
	void EnumStoragesForParty( const int nParty, struct IEnumStoragesPredicate *pPred );
	void EnumStoragesInRange( const CVec2 &vCenter, const int nParty, const float fMaxPathLength, const float fMaxOffset,
														class CCommonUnit *pUnitToFindPath, struct IEnumStoragesPredicate *pPred );

	void DeleteObjWOUpdates( class CExistingObject *pObj );

	friend class CStObjIter<false>;
	friend class CStObjIter<true>;
};


