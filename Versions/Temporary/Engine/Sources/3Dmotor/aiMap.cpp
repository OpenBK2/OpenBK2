#include "stdafx.h"
#include "aiMap.h"
#include "System/BasicShare.h"
#include "aiTrace.h"
#include "aiRender.h"
#include "OcTree.h"
#include "aiObjectLoader.h"
#include "GBind.h"
#include "DBScene.h"
#include "3DLib/Transform.h"
#include "GMesh.h"
#include "SuperCollider.h"
#include "3DLib/Bound.h"
#include "3DLib/MemObject.h"

#include <boost/config.hpp>

const int N_MIN_FLOOR = -3;

//static NGScene::CResourceTracker aiGeometryCheckers( "AIGeometries" );


namespace NAI
{
CBasicShare<CDBPtr<NDb::SAIGeometry>, CLoadAIGeometryFromGranny, SDBPtrHash> shareAIModel(107); // используется также в MakeBuilding для определения какие куски присутсвуют в геометрии
CBasicShare<CDBPtr<NDb::SAIGeometry>, CFileSkinPointsLoadFromGranny, SDBPtrHash> shareSkinPoints(108);
//CBasicShare<int, CLoadTwoBSPTrees> shareBSPTrees(117);

class CConvexHull;
class CUserHullsTracker : public CObjectBase
{
	OBJECT_NOCOPY_METHODS(CUserHullsTracker);
	typedef std::unordered_map<CPtr<CObjectBase>, std::vector<CPtr<CConvexHull> >> SUserHash;
	ZDATA
	SUserHash data;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&data); return 0; }
public:
	void AddHull( CObjectBase *pUser, CConvexHull *pHull ) { data[pUser].push_back( pHull ); }
	void RemoveHull( CObjectBase *pUser, CConvexHull *pHull ) 
	{
		SUserHash::iterator i = data.find( pUser );
		ASSERT( i != data.end() );
		if ( i == data.end() )
			return;
		std::vector<CPtr<CConvexHull> >::iterator k = find( i->second.begin(), i->second.end(), pHull );
		ASSERT( k != i->second.end() );
		if ( k != i->second.end() )
			i->second.erase( k );
		if ( i->second.empty() )
			data.erase( i );
	}
};

class CVolumeNode;
class CConvexHull: public CObjectBase
{
public:
	struct SMap
	{
		int nPieceID, nUserID;
		SMap() {}
		SMap( int _nPieceID, int _nUserID ): nPieceID(_nPieceID), nUserID(_nUserID) {}
	};
	ZDATA
	CPtr<CVolumeNode> pNode;
	CDGPtr<CPtrFuncBase<CGeometryInfo> > pGeometry;
	SHMatrix pos;
	std::vector<SMap> pieces;
	SSourceInfo src;
	int nIndexInNode;
	CPtr<CUserHullsTracker> pUserHulls;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pNode); f.Add(3,&pGeometry); f.Add(4,&pos); f.Add(5,&pieces); f.Add(6,&src); f.Add(7,&nIndexInNode); f.Add(8,&pUserHulls); return 0; }
	//
	CConvexHull() {}
	CConvexHull( CUserHullsTracker *_pUserHulls, CPtrFuncBase<CGeometryInfo> *_pGeometry, const SHMatrix &_pos,
		const NDb::CResource *_pArmor, CObjectBase *_pSrc, int _nMask, int _nFloor ) 
		: pGeometry(_pGeometry), pos(_pos), src( _pSrc, _pArmor, _nFloor, _nMask ), pUserHulls(_pUserHulls)
	{
		if ( IsValid( pUserHulls ) )
			pUserHulls->AddHull( _pSrc, this );
	}
	~CConvexHull();
	bool SetNode( CVolumeNode *_p, const SBound &_bound );
	const SBound& GetLinkedBound();
	void SetLinkedBound( const SBound &b );
	void AssignUserID( int nUserID );
	void EstimateBound( SBound *pRes );
};

class CStaticConvexHull: public CConvexHull
{
	OBJECT_NOCOPY_METHODS(CStaticConvexHull);
public:
	CStaticConvexHull() {}
	CStaticConvexHull( CUserHullsTracker *_pUserHulls, CPtrFuncBase<CGeometryInfo> *_pGeometry, const SHMatrix &_pos,
		const NDb::CResource *_pArmor, CObjectBase *_pSrc, int _nMask, int _nFloor ) 
		: CConvexHull( _pUserHulls, _pGeometry, _pos, _pArmor, _pSrc, _nMask, _nFloor )
	{
	}
//	~CStaticConvexHull();
};

class CDynamicConvexHull: public CConvexHull
{
	OBJECT_NOCOPY_METHODS(CDynamicConvexHull);
public:
	ZDATA_(CConvexHull)
	CDGPtr<CFuncBase<SBound> > pBound;
	CDGPtr<CFuncBase<NAnimation::SGrannySkeletonPose> > pAnimationTracker;
	//SBound linkedBound;
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CConvexHull*)this); f.Add(2,&pBound); f.Add(3,&pAnimationTracker); return 0; }
	//
	CDynamicConvexHull() {}
	CDynamicConvexHull( CUserHullsTracker *_pUserHulls, CPtrFuncBase<CGeometryInfo> *_pGeometry, const SHMatrix &_pos,
		const NDb::CResource *_pArmor, CObjectBase *_pSrc, int _nMask, int _nFloor,
		CFuncBase<SBound> *_pBound, CFuncBase<NAnimation::SGrannySkeletonPose> *_pAnimation ) 
		: CConvexHull( _pUserHulls, _pGeometry, _pos, _pArmor, _pSrc, _nMask, _nFloor ),
		pBound( _pBound), pAnimationTracker(_pAnimation)
	{
	}
