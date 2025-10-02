#pragma once
#include "3Dmotor_export.h"

#include "aiInterval.h"
#include "aiVisitor.h"

namespace NDb
{
	struct SModel;
//	struct SRPGArmor;
}
struct IVisObj;
class CTransformStack;

namespace NAI
{
const int MUST_BE_IN_OBJECT = -100;

class CBSPTree;
class CExplVoxelRenderer;
class CVisionVoxelRenderer;
class CFastRenderer;
class IPrepareCollider;
class IStabilityTrackers;

class CFloorsSet
{
protected:
	vector<int> floors;
	friend class CAIMap;
public:
	CFloorsSet() {}
	CFloorsSet( int nFloor ) { floors.push_back( nFloor ); }
	CFloorsSet( int nFloor1, int nFloor2 ) { floors.push_back( nFloor1 ); if ( nFloor2 != nFloor1 ) floors.push_back( nFloor2 ); }
	CFloorsSet( const vector<int> &_floors ): floors(_floors) {}
};

struct SObjectInfo
{
	vector<CVec3> points;
	vector<STriangle> tris;
	int nPieceID;
	CDBPtr<NDb::CResource> pArmor;
	int nTSFlags;
};

interface IAIMapTracker: public CObjectBase
{
public:
	virtual void OnChange() = 0;
};

class CBSPTree;
interface IAIMap: public CObjectBase, public IAIVisitor
{
public:
	enum ESplitTerrainHGroups
	{
		STH_SPLIT_TERR_HG,
		STH_UNION_TERR_HG
	};
	enum ESort
	{
		STH_NOSORT,
		STH_SORT_INTERVALS,
		STH_SORT_AND_REDUCE_TERRAIN
	};
	enum ESyncType
	{
		ST_FAST,
		ST_NORMAL
	};
	struct SSelectedObject
	{
		CObjectBase *pUserData;
		int nUserID;
	};
	virtual void Sync( ESyncType st = ST_NORMAL ) = 0;
	virtual void GetEntities( list<SObjectInfo> *pRes, int nMask, const CFloorsSet &hg = CFloorsSet() ) = 0;
	virtual void Trace( const CRay &, vector<SInterval> *pIntersections, int nMask, const CFloorsSet &hg = CFloorsSet(), ESplitTerrainHGroups shg = STH_UNION_TERR_HG ) = 0;
	virtual void TraceGrid( CFastRenderer *pRes, int nMask, ESort sort, const CFloorsSet &fs = CFloorsSet(), ESplitTerrainHGroups shg = STH_UNION_TERR_HG ) = 0;
	// fRadiusKoef - (1 - touches frustrum, 0 - center inside, -1 - whole inside)
	virtual void Select( vector<SSelectedObject> *pRes, const CTransformStack &ts, float fRadiusKoef, int nMask, const CFloorsSet &hg = CFloorsSet() ) = 0;
	virtual bool GetUnitHLPos( CVec3 *pRes, CObjectBase *_pUserData, int nUserID ) = 0;
	virtual void GetAccessibleUnitHL( vector<int> *pRes, const CVec3 &ptFrom, CObjectBase *_pUserData, float fMaxDistance ) = 0;
	virtual bool CalcIntersection( const CVec3 &ptCenter, float fRadius, int s, CObjectBase *pIgnoreUser = 0 ) = 0;
	virtual void AddTracker( IAIMapTracker *pTracker, const SBound &b, int nMask, bool bInformOnDoorFlip = false ) = 0;
	virtual void SelectHullPointers( vector<CPtr<CObjectBase> > *pRes, const SBound &b, int nMaskOr, int nMaskNot ) = 0;
	//virtual void DebugInformTrackers( const SBound &b ) = 0;
};
void GetGeometry( list<SObjectInfo> *pRes, vector<SMassSphere> *pSpheres, const NDb::SAIGeometry * pAIGeom, bool *pbClosed = 0 );
void GetSpheres( const NDb::SModel *pModel, vector<SMassSphere> *pRes, CVec3 *pMassCenter );

_3DMOTOR_EXPORT IAIMap* CreateAIMap();

void FindClosePositionOnSurface( IAIMap *pMap, const CVec3 &ptPos, CVec3 *pRes, int nFlags );
} // namespace