//	~CDynamicConvexHull();
};

static void Convert( SObjectInfo *pRes, const SConvexHull &h )
{
	pRes->points.resize( h.points.size() );
	for ( int i = 0; i < h.points.size(); ++i )
		h.trans.RotateHVector( &pRes->points[i], h.points[i] );
	h.tris.BuildTriangleList( &pRes->tris );
	pRes->nPieceID = h.nUserID;
	pRes->pArmor = h.src.pArmor;
	pRes->nTSFlags = h.src.nTSFlags;
}


static void Convert( std::vector<SObjectInfo> *pRes, const SHullSet &h )
{
	SObjectInfo r;
	for ( std::vector<SConvexHull>::const_iterator i = h.objects.begin(); i != h.objects.end(); ++i )
	{
		Convert( &r, *i );
		pRes->push_back( r );
	}
	for ( std::vector<SConvexHull>::const_iterator i = h.terrain.begin(); i != h.terrain.end(); ++i )
	{
		Convert( &r, *i );
		pRes->push_back( r );
	}
}

template<class TTest>
static void SelectPieces( SHullSet *pRes, const TTest &f, CConvexHull *pHull, const SBound &b )
{
	if ( f( b.s.ptCenter, b.s.fRadius, true ) )
	{
		const SHMatrix &trans = pHull->pos;
		pHull->pGeometry.Refresh();
		CGeometryInfo *pGeom = pHull->pGeometry->GetValue();
		if ( pHull->pieces.empty() )
		{
			for ( CGeometryInfo::CPieceMap::const_iterator i = pGeom->pieces.begin(); i != pGeom->pieces.end(); ++i )
			{
				SConvexHull ch( i->second.points, i->second.edges, trans, pHull->src, i->first );//, i->second.precalc );
				if ( pHull->src.pUserData )
					pRes->objects.push_back( ch );
				else
					pRes->terrain.push_back( ch );
			}
		}
		else
		{
			for ( int i = 0; i < pHull->pieces.size(); ++i )
			{
				const CConvexHull::SMap &m = pHull->pieces[i];
				CGeometryInfo::CPieceMap::const_iterator k = pGeom->pieces.find( m.nPieceID );
				if ( k != pGeom->pieces.end() )
				{
					SConvexHull ch( k->second.points, k->second.edges, trans, pHull->src, m.nUserID );//, k->second.precalc );
					if ( pHull->src.pUserData )
						pRes->objects.push_back( ch );
					else
						pRes->terrain.push_back( ch );
				}
				else
					ASSERT( 0 && "specified piece not found in geometry" );
			}
		}
	}
}

struct SAlwaysTrue
{
	BOOST_FORCEINLINE bool operator()( const CVec3 &p, float f, bool b ) const { return true; }
};

void GetGeometry( std::vector<SObjectInfo> *pRes, std::vector<SMassSphere> *pSpheres, const NDb::SAIGeometry * pAIGeom, bool *pbClosed )
{
	if ( !pAIGeom )
		return;
	CDGPtr<CPtrFuncBase<CGeometryInfo> > pGeom = shareAIModel.Get( pAIGeom );
	SHMatrix trans;
	Identity( &trans );
	pGeom.Refresh();
	CGeometryInfo *pGInfo = pGeom->GetValue();
	if ( !pGInfo )
		return;
	*pSpheres = pGInfo->spheres;
	CObj<CConvexHull> pHull = new CStaticConvexHull( 0, pGeom, trans, 0, 0, 0, 0 );
	if ( pGInfo->pieces.size() > 6 )
		pHull->pieces.push_back( CConvexHull::SMap( 0, 0 ) );
	SHullSet res;
	SelectPieces( &res, SAlwaysTrue(), pHull, SBound() );
	Convert( pRes, res );
	if ( pbClosed )
	{
		*pbClosed = true;
		for ( CGeometryInfo::CPieceMap::iterator i = pGInfo->pieces.begin(); i != pGInfo->pieces.end(); ++i )
			*pbClosed = pbClosed && i->second.edges.IsClosed();
	}
}

void GetSpheres( const NDb::SModel *pModel, std::vector<SMassSphere> *pRes, CVec3 *pMassCenter )
{
	if ( !pModel || !pModel->pGeometry )
		return;
	const NDb::SAIGeometry *pAIGeom = pModel->pGeometry->pAIGeometry;
	if ( !pAIGeom )
		return;
	if ( pModel->pSkeleton )
	{
		CDGPtr< CPtrFuncBase<CFileSkinPoints> > pGeom = shareSkinPoints.Get( pAIGeom );
		pGeom.Refresh();
		CFileSkinPoints *pInfo = pGeom->GetValue();
		for ( int i = 0; i < pInfo->spheres.size(); ++i )
			pRes->push_back( pInfo->spheres[i] );
		*pMassCenter = pInfo->massCenter;
	}
	else
	{
		CDGPtr< CPtrFuncBase<CGeometryInfo> > pGeom = shareAIModel.Get( pAIGeom );
		pGeom.Refresh();
		CGeometryInfo *pInfo = pGeom->GetValue();
		for ( int i = 0; i < pInfo->spheres.size(); ++i )
			pRes->push_back( pInfo->spheres[i] );
		*pMassCenter = pInfo->massCenter;
	}
}

// CHGSLayer

struct SFloorsSelector
{
	std::vector<char> take;

	bool IsTaken( int _nFloor ) const 
	{ 
		unsigned int n = _nFloor - N_MIN_FLOOR;
		if ( n >= take.size() ) 
			return true;
		return take[n] != 0; 
	}
};


//! node size
const int N_MINIMAL_OCTREE_NODE = 4; 
class CVolumeNode : public COcTreeNode<CVolumeNode, N_MINIMAL_OCTREE_NODE>
{
	OBJECT_BASIC_METHODS( CVolumeNode );
	typedef COcTreeNode<CVolumeNode, N_MINIMAL_OCTREE_NODE> CParent;
	void InformLowerTrackers( const SBound &b, int nMask, bool bDoorFlipped );
	void InformCurrentTrackers( const SBound &b, int nMask, bool bDoorFlipped );
public:
	struct STrackerDescr
	{
		ZDATA
		CPtr<IAIMapTracker> pTracker;
		SBound bound;
		int nMask;
		bool bInformOnDoorFlip;
		ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pTracker); f.Add(3,&bound); f.Add(4,&nMask); f.Add(5,&bInformOnDoorFlip); return 0; }
		STrackerDescr() {}
		STrackerDescr( IAIMapTracker *_p, const SBound &_b, int _nMask, bool _bInform ): 
			pTracker(_p), bound(_b), nMask(_nMask), bInformOnDoorFlip( _bInform ) {}
	};
	struct SElementInfo
	{
		ZDATA
		CPtr<CConvexHull> pHull;
		int nFlags, nFloor;
		ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pHull); f.Add(3,&nFlags); f.Add(4,&nFloor); return 0; }
	};
	typedef std::vector<SElementInfo> CElemList;
	ZDATA_(CParent)
	//CElemList hulls;
	std::vector<SElementInfo> hulls;
	std::vector<SBound> hullBounds;
	std::vector<STrackerDescr> trackers;
	int nFree;
	SBound bInform;
	int nInformMask;
	ZEND int operator&( CStructureSaver &f ) { f.Add(1,(CParent*)this); f.Add(2,&hulls); f.Add(3,&hullBounds); f.Add(4,&trackers); f.Add(5,&nFree); f.Add(6,&bInform); f.Add(7,&nInformMask); return 0; }

	CVolumeNode() : nFree(-1), nInformMask(0) {}
	int AddHull( CConvexHull *pHull, const SBound &bound );
	void RemoveHull( int nIndex );//CConvexHull *pHull );
	void SetLinkedBound( int nIndex, const SBound &b );
	void InformTrackers( const SBound &b, int nMask, bool bDoorFlipped = false );
	void AddInform( const SBound &b, int nMask, bool bTraverseUp = true );
	void CallCachedInforms();
	void AddTracker( IAIMapTracker *_pTracker, const SBound &_bound, int nMask, bool bInformOnDoorFlip );
	virtual bool IsEmpty();
};

inline static bool IsValidInCurrentState( int nFlags )
{
	return true;//( ( nFlags & ( NWorld::TS_STATE_OPEN | NWorld::TS_STATE_CLOSED ) ) == 0 ) ||	( nFlags & NWorld::TS_DOOR_HULL_VALID );
}

class CAIMap: public IAIMap
{
	OBJECT_NOCOPY_METHODS(CAIMap);
	//typedef hash_map<int, CObj<CVolumeNode> > CVolumeNodesHash;
	//typedef COrdinarySyncDst<IVisObj,CAIMap> TParent;
	//
	ZDATA
	CObj<CVolumeNode> pRoot;
	std::vector<CPtr<CDynamicConvexHull> > dynamicHulls;
	int nAllTrackersMask; // for fast checks
	int nMaxFloor;
	//CPtr<IStabilityTrackers> pStability;
	CObj<CUserHullsTracker> pUserHullsTracker;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pRoot); f.Add(3,&dynamicHulls); f.Add(4,&nAllTrackersMask); f.Add(5,&nMaxFloor); f.Add(6,&pUserHullsTracker); return 0; }
	//
	CVolumeNode* GetNode( const CVec3 &ptCenter, float fRadius );
	CVolumeNode* GetNode( CConvexHull *pHull, SBound *pBound );
	void InsertHull( CConvexHull *pHull );
	void RegisterFloor( int nFloor );
	bool InitFloorsSelector( SFloorsSelector *pRes, const CFloorsSet &fs );
	void CallInform( CConvexHull *pHull, const SBound &b, bool bDoorFlipped = false );
	//
	// adding new hull 
	CObjectBase *AddHull( const NDb::SAIGeometry *pAIGeom, 
		const SHMatrix &pos, 
		const NDb::CResource *pArmor, int nFloor, int nMask );
	CObjectBase *AddHull( CMemObject *pAIGeom, 
		const SHMatrix &pos, 
		const NDb::CResource *pArmor, int nFloor, int nMask );
	CObjectBase *AddAnimatedHull( const NDb::SAIGeometry *pAIGeom, const NAnimation::SGrannySkeletonHandle &skeletonH, 
		CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, 
		const NDb::CResource *pArmor, int nFloor, int nMask );
	void AssignUserID( CObjectBase *p, int _nUserID );
	//
	template<class TTest>
		void SelectHulls( SHullSet *pRes, const TTest &f, CVolumeNode *pNode, const SFloorsSelector &fSelect, 
			int nMask, bool bSelect2DoorHulls = false )
		{
			if ( pNode == 0 )
				return;
			SSphere t;
			pNode->GetBound( &t );
			if ( !f( t.ptCenter, t.fRadius, false ) )
				return;
			for ( CVolumeNode::CElemList::iterator i = pNode->hulls.begin(); i != pNode->hulls.end(); ++i )
			{
				CConvexHull *pHull = i->pHull;
				if ( pHull )
				{
					ASSERT( IsValid(pHull) );
					int nFlags = i->nFlags;
					if ( ( nFlags & nMask ) != 0 && fSelect.IsTaken( i->nFloor ) )
					{
						if ( bSelect2DoorHulls || IsValidInCurrentState( nFlags ) )
						{
							SelectPieces( pRes, f, pHull, pNode->hullBounds[ i - pNode->hulls.begin() ] );
						}
					}
				}
			}
			for ( int i = 0; i < 8; ++i )
				SelectHulls( pRes, f, pNode->GetNode(i), fSelect, nMask, bSelect2DoorHulls );
		}
	template<class TTest>
		void SelectFloorSet( SHullSet *pRes, const CFloorsSet &fs, int nMask, const TTest &f, bool bSelect2DoorHulls = false )
		{
			SFloorsSelector fSelect;
			if ( !InitFloorsSelector( &fSelect, fs ) )
				return;
			SelectHulls( pRes, f, pRoot, fSelect, nMask, bSelect2DoorHulls );
		}
	void FlipDoorWindow( CObjectBase *pWhat, bool bOpen, CVolumeNode *pNode, std::vector<CConvexHull*> *pCallInformForThem );
	void SelectHullPointers( std::vector<CPtr<CObjectBase> > *pRes, const SBound &b, int nMaskOr, int nMaskNot, CVolumeNode *pNode );
public:
	CAIMap(): nMaxFloor(0), nAllTrackersMask(0) {}
	CAIMap( int );
	virtual void Sync( ESyncType st );
	virtual void GetEntities( std::vector<SObjectInfo> *pRes, int nMask, const CFloorsSet &fs );
	virtual void Trace( const CRay &, std::vector<SInterval> *pIntersections, int nMask, const CFloorsSet &fs, ESplitTerrainHGroups shg );
	virtual void TraceGrid( CFastRenderer *pRes, int nMask, ESort sort, const CFloorsSet &fs = CFloorsSet(), ESplitTerrainHGroups shg = STH_UNION_TERR_HG );
	virtual void Select( std::vector<SSelectedObject> *pRes, const CTransformStack &ts, float fRadiusKoef, int nMask, const CFloorsSet &hg );
	virtual bool CalcIntersection( const CVec3 &ptCenter, float fRadius, int s, CObjectBase *pIgnoreUser );
	virtual void AddTracker( IAIMapTracker *pTracker, const SBound &b, int nMask, bool bInformOnDoorFlip = false );
	virtual void SelectHullPointers( std::vector<CPtr<CObjectBase> > *pRes, const SBound &b, int nMaskOr, int nMaskNot );
};

// CConvexHull

void CConvexHull::AssignUserID( int nUserID )
{
	pGeometry.Refresh();
	CGeometryInfo &g = *pGeometry->GetValue();
	for ( CGeometryInfo::CPieceMap::const_iterator i = g.pieces.begin(); i != g.pieces.end(); ++i )
		pieces.push_back( SMap( i->first, nUserID ) );
}

void CConvexHull::EstimateBound( SBound *pRes )
{
	CVec3 ptCenter;
	pGeometry.Refresh();
	const CGeometryInfo &g = *pGeometry->GetValue();
	float fRadius = sqrt( CalcRadius2( g.bound, pos ) );
	pos.RotateHVector( &ptCenter, g.bound.s.ptCenter );
	pRes->SphereInit( ptCenter, fRadius );
}

bool CConvexHull::SetNode( CVolumeNode *_p, const SBound &_bound )
{
	if ( pNode == _p )
		return false;
	if ( IsValid(pNode) )
		pNode->RemoveHull( nIndexInNode );
	pNode = _p;
	nIndexInNode = pNode->AddHull( this, _bound );
	return true;
}

const SBound& CConvexHull::GetLinkedBound() 
{ 
	ASSERT( pNode ); 
	return pNode->hullBounds[nIndexInNode]; 
}

void CConvexHull::SetLinkedBound( const SBound &b )
{
	pNode->SetLinkedBound( nIndexInNode, b );
}

CConvexHull::~CConvexHull()
{
	if ( IsValid( pUserHulls ) )
		pUserHulls->RemoveHull( src.pUserData, this );
	if ( !IsValid(pNode) )
		return;
	pNode->RemoveHull( nIndexInNode );
}

// CVolumeNode

bool CVolumeNode::IsEmpty()
{
	bool bEmpty = true;
	for ( int k = 0; k < hulls.size(); ++k )
	{
		if ( hulls[k].pHull )
		{
			ASSERT( IsValid( hulls[k].pHull ) );
			bEmpty = false;
			break;
		}
	}
	/*for ( CElemList::iterator i = hulls.begin(); i != hulls.end(); )
	{
		if ( !IsValid( *i ) )
			i = hulls.erase( i );
		else
			++i;
	}*/
	for ( std::vector<STrackerDescr>::iterator i = trackers.begin(); i != trackers.end(); )
	{
		if ( !IsValid( i->pTracker ) )
			i = trackers.erase( i );
		else
		{
			++i;
			bEmpty = false;
		}
	}
	return bEmpty;
}

int CVolumeNode::AddHull( CConvexHull *pHull, const SBound &bound )
{
	// find place
	int nPlace;
	if ( nFree < 0 )
	{
		hulls.resize( hulls.size() + 1 );
		hullBounds.resize( hullBounds.size() + 1 );
		nPlace = hulls.size() - 1;
	}
	else
	{
		nPlace = nFree;
		nFree = hulls[nFree].nFlags;
	}
	// store data
	SElementInfo &info = hulls[nPlace];
	info.pHull = pHull;
	info.nFlags = pHull->src.nTSFlags;
	info.nFloor = pHull->src.nFloor;
	hullBounds[nPlace] = bound;
	AddInform( bound, info.nFlags );
	//InformTrackers( bound, info.nFlags );
	return nPlace;
}

void CVolumeNode::RemoveHull( int nIndex )
{
	AddInform( hullBounds[nIndex], hulls[nIndex].nFlags );

	//InformTrackers( hullBounds[nIndex], hulls[nIndex].nFlags );
	hulls[nIndex].pHull = 0;
	hulls[nIndex].nFlags = nFree;
	nFree = nIndex;
}

void CVolumeNode::SetLinkedBound( int nIndex, const SBound &b )
{
	hullBounds[nIndex] = b;
	AddInform( hullBounds[nIndex], hulls[nIndex].nFlags );
	//InformTrackers( hullBounds[nIndex], hulls[nIndex].nFlags );
}

void CVolumeNode::AddInform( const SBound &b, int nMask, bool bTraverseUp )
{
	if ( bTraverseUp )
	{
		CVolumeNode *pUp = this;
		while ( pUp->GetUpLink() )
			pUp = pUp->GetUpLink();
		pUp->AddInform( b, nMask, false );
		return;
	}

	SSphere sTest;
	GetBound( &sTest );
	SBound bInternal;
	bInternal.SphereInit( sTest.ptCenter, sTest.fRadius );

	if ( !DoesIntersect( b, bInternal ) )
		return;

	if ( nInformMask == 0 )
	{
		nInformMask = nMask;
		bInform = b;
	}
	else
	{
		if ( ( nMask & nInformMask ) == nMask )
		{
			CVec3 vDif = bInform.s.ptCenter - b.s.ptCenter;
			if ( 
				fabs( vDif.x ) <= bInform.ptHalfBox.x - b.ptHalfBox.x && 
				fabs( vDif.y ) <= bInform.ptHalfBox.y - b.ptHalfBox.y && 
				fabs( vDif.z ) <= bInform.ptHalfBox.z - b.ptHalfBox.z )
				return;
		}
		CVec3 ptMin( bInform.s.ptCenter - bInform.ptHalfBox );
		CVec3 ptMax( bInform.s.ptCenter + bInform.ptHalfBox );
		nInformMask |= nMask;
		ASSERT( b.ptHalfBox.x >= 0 );
		ASSERT( b.ptHalfBox.y >= 0 );
		ASSERT( b.ptHalfBox.z >= 0 );
		ptMin.Minimize( b.s.ptCenter - b.ptHalfBox );
		ptMax.Maximize( b.s.ptCenter + b.ptHalfBox );
		bInform.BoxInit( ptMin, ptMax );
	}
	ASSERT( bInform.ptHalfBox.x >= 0 );
	ASSERT( bInform.ptHalfBox.y >= 0 );
	ASSERT( bInform.ptHalfBox.z >= 0 );

	for ( int k = 0; k < 8; ++k )
	{
		if ( CVolumeNode *pD = GetNode(k) )
			pD->AddInform( b, nMask, false );
	}
}

void CVolumeNode::CallCachedInforms()
{
	if ( nInformMask == 0 )
		return;
	
	for ( std::vector<STrackerDescr>::iterator i = trackers.begin(); i != trackers.end(); )
	{
		if ( (nInformMask & i->nMask) != 0 )
		{
			if ( !IsValid( i->pTracker ) )
				i = trackers.erase( i );
			else
			{
				ASSERT( bInform.ptHalfBox.x >= 0 );
				ASSERT( bInform.ptHalfBox.y >= 0 );
				ASSERT( bInform.ptHalfBox.z >= 0 );
				if ( DoesIntersect( i->bound, bInform ) )
					i->pTracker->OnChange();
				++i;
			}
		}
		else
			++i;
	}

	nInformMask = 0;
	for ( int k = 0; k < 8; ++k )
	{
		if ( CVolumeNode *pD = GetNode(k) )
			pD->CallCachedInforms();
	}
}

void CVolumeNode::InformTrackers( const SBound &b, int nMask, bool bDoorFlipped )
{
	SSphere sTest;
	GetBound( &sTest );
	SBound bInternal;
	bInternal.SphereInit( sTest.ptCenter, sTest.fRadius );
	if ( !DoesIntersect( b, bInternal ) )
		return;

	for ( std::vector<STrackerDescr>::iterator i = trackers.begin(); i != trackers.end(); )
	{
		if ( (nMask & i->nMask) != 0 )
		{
			if ( !IsValid( i->pTracker ) )
				i = trackers.erase( i );
			else
			{
				if ( DoesIntersect( i->bound, b ) && ( (!bDoorFlipped) || i->bInformOnDoorFlip ) )
					i->pTracker->OnChange();
				++i;
			}
		}
		else
			++i;
	}

	for ( int k = 0; k < 8; ++k )
		if ( GetNode(k) ) 
			GetNode(k)->InformTrackers( b, nMask, bDoorFlipped );
}

void CVolumeNode::AddTracker( IAIMapTracker *_pTracker, const SBound &_bound, int nMask, bool bInformOnDoorFlip )
{
	trackers.push_back( STrackerDescr( _pTracker, _bound, nMask, bInformOnDoorFlip ) );
}

// CAIMap

CAIMap::CAIMap( int )
: nMaxFloor(0), nAllTrackersMask(0)
{
	//pStability = CreateStabilityTrackers( _pWorld );
	pUserHullsTracker = new CUserHullsTracker;
}

CVolumeNode* CAIMap::GetNode( const CVec3 &ptCenter, float fRadius )
{
	if ( !pRoot )
	{
		pRoot = new CVolumeNode;
		pRoot->SetSize( CVec3( -128, -128, -128 ), 1024 );
	}
	return pRoot->GetNode( ptCenter, fRadius );
}

/*void CAIMap::DebugInformTrackers( const SBound &b )
{
	CVolumeNode *pNode = GetNode( b.s.ptCenter, b.s.fRadius );
	pNode->AddInform( b, NWorld::TS_ITEM_BLOCKER );
	pRoot->CallCachedInforms();
}*/

CVolumeNode* CAIMap::GetNode( CConvexHull *pHull, SBound *pBound )
{
	pHull->EstimateBound( pBound );
	return GetNode( pBound->s.ptCenter, pBound->s.fRadius );
}

void CAIMap::InsertHull( CConvexHull *pHull )
{
	if ( !IsValid( pHull ) )
		return;
	SBound bTest;
	CVolumeNode *pNode = GetNode( pHull, &bTest );
	pHull->SetNode( pNode, bTest );
//	GetStabilityTrackers()->EnlargeMap( bTest.s.ptCenter - bTest.ptHalfBox, bTest.s.ptCenter + bTest.ptHalfBox );
}

void CAIMap::RegisterFloor( int nFloor )
{
	nMaxFloor = (std::max)( nFloor, nMaxFloor );
}

bool CAIMap::InitFloorsSelector( SFloorsSelector *pRes, const CFloorsSet &fs )
{
	if ( !fs.floors.empty() )
	{
		bool bTaken = false;
		pRes->take.resize( nMaxFloor - N_MIN_FLOOR + 1, 0 );
		for ( int k = 0; k < fs.floors.size(); ++k )
		{
			int nFloor = fs.floors[k];
			if ( nFloor <= nMaxFloor )
			{
				pRes->take[ nFloor - N_MIN_FLOOR ] = 1;
				bTaken = true;
			}
		}
		if ( !bTaken )
			return false;
	}
	return true;
}

CObjectBase *CAIMap::AddHull( const NDb::SAIGeometry *pAIGeom, 
	const SHMatrix &pos, 
	const NDb::CResource *pArmor, int nFloor, int nMask )
{
	if ( !pAIGeom )
		return 0;
	if ( !NGScene::CResourceFileOpener::DoesExist( "AIGeometries", GetIntResKey( pAIGeom ) ) )
		return 0;
	RegisterFloor( nFloor );
	CConvexHull *pRes = new CStaticConvexHull( pUserHullsTracker, shareAIModel.Get( pAIGeom ), pos, pArmor, 
		GetCurrentSrcObject(), nMask, nFloor );
	InsertHull( pRes );
	Register( pRes );
	return pRes;
}

void CAIMap::AssignUserID( CObjectBase *_p, int _nUserID )
{
	if ( !_p )
		return;
	if ( CDynamicCast<CConvexHull> p = _p )
		p->AssignUserID( _nUserID );
}

CObjectBase *CAIMap::AddHull( CMemObject *pModel, 
	const SHMatrix &pos, 
	const NDb::CResource *pArmor, int nFloor, int nMask )
{
	if ( pModel->IsPolyLine() )
		return 0;
	RegisterFloor( nFloor );
	CObjectBase *pUser = pArmor ? GetCurrentSrcObject() : 0;
	CConvexHull *pRes = new CStaticConvexHull( pUserHullsTracker, new CMemGeometryInfo( pModel ), pos, pArmor,
		pUser, nMask, nFloor );
	InsertHull( pRes );
	Register( pRes );
	return pRes;
}

CObjectBase *CAIMap::AddAnimatedHull( const NDb::SAIGeometry *pAIGeom, const NAnimation::SGrannySkeletonHandle &skeletonH, 
	CFuncBase<NAnimation::SGrannySkeletonPose> *pAnimation, 
	const NDb::CResource *pArmor, int nFloor, int nMask )
{
	if ( !pAIGeom )
		return 0;
	if ( !NGScene::CResourceFileOpener::DoesExist( "AIGeometries", GetIntResKey( pAIGeom ) ) )
		return 0;
	ASSERT( IsValid( pAnimation ) );
	NGScene::CBind *pBind = new NGScene::CBind( pAnimation, skeletonH );
	CSkinner *pSkin = new CSkinner( 
		shareSkinPoints.Get( pAIGeom ),
		pBind );
	
	SHMatrix id;
	Identity( &id );
	CDynamicConvexHull *pRes = new CDynamicConvexHull( pUserHullsTracker, pSkin, id, pArmor, 
		GetCurrentSrcObject(), nMask, nFloor, 
		new NGScene::CMeshBound( pBind ), pAnimation );

	dynamicHulls.push_back( pRes );
	Register( pRes );
	// CRAP - animated objects on AI map cannot change destroy stages etc. with this ASSERT
	//ASSERT( ( nAllTrackersMask & nMask ) == 0 );
	return pRes;
}

void CAIMap::Sync( ESyncType st )
{
//	TParent::Sync();
	static int nSlowVolumeWalk;
	++nSlowVolumeWalk;
	if ( IsValid( pRoot ) && ( nSlowVolumeWalk & 0xff ) == 0 )
		pRoot->Walk();
	for ( std::vector<CPtr<CDynamicConvexHull> >::iterator i = dynamicHulls.begin(); i != dynamicHulls.end(); )
	{
		CDynamicConvexHull *pHull = *i;
		if ( !IsValid( pHull ) )
			i = dynamicHulls.erase( i );
		else
		{
			bool bInform = pHull->pAnimationTracker.Refresh(), bBoundChanged = pHull->pBound.Refresh();
			if ( bBoundChanged )
			{
				const SBound &b = pHull->pBound->GetValue();
				CVolumeNode *pNode = GetNode( b.s.ptCenter, b.s.fRadius );
				bInform = !pHull->SetNode( pNode, b );
			}
			if ( bInform && IsValid( pHull->pNode ) )
			{
				pRoot->AddInform( pHull->GetLinkedBound(), pHull->src.nTSFlags, false );
				//CallInform( pHull->pNode, pHull, pHull->GetLinkedBound() );
				if ( bBoundChanged )
				{
					const SBound &b = pHull->pBound->GetValue();
					pHull->SetLinkedBound( b );
				}
			}
			++i;
		}
	}
	if ( st == ST_FAST )
		return;
	if ( IsValid( pRoot ) )
		pRoot->CallCachedInforms();
}

void CAIMap::CallInform( CConvexHull *pHull, const SBound &b, bool bDoorFlipped )
{
	if ( pHull->src.nTSFlags & nAllTrackersMask )
		pRoot->InformTrackers( b, pHull->src.nTSFlags, bDoorFlipped );
}

void CAIMap::AddTracker( IAIMapTracker *pTracker, const SBound &b, int nMask, bool bInformOnDoorFlip )
{
	CVolumeNode *pNode = GetNode( b.s.ptCenter, b.s.fRadius );

#ifdef _DEBUG
	if ( pNode != pRoot || b.s.fRadius < 100.0f )
	{
		CVolumeNode *pPrt = pNode;
		while ( pPrt )
		{
			SSphere sph;
			pPrt->GetBound( &sph );
			float fDiff = fabs( sph.ptCenter - b.s.ptCenter );
			bool bSphereIsInside = ( fDiff <= sph.fRadius - b.s.fRadius );
			fDiff = fabs( sph.ptCenter - (b.s.ptCenter - b.ptHalfBox) );
			bool bBoxIsInside = ( fDiff <= sph.fRadius );
			fDiff = fabs( sph.ptCenter - (b.s.ptCenter + b.ptHalfBox) );
			bBoxIsInside &= ( fDiff <= sph.fRadius );
			ASSERT( bSphereIsInside || bBoxIsInside );	
			pPrt = pPrt->GetUpLink();
		}
	}
#endif

	pNode->AddTracker( pTracker, b, nMask, bInformOnDoorFlip );
	nAllTrackersMask |= nMask;
}

template<class T>
struct STestSphere
{
	T *p;
	STestSphere( T *_p ): p(_p) {}
	bool operator()( const CVec3 &ptCenter, float fRadius, bool ) const { return p->TestSphere( ptCenter, fRadius ); }
};

template<class T>
void TraceEntities( SHullSet &res, T *pRes, IAIMap::ESplitTerrainHGroups shg )
{
	if ( shg == IAIMap::STH_SPLIT_TERR_HG )
	{
		for ( std::vector<SConvexHull>::iterator i = res.terrain.begin(); i != res.terrain.end(); ++i )
			pRes->TraceEntity( *i, true );
	}
	else
		pRes->TraceEntity( res.terrain, true );
	for ( std::vector<SConvexHull>::iterator i = res.objects.begin(); i != res.objects.end(); ++i )
	{
		pRes->TraceEntity( *i, false );
	}
}

void CAIMap::Trace( const CRay &r, std::vector<SInterval> *pIntersections, int nMask, const CFloorsSet &fs, ESplitTerrainHGroups shg )
{
	SHullSet res;
	CTracer trace( *pIntersections );
	trace.InitProjection( r );
	SelectFloorSet( &res, fs, nMask, STestSphere<CTracer>( &trace ) );
	TraceEntities( res, &trace, shg );
	SortIntervals( pIntersections );
}

void CAIMap::TraceGrid( CFastRenderer *pRes, int nMask, ESort sort, const CFloorsSet &fs , ESplitTerrainHGroups shg )
{
	SHullSet res;
	SelectFloorSet( &res, fs, nMask, STestSphere<CFastRenderer>( pRes ) );
	TraceEntities( res, pRes, shg );
	if ( sort == STH_SORT_AND_REDUCE_TERRAIN )
		pRes->ReduceTerrain();
	if ( sort == STH_SORT_INTERVALS || sort == STH_SORT_AND_REDUCE_TERRAIN )
		pRes->SortIntervals();
}

struct SFrustrumSelect
{
	CTransformStack &ts;
	float fRadiusKoef;
	SFrustrumSelect( CTransformStack &_ts, float _f ): ts(_ts), fRadiusKoef(_f) {}
	bool operator()( const CVec3 &ptCenter, float fRadius, bool bCheckPiece ) const
	{
		if ( bCheckPiece )
			return ts.IsIn( SSphere( ptCenter, fRadius * fRadiusKoef ) );
		else
			return ts.IsIn( SSphere( ptCenter, fRadius ) );
	}
};
static void AddObjects( std::vector<IAIMap::SSelectedObject> *pRes, const std::vector<SConvexHull> &objects )
{
	for ( int k = 0; k < objects.size(); ++k )
	{
		const SConvexHull &c = objects[k];
		IAIMap::SSelectedObject &r = pRes->emplace_back();
		r.pUserData = c.src.pUserData;
		r.nUserID = c.nUserID;
	}
}
void CAIMap::Select( std::vector<SSelectedObject> *pRes, const CTransformStack &_ts, float fRadiusKoef, int nMask, const CFloorsSet &fs )
{
	SHullSet res;
	CTransformStack ts(_ts);
	SelectFloorSet( &res, fs, nMask, SFrustrumSelect( ts, fRadiusKoef ) );
	AddObjects( pRes, res.objects );
	AddObjects( pRes, res.terrain );
}

static bool CalcModelSphereIntersection( const SConvexHull &h, const CVec3 &ptCenter, float fRadius )
{
	// SLOW implementation
	const SHMatrix &rot = h.trans;
	std::vector<CVec3> pts;
	pts.resize( h.points.size() );
	for ( int i = 0; i < pts.size(); ++i )
		rot.RotateHVector( &pts[i], h.points[i] );
	std::vector<STriangle> tris;
	h.tris.BuildTriangleList( &tris );
	for ( int i = 0; i < tris.size(); ++i )
	{
		const STriangle &t = tris[i];
		if ( NCollider::DoesTriSphereIntersect( pts[t.i1], pts[t.i2], pts[t.i3], ptCenter, fRadius ) )
			return true;
	}
	return false;
}

struct SSphereSphere
{
	CVec3 ptCenter;
	float fRadius;

	SSphereSphere( const CVec3 &_ptCenter, float _fRadius ): ptCenter(_ptCenter), fRadius(_fRadius) {}
	bool operator()( const CVec3 &_ptCenter, float _fRadius, bool ) const 
	{ 
		return fabs2( ptCenter - _ptCenter ) < sqr( fRadius + _fRadius ); 
	}
};

bool CAIMap::CalcIntersection( const CVec3 &vCenter, float fRadius, int nMask, CObjectBase *pIgnoreUser )
{
	SHullSet res;
	SelectFloorSet( &res, CFloorsSet(), nMask, SSphereSphere( vCenter, fRadius ) );
	for ( std::vector<SConvexHull>::iterator i = res.objects.begin(); i != res.objects.end(); ++i )
	{
		if ( pIgnoreUser && pIgnoreUser == i->src.pUserData )
			continue;
		if ( CalcModelSphereIntersection( *i, vCenter, fRadius ) )
			return true;
	}
	for ( std::vector<SConvexHull>::iterator i = res.terrain.begin(); i != res.terrain.end(); ++i )
	{
		if ( pIgnoreUser && pIgnoreUser == i->src.pUserData )
			continue;
		if ( CalcModelSphereIntersection( *i, vCenter, fRadius ) )
			return true;
	}
	return false;
}

void CAIMap::GetEntities( std::vector<SObjectInfo> *pRes, int nMask, const CFloorsSet &fs )
{
	SHullSet res;
	pRes->clear();	
	SelectFloorSet( &res, fs, nMask, SAlwaysTrue() );
	Convert( pRes, res );
}

void CAIMap::SelectHullPointers( std::vector<CPtr<CObjectBase> > *pRes, const SBound &b, int nMaskOr, int nMaskNot )
{
	SelectHullPointers( pRes, b, nMaskOr, nMaskNot, pRoot );
}

void CAIMap::SelectHullPointers( std::vector<CPtr<CObjectBase> > *pRes, const SBound &b, int nMaskOr, int nMaskNot, CVolumeNode *pNode )
{
	if ( pNode == 0 )
		return;
	SSphere t;
	pNode->GetBound( &t );
	SBound test; test.SphereInit( t.ptCenter, t.fRadius );
	if ( !DoesIntersect( b, test ) )
		return;
	for ( int i = 0; i < pNode->hulls.size(); ++i )
	{
		CConvexHull *pHull = pNode->hulls[i].pHull;
		if ( pHull )
		{
			ASSERT( IsValid(pHull) );
			int nFlags = pNode->hulls[i].nFlags;
			if ( !IsValidInCurrentState( nFlags ) )
				continue;
			if ( nFlags & nMaskOr )
			{
				if ( nFlags & nMaskNot )
					continue;
				test = pNode->hullBounds[i];
				if ( DoesIntersect( b, test ) )
					pRes->push_back( pHull );
			}
		}
	}
	for ( int i = 0; i < 8; ++i )
		SelectHullPointers( pRes, b, nMaskOr, nMaskNot, pNode->GetNode(i) );
}


IAIMap* CreateAIMap()
{
	return new CAIMap( 1 );
}

void FindClosePositionOnSurface( IAIMap *pMap, const CVec3 &ptPos, CVec3 *pRes, int nFlags )
{
	CRay r;
	r.ptOrigin = ptPos;
	r.ptDir.Set( 0, 0, -1 );
	std::vector<SInterval> intersections;
	*pRes = ptPos;
	bool bIsSet = false;
	pMap->Trace( r, &intersections, nFlags );
	for ( std::vector<SInterval>::iterator it = intersections.begin(); it != intersections.end(); ++it )
	{
		if ( it->enter.fT < 0 )
			continue;
		if ( bIsSet )
			pRes->z = (std::max)( pRes->z, ptPos.z - it->enter.fT );
		else
		{
			pRes->z = ptPos.z - it->enter.fT;
			bIsSet = true;
		}
	}
}

} // namespace
using namespace NAI;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02911000, CAIMap )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x01071140, CVolumeNode )
BASIC_REGISTER_CLASS( _3DMOTOR, IAIMap )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02942160, CStaticConvexHull )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x02942161, CDynamicConvexHull )
using namespace NGScene;
REGISTER_SAVELOAD_TEMPL_CLASS( _3DMOTOR, 0x028b2140, CResourcePrecache<CLoadAIGeometryFromA5Exporter>, CResourcePrecache )
REGISTER_SAVELOAD_TEMPL_CLASS( _3DMOTOR, 0x028b2141, CResourcePrecache<CFileSkinPointsLoadFromA5Exporter>, CResourcePrecache )
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0x01443110, CUserHullsTracker )

