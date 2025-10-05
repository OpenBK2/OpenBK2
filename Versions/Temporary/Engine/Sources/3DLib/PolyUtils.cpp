#include "stdafx.h"
#include "PolyUtils.h"
#include "Ring.h"

#include <algorithm>

// CONST

#pragma optimize( "p", on )
enum EOper
{
	OP_SUB,
	OP_XOR,
	OP_UNION,
	OP_INTERSECTION
};
const int
	CLIP_UNKNOWN				= 0x00,
	CLIP_MARK						= 0x01,
	CLIP_SHARED_SAME		= 0x02,
	CLIP_SHARED_TOWARDS	= 0x04,
	CLIP_INSIDE					= 0x08,
	CLIP_OUTSIDE				= 0x10,
	CLIP_DELETE         = 0x20;


// CODE

bool IsPolygonInverse( const std::vector<CVec2> &vPolygon )
{
	float fSquare = 0;

	for ( int nTemp = 0; nTemp < vPolygon.size(); nTemp++ )
	{
		const CVec2 &vBeg = vPolygon[nTemp];
		const CVec2 &vEnd = vPolygon[(nTemp + 1) % vPolygon.size()];

		float fMin = Min( vBeg.x, vEnd.x );
		fSquare += ( vEnd.y + vBeg.y ) * ( vEnd.x - vBeg.x );
	}

	return fSquare > 0;
}

bool IsPointInPolygon( const std::vector<CVec2> &vPolygon, const CVec2 &vPoint )
{
	bool bInside = false;
	
	for ( int nTemp = 0; nTemp < vPolygon.size(); nTemp++ )
	{
		const CVec2 &vBeg = vPolygon[nTemp];
		const CVec2 &vEnd = vPolygon[( nTemp + 1 ) % vPolygon.size()];
		if ( ( vEnd.y <= vPoint.y ) && ( vPoint.y < vBeg.y ) && ( ( vBeg.y - vEnd.y ) * ( vPoint.x - vEnd.x) < ( vPoint.y - vEnd.y ) * ( vBeg.x - vEnd.x ) ) )
			bInside = !bInside;
		else if ( ( vBeg.y <= vPoint.y ) && ( vPoint.y < vEnd.y ) && ( ( vBeg.y - vEnd.y) * ( vPoint.x - vEnd.x ) > ( vPoint.y - vEnd.y ) * ( vBeg.x - vEnd.x ) ) )
			bInside = !bInside;
	}
	
	return bInside;
}

struct SPlane2
{
	CVec2 n;
	float f;
};
static void BuildPlane( SPlane2 *pRes, const CVec2 &vFrom, const CVec2 &vTo )
{
	SPlane2 &edge = *pRes;
	edge.n.x = vTo.y - vFrom.y;
	edge.n.y = vFrom.x - vTo.x;
	if ( vFrom.y > vTo.y )
		edge.f = - vFrom.x * edge.n.x - vFrom.y * edge.n.y;
	else if ( vFrom.y < vTo.y )
		edge.f = - vTo.x * edge.n.x - vTo.y * edge.n.y;
	else if ( vFrom.x > vTo.x )
		edge.f = - vFrom.x * edge.n.x - vFrom.y * edge.n.y;
	else
		edge.f = - vTo.x * edge.n.x - vTo.y * edge.n.y;
}
static bool GetCross( const SPlane2 &p1, const SPlane2 &p2, CVec2 *pRes )
{
	float fDet = p1.n.x * p2.n.y - p1.n.y * p2.n.x;
	if ( fDet == 0 )
		return false;
	fDet = 1.0f / fDet;
	pRes->x = fDet * ( -p2.n.y * p1.f + p1.n.y * p2.f );
	pRes->y = fDet * (  p2.n.x * p1.f - p1.n.x * p2.f );
	return true;
}
float Dot( const CVec2 &a, const CVec2 &b ) { return a.x * b.x + a.y * b.y; }
float CalcSide( const SPlane2 &p, const CVec2 &vPoint )
{
	return p.f + Dot( p.n, vPoint );
}
static bool IsInLine( const CVec2 &b, const CVec2 &e, const CVec2 &v )
{
	CVec2 vDir( e - b );
	if ( Dot( vDir, v - b ) >= 0 && Dot( -vDir, v - e ) >= 0 )
		return true;
	return false;
}
float IsAlmostSame( float a, float b )
{
	return fabsf( a - b ) <= 0.0001f * a;//Max( a, b );
}
static ELineDispos TrueIntersectLines( const CVec2 &vLine1Beg, const CVec2 &vLine1End, const CVec2 &vLine2Beg, const CVec2 &vLine2End, CVec2 *pvRes )
{
	SPlane2 p1, p2;
	BuildPlane( &p1, vLine1Beg, vLine1End );
	BuildPlane( &p2, vLine2Beg, vLine2End );
	if ( !GetCross( p1, p2, pvRes ) )
	{
		// either parallel or same
		if ( p1.n.x * p2.f == p2.n.x * p1.f && p1.n.y * p2.f == p2.n.y * p1.f )
		//if ( IsAlmostSame( p1.n.x * p2.f, p2.n.x * p1.f ) && IsAlmostSame( p1.n.y * p2.f, p2.n.y * p1.f ) )
			return SAME;
		else
			return PARALLEL;
	}
	float f1b = CalcSide( p2, vLine1Beg );
	float f1e = CalcSide( p2, vLine1End );
	float f2b = CalcSide( p1, vLine2Beg );
	float f2e = CalcSide( p1, vLine2End );
	float f1 = f1b * f1e, f2 = f2b * f2e;
	if ( f1 == 0 )
	{
		if ( f1b == 0 && IsInLine( vLine2Beg, vLine2End, vLine1Beg ) )
		{
			*pvRes = vLine1Beg;
			return INTERSECT;
		}
		if ( f1e == 0 && IsInLine( vLine2Beg, vLine2End, vLine1End ) )
		{
			*pvRes = vLine1End;
			return INTERSECT;
		}
	}
	if ( f2 == 0 )
	{
		if ( f2b == 0 && IsInLine( vLine1Beg, vLine1End, vLine2Beg ) )
		{
			*pvRes = vLine2Beg;
			return INTERSECT;
		}
		if ( f2e == 0 && IsInLine( vLine1Beg, vLine1End, vLine2End ) )
		{
			*pvRes = vLine2End;
			return INTERSECT;
		}
	}
	if ( f1 > 0 )
		return NOTINTERSECT;
	if ( f2 > 0 )
		return NOTINTERSECT;
	return INTERSECT;
}

bool IsMore( const CVec2 &a, const CVec2 &b )
{
	return a.y > b.y || ( a.y == b.y && a.x > b.x );
}

ELineDispos IntersectLines( const CVec2 &vLine1Beg, const CVec2 &vLine1End, const CVec2 &vLine2Beg, const CVec2 &vLine2End, CVec2 *pvRes )
{
	const CVec2 *p1b = &vLine1Beg;
	const CVec2 *p1e = &vLine1End;
	const CVec2 *p2b = &vLine2Beg;
	const CVec2 *p2e = &vLine2End;
	if ( IsMore( *p1e, *p1b ) )
		std::swap( p1b, p1e );
	if ( IsMore( *p2e, *p2b ) )
		std::swap( p2b, p2e );
	if ( IsMore( *p1b, *p2b ) )
	{
		std::swap( p1b, p2b );
		std::swap( p1e, p2e );
	}
	return TrueIntersectLines( *p1b, *p1e, *p2b, *p2e, pvRes );
}

void DumpPolyList( const TPolygonsList &polygonsList )
{
	/*DebugTrace( "%d polygons\n", polygonsList.size() );
	for ( TPolygonsList::const_iterator iTempPoly = polygonsList.begin(); iTempPoly != polygonsList.end(); iTempPoly++ )
	{
		DebugTrace( "%d points in polygon(%s)\n", iTempPoly->size(), IsPolygonInverse( *iTempPoly ) ? "inverse" : "normal" );
		for ( vector<CVec2>::const_iterator iTempPoint = iTempPoly->begin(); iTempPoint != iTempPoly->end(); iTempPoint++ )
			DebugTrace( "%.2f %.2f\n", iTempPoint->x, iTempPoint->y );
	}*/
}


// POLYGON CLIPPER

class CPolyClipper
{
protected:
	struct SLink;
	struct SEdge;
	struct SPoint;
	struct SPolygon;
		
	enum EDir
	{
		DIR_FORWARD,
		DIR_BACKWARD
	};
	enum EPolygon
	{
		POLY_SOURCE,
		POLY_CLIPPOLY
	};
	struct SLink
	{
		float fAngle;
		EPolygon eType;
		CRing<SEdge>::iterator iEdge;

		SLink(): fAngle( 0.0f ) {}
		SLink( float _fAngle, EPolygon _eType, CRing<SEdge>::iterator _iEdge ): fAngle( _fAngle ), eType( _eType ), iEdge( _iEdge ) {}
	};
	struct SPoint
	{
		CVec2 vPoint;
		CRing<SLink> listLinks;

		SPoint( const CVec2 &_vPoint ): vPoint( _vPoint ) {}
	};
	struct SEdge
	{
		int nFlags;
		CRing<SEdge>::iterator iSharedEdge;
		std::list<SPoint>::iterator iBegPoint;
		std::list<SPoint>::iterator iEndPoint;
		EPolygon polygon;

		SEdge(): nFlags( CLIP_UNKNOWN ) {}
	};
	struct SPolygon
	{
		bool bInverse;
		CRing<SEdge> listEdges;
	};
	
	struct SSortLinks
	{
	public:
		bool operator()( const SLink &sI1, const SLink &sI2 ) const 
		{ 
			return ( sI1.fAngle < sI2.fAngle ); 
		}
	};
	struct SLinkSet
	{
		float fAngle;
		std::vector<SLink> links;
	};
	
	std::list<SPoint> listPoints;
	std::list<SPolygon> listPolygons;
	std::list<SPolygon> listClipPolygons;
	
	void ShowInfo();

	void AddPolygon( const std::vector<CVec2> &inPolygon, SPolygon *psPolygon, EPolygon polygon );
	void AddPolygonsList( const TPolygonsList &polyList, std::list<SPolygon> *pRes, EPolygon polygon );

	void ClearMarks( SPolygon *psPolygon );
	void MakeLinks( EPolygon eType, SPolygon *psPolygon );
	void SortLinks();
	void DeletePeerEdges( SPolygon *psPolygon );
	void MakeFlags( SPolygon *psPolygon, std::list<SPolygon> *pContPolygons );
	std::list<SPoint>::iterator InsertPoint( const CVec2 &vPoint );
	void InsertEdge( SPolygon *pResult, const SEdge &edge, std::vector<CVec2> *pCrossPoints );
	//void InsertPointOnLine( const CVec2 &vPoint, list<CRing<SEdge>::iterator> *pListSplits, SPolygon *pResPolygon );
	void FindIntersections( const SPolygon &sPolygon, const SPolygon &sClipPolygon, SPolygon *psResult );
	bool IsPointInPolygon( const CVec2 &vPoint, const SPolygon &polygon );

	bool EdgeRule( EOper eOp, EPolygon ePoly, const SEdge &sEdge, EDir *peDir );
	void JumpLink( EOper eOp, SPolygon *psPolygon, CRing<SEdge>::iterator *piEdge, EDir *pcDir );
	void CollectPolygons( EOper eOp, EPolygon eType, SPolygon *psPolygon, std::list< std::vector<CVec2> > *psList );
		
	void DumpEdge( const SEdge &sEdge );
	void DumpPolygon( const SPolygon &sPolygon );

public:
	CPolyClipper() {}

	void SetPolygons( const TPolygonsList &sourceList, const TPolygonsList &sourceClipList );
	void Boolean( EOper eOp, TPolygonsList *pListPolygons );
};


void CPolyClipper::DumpEdge( const SEdge &sEdge )
{
	/*DebugTrace( "(%.2f %.2f)-(%.2f %.2f)", sEdge.iBegPoint->vPoint.x, sEdge.iBegPoint->vPoint.y, sEdge.iEndPoint->vPoint.x, sEdge.iEndPoint->vPoint.y );
	DebugTrace( " Flags: " );
	if ( sEdge.nFlags & CLIP_MARK )
		DebugTrace( "MARK " );
	if ( sEdge.nFlags & CLIP_INSIDE )
		DebugTrace( "INSIDE " );
	if ( sEdge.nFlags & CLIP_OUTSIDE )
		DebugTrace( "OUTSIDE " );
	if ( sEdge.nFlags & CLIP_SHARED_SAME )
		DebugTrace( "SHARED_SAME " );
	if ( sEdge.nFlags & CLIP_SHARED_TOWARDS )
		DebugTrace( "SHARED_TOWARDS " );

	DebugTrace( "\n" );*/
}

void CPolyClipper::DumpPolygon( const SPolygon &sPolygon )
{
	//DebugTrace( "Polygon(%s) edges (%d):\n", sPolygon.bInverse ? "inverse":"normal", sPolygon.listEdges.size() );

	CRing<SEdge>::const_iterator iTemp = sPolygon.listEdges.begin();
	for ( int nTemp = 0; nTemp < sPolygon.listEdges.size(); nTemp++, iTemp++ )
		DumpEdge( *iTemp );
}

void CPolyClipper::SetPolygons( const TPolygonsList &sourceList, const TPolygonsList &sourceClipList )
{
	AddPolygonsList( sourceList, &listPolygons, POLY_SOURCE );
	AddPolygonsList( sourceClipList, &listClipPolygons, POLY_CLIPPOLY );

	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
	{
		SPolygon &sInPolygon = *iTemp;
		for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		{
			SPolygon &sInClipPolygon = *iTemp;

			SPolygon sTempPolygon, sTempClipPolygon;
			FindIntersections( sInPolygon, sInClipPolygon, &sTempPolygon );
			FindIntersections( sInClipPolygon, sInPolygon, &sTempClipPolygon );

			sInPolygon = sTempPolygon;
			sInClipPolygon = sTempClipPolygon;
		}
	}

	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
		DeletePeerEdges( &*iTemp );
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		DeletePeerEdges( &*iTemp );

	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
		MakeLinks( POLY_SOURCE, &(*iTemp) );
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		MakeLinks( POLY_CLIPPOLY, &(*iTemp) );

	SortLinks();

	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
	{
		SPolygon &sInPolygon = *iTemp;
		MakeFlags( &sInPolygon, &listClipPolygons );
	}
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
	{
		SPolygon &sInClipPolygon = *iTemp;
		MakeFlags( &sInClipPolygon, &listPolygons );
	}
}

void CPolyClipper::Boolean( EOper eOp, std::list< std::vector<CVec2> > *pListPolygons )
{
	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
		ClearMarks( &(*iTemp) );
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		ClearMarks( &(*iTemp) );

	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
		CollectPolygons( eOp, POLY_SOURCE, &(*iTemp), pListPolygons );
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		CollectPolygons( eOp, POLY_CLIPPOLY, &(*iTemp), pListPolygons );
}

//	Internal functions

bool CPolyClipper::IsPointInPolygon( const CVec2 &vPoint, const SPolygon &sPolygon )
{
	bool bInside = false;
	
	CRing<SEdge>::const_iterator iTemp = sPolygon.listEdges.begin();
	for ( int nTemp = 0; nTemp < sPolygon.listEdges.size(); nTemp++, iTemp++ )
	{
		const CVec2 &vBeg = iTemp->iBegPoint->vPoint;
		const CVec2 &vEnd = iTemp->iEndPoint->vPoint;
		if ( ( vEnd.y <= vPoint.y ) && ( vPoint.y < vBeg.y ) && ( ( vBeg.y - vEnd.y ) * ( vPoint.x - vEnd.x) < ( vPoint.y - vEnd.y ) * ( vBeg.x - vEnd.x ) ) )
			bInside = !bInside;
		else if ( ( vBeg.y <= vPoint.y ) && ( vPoint.y < vEnd.y ) && ( ( vBeg.y - vEnd.y) * ( vPoint.x - vBeg.x ) > ( vPoint.y - vBeg.y ) * ( vBeg.x - vEnd.x ) ) )
			bInside = !bInside;
	}
	
	return bInside;
}

void CPolyClipper::AddPolygon( const std::vector<CVec2> &inPolygon, SPolygon *psPolygon, EPolygon polygon )
{
	psPolygon->listEdges.clear();

	for ( int nTemp = 0; nTemp < inPolygon.size(); nTemp++ )
	{
		const CVec2 &vBegPoint = inPolygon[ nTemp % inPolygon.size() ];
		const CVec2 &vEndPoint = inPolygon[ ( nTemp + 1 ) % inPolygon.size() ];

		bool bBegSet = false, bEndSet = false;
		std::list<SPoint>::iterator iBegPoint, iEndPoint;
		iBegPoint = InsertPoint( vBegPoint );
		iEndPoint = InsertPoint( vEndPoint );

		if ( iBegPoint == iEndPoint )
			continue;
		CRing<SEdge>::iterator iEdge = psPolygon->listEdges.add();
		iEdge->iBegPoint = iBegPoint;
		iEdge->iEndPoint = iEndPoint;
		//const CVec2 &vB = iEdge->iBegPoint->vPoint;
		//const CVec2 &vE = iEdge->iEndPoint->vPoint;
		//iEdge->fAngle = atan2( vE.y - vB.y, vE.x - vB.x ) * 360 / FP_2PI;
		//while ( iEdge->fAngle < 0 )
		//			iEdge->fAngle += 360;
		iEdge->polygon = polygon;
	}
}

void CPolyClipper::AddPolygonsList( const TPolygonsList &polyList, std::list<SPolygon> *pRes, EPolygon polygon )
{
	for ( TPolygonsList::const_iterator iTemp = polyList.begin(); iTemp != polyList.end(); iTemp++ )
	{
		SPolygon &sPoly = pRes->emplace_back();
		AddPolygon( *iTemp, &sPoly, polygon );
		sPoly.bInverse = IsPolygonInverse( *iTemp );
	}
}

void CPolyClipper::MakeLinks( EPolygon eType, SPolygon *psPolygon )
{
	CRing<SEdge>::iterator iEdge = psPolygon->listEdges.begin();
	for ( int nTemp = 0; nTemp < psPolygon->listEdges.size(); nTemp++, iEdge++ )
	{
		if ( iEdge->nFlags & CLIP_DELETE )
			continue;
		const CVec2 &vB = iEdge->iBegPoint->vPoint;
		const CVec2 &vE = iEdge->iEndPoint->vPoint;
		float fAngle = atan2( vE.y - vB.y, vE.x - vB.x ) * 360 / FP_2PI;
		while ( fAngle < 0 )
			fAngle += 360;

		iEdge->iBegPoint->listLinks.add( SLink( fAngle, eType, iEdge ) );
		float fBackAngle = fAngle + 180;
		if ( fBackAngle >= 360 )
			fBackAngle -= 360;
		iEdge->iEndPoint->listLinks.add( SLink( fBackAngle, eType, iEdge ) );
	}
}

static float GetAngleDif( float a, float b )
{
	float f = fabs( a - b );
	return Min( f, 360 - f );
}

void CPolyClipper::SortLinks()
{
	for ( std::list<SPoint>::iterator iPoint = listPoints.begin(); iPoint != listPoints.end(); iPoint++ )
	{
		std::vector<SLink> vSortedLinks( iPoint->listLinks.size() );

		int nLink;
		CRing<SLink>::iterator iLink = iPoint->listLinks.begin();
		for ( nLink = 0; nLink < iPoint->listLinks.size(); nLink++, iLink++ )
			vSortedLinks[nLink] = *iLink;
		std::sort( vSortedLinks.begin(), vSortedLinks.end(), SSortLinks() );
		//ASSERT( !vSortedLinks.empty() );
		if ( vSortedLinks.empty() )
			continue;

		bool bIsCrossPoint = false;
		EPolygon test = vSortedLinks[0].iEdge->polygon;
		for ( int k = 1; k < vSortedLinks.size(); ++k )
		{
			if ( vSortedLinks[k].iEdge->polygon != test )
			{
				bIsCrossPoint = true;
				break;
			}
		}
		if ( bIsCrossPoint )
		{
			// group edges by angle
			std::vector<SLinkSet> links;
			for ( int k = 0; k < vSortedLinks.size(); ++k )
			{
				if ( !links.empty() && GetAngleDif( links.back().fAngle, vSortedLinks[k].fAngle ) < 0.01f )
				{
					links.back().links.push_back( vSortedLinks[k] );
				}
				else
				{
					SLinkSet &ls = links.emplace_back();
					ls.fAngle = vSortedLinks[k].fAngle;
					ls.links.push_back( vSortedLinks[k] );
				}
			}
			// for each incident edge set inside or outside flags
			std::vector<int> vCountSrc( links.size() ), vCountClip( links.size() );
			int nCountSrc = 0, nMinCountSrc = 1000, nMaxCountSrc = -1000;
			int nCountClip = 0, nMinCountClip = 1000, nMaxCountClip = -1000;
			for ( int k = 0; k < links.size(); ++k )
			{
				const SLinkSet &ls = links[k];
				vCountSrc[k] = nCountSrc;
				vCountClip[k] = nCountClip;
				for ( int i = 0; i < ls.links.size(); ++i )
				{
					const SLink &lnk = ls.links[i];
					if ( lnk.iEdge->polygon == POLY_SOURCE )
					{
						if ( lnk.iEdge->iBegPoint == iPoint )
							++nCountSrc;
						if ( lnk.iEdge->iEndPoint == iPoint )
							--nCountSrc;
					}
					else
					{
						if ( lnk.iEdge->iBegPoint == iPoint )
							++nCountClip;
						if ( lnk.iEdge->iEndPoint == iPoint )
							--nCountClip;
					}
				}
				nMinCountSrc = Min( nMinCountSrc, nCountSrc );
				nMaxCountSrc = Max( nMaxCountSrc, nCountSrc );
				nMinCountClip = Min( nMinCountClip, nCountClip );
				nMaxCountClip = Max( nMaxCountClip, nCountClip );
			}
			ASSERT( nMaxCountSrc - nMinCountSrc <= 1 );
			ASSERT( nMaxCountClip - nMinCountClip <= 1 );
			if ( nMaxCountSrc - nMinCountSrc == 1 && nMaxCountClip - nMinCountClip == 1 )
			{
				for ( int k = 0; k < links.size(); ++k )
				{
					const SLinkSet &ls = links[k];
					if ( ls.links.size() != 1 )
						continue;
					const SLink &lnk = ls.links[0];
					int nFlags;
					if ( lnk.eType == POLY_SOURCE )
						nFlags = vCountClip[k] - nMinCountClip > 0 ? CLIP_INSIDE : CLIP_OUTSIDE;
					else
						nFlags = vCountSrc[k] - nMinCountSrc > 0 ? CLIP_INSIDE : CLIP_OUTSIDE;
					ASSERT( lnk.iEdge->nFlags == CLIP_UNKNOWN || lnk.iEdge->nFlags == nFlags );
					lnk.iEdge->nFlags = nFlags;
				}
			}
		}
		// store resulting links
		iPoint->listLinks.clear();
		for ( nLink = 0; nLink < vSortedLinks.size(); nLink++ )
		{
			iPoint->listLinks.add( vSortedLinks[nLink] );
		}
	}
}

void CPolyClipper::ClearMarks( SPolygon *psPolygon )
{
	CRing<SEdge>::iterator iEdge = psPolygon->listEdges.begin();
	for ( int nTemp = 0; nTemp < psPolygon->listEdges.size(); nTemp++, iEdge++ )
	{
		iEdge->nFlags = iEdge->nFlags & ( ~CLIP_MARK );
	}
}

void CPolyClipper::DeletePeerEdges( SPolygon *psPolygon )
{
	// loop through points & search for same
	CRing<SEdge>::iterator iPolyEdge = psPolygon->listEdges.begin();
	for ( int nPoly = 0; nPoly < psPolygon->listEdges.size(); nPoly++, iPolyEdge++ )
	{
		const CVec2 &vPolyLineBeg = iPolyEdge->iBegPoint->vPoint;
		const CVec2 &vPolyLineEnd = iPolyEdge->iEndPoint->vPoint;

		CRing<SEdge>::iterator iClipEdge = psPolygon->listEdges.begin();
		for ( int nClip = 0; nClip < psPolygon->listEdges.size(); nClip++, iClipEdge++ )
		{
			const CVec2 &vClipLineBeg = iClipEdge->iBegPoint->vPoint;
			const CVec2 &vClipLineEnd = iClipEdge->iEndPoint->vPoint;
			if ( vPolyLineBeg == vClipLineEnd && vPolyLineEnd == vClipLineBeg )
			{
				iPolyEdge->nFlags = CLIP_DELETE;
				iClipEdge->nFlags = CLIP_DELETE;
			}
		}
	}
}

void CPolyClipper::MakeFlags( SPolygon *psPolygon, std::list<SPolygon> *pContPolygons )
{
	// loop through points & search for same
	for ( std::list<SPolygon>::iterator i = pContPolygons->begin(); i != pContPolygons->end(); ++i )
	{
		SPolygon &contPolygon = *i;
		CRing<SEdge>::iterator iPolyEdge = psPolygon->listEdges.begin();
		for ( int nPoly = 0; nPoly < psPolygon->listEdges.size(); nPoly++, iPolyEdge++ )
		{
			if ( iPolyEdge->nFlags & CLIP_DELETE )
				continue;
			const CVec2 &vPolyLineBeg = iPolyEdge->iBegPoint->vPoint;
			const CVec2 &vPolyLineEnd = iPolyEdge->iEndPoint->vPoint;

			CRing<SEdge>::iterator iClipEdge = contPolygon.listEdges.begin();
			for ( int nClip = 0; nClip < contPolygon.listEdges.size(); nClip++, iClipEdge++ )
			{
				if ( iClipEdge->nFlags & CLIP_DELETE )
					continue;
				const CVec2 &vClipLineBeg = iClipEdge->iBegPoint->vPoint;
				const CVec2 &vClipLineEnd = iClipEdge->iEndPoint->vPoint;

				if ( vPolyLineBeg == vClipLineBeg && vPolyLineEnd == vClipLineEnd )
				{
					iPolyEdge->nFlags = CLIP_SHARED_SAME;
					iPolyEdge->iSharedEdge = iClipEdge;
					break;
				}
				else if ( vPolyLineBeg == vClipLineEnd && vPolyLineEnd == vClipLineBeg )
				{
					iPolyEdge->nFlags = CLIP_SHARED_TOWARDS;
					iPolyEdge->iSharedEdge = iClipEdge;
					break;
				}
			}
		}
	}

	// loop through edges & propagate existing marks
	{
		CRing<SEdge>::iterator iPolyEdge = psPolygon->listEdges.begin();
		for(;;)
		{
			bool bTouched = false;
			for ( int nPoly = 0; nPoly < psPolygon->listEdges.size(); nPoly++, iPolyEdge++ )
			{
				if ( iPolyEdge->nFlags != CLIP_INSIDE && iPolyEdge->nFlags != CLIP_OUTSIDE )
					continue;
				CRing<SLink> &links = iPolyEdge->iEndPoint->listLinks;
				CRing<SLink>::iterator iLink = links.begin();
				for ( int i = 0; i < links.size(); ++i, ++iLink )
				{
					if ( iLink->iEdge->polygon != iPolyEdge->polygon )
						continue;
					if ( iLink->iEdge->nFlags == CLIP_UNKNOWN )
					{
						ASSERT( iLink->iEdge->polygon == iPolyEdge->polygon );
						iLink->iEdge->nFlags = iPolyEdge->nFlags;
						bTouched = true;
					}
				}
			}
			if ( !bTouched )
				break;
		}
	}

	// mark the rest with point in polygon test
	{
		CRing<SEdge>::iterator iPolyEdge = psPolygon->listEdges.begin();
		for ( int nPoly = 0; nPoly < psPolygon->listEdges.size(); nPoly++, iPolyEdge++ )
		{
			if ( iPolyEdge->nFlags != CLIP_UNKNOWN )
				continue;
			const CVec2 &vPolyLineBeg = iPolyEdge->iBegPoint->vPoint;
			const CVec2 &vPolyLineEnd = iPolyEdge->iEndPoint->vPoint;
			CVec2 vTest( 0.5f * ( vPolyLineBeg + vPolyLineEnd ) );
			bool bTestRet = false, bIsInHole = false;
			for ( std::list<SPolygon>::iterator i = pContPolygons->begin(); i != pContPolygons->end(); ++i )
			{
				SPolygon &contPolygon = *i;
				if ( contPolygon.bInverse )
					bIsInHole |= IsPointInPolygon( vTest, contPolygon );
				else
					bTestRet |= IsPointInPolygon( vTest, contPolygon );
			}
			bTestRet &= !bIsInHole;
			iPolyEdge->nFlags = bTestRet ? CLIP_INSIDE : CLIP_OUTSIDE;
		}
	}
}

const float F_GRID = 4096;
std::list<CPolyClipper::SPoint>::iterator CPolyClipper::InsertPoint( const CVec2 &_vPoint )
{
	CVec2 vPoint( Float2Int( _vPoint.x * F_GRID ) * (1/F_GRID), Float2Int( _vPoint.y * F_GRID ) * (1/F_GRID) );
	for ( std::list<SPoint>::iterator iTemp = listPoints.begin(); iTemp != listPoints.end(); ++iTemp )
	{
		if ( iTemp->vPoint == vPoint )
			return iTemp;
	}
	return listPoints.insert( listPoints.end(), SPoint( vPoint ) );
}

struct SCrossSort
{
	CVec2 vDir;
	
	SCrossSort( const CVec2 &_vDir ) : vDir(_vDir) {}
	bool operator()( const CVec2 &a, const CVec2 &b )
	{
		return Dot( a, vDir ) < Dot( b, vDir );
	}
};
void CPolyClipper::InsertEdge( SPolygon *pResult, const SEdge &_edge, std::vector<CVec2> *pCrossPoints )
{
	const CVec2 &vStart = _edge.iBegPoint->vPoint;
	const CVec2 &vEnd = _edge.iEndPoint->vPoint;
	sort( pCrossPoints->begin(), pCrossPoints->end(), SCrossSort( vEnd - vStart ) );
	std::list<SPoint>::iterator iPrev = _edge.iBegPoint;
	SEdge edge(_edge);
	for ( int k = 0; k < pCrossPoints->size(); ++k )
	{
		edge.iBegPoint = iPrev;
		iPrev = InsertPoint( (*pCrossPoints)[k] );
		edge.iEndPoint = iPrev;
		if ( edge.iBegPoint != edge.iEndPoint )
			pResult->listEdges.add( edge );
	}
	edge.iBegPoint = iPrev;
	edge.iEndPoint = _edge.iEndPoint;
	if ( edge.iBegPoint != edge.iEndPoint )
		pResult->listEdges.add( edge );
}

// добавляем в первый полигон все пересечения с ребрами второго
void CPolyClipper::FindIntersections( const SPolygon &sPolygon, const SPolygon &sClipPolygon, SPolygon *psResult )
{
	SPolygon sResPolygon;
	
	CRing<SEdge>::const_iterator iPolyEdge = sPolygon.listEdges.begin();
	for ( int nPoly = 0; nPoly < sPolygon.listEdges.size(); nPoly++, iPolyEdge++ )
	{
		const CVec2 &vPolyLineBeg = iPolyEdge->iBegPoint->vPoint;
		const CVec2 &vPolyLineEnd = iPolyEdge->iEndPoint->vPoint;
		
		//list<CRing<SEdge>::iterator> listSplits;
		//listSplits.push_back( sResPolygon.listEdges.add( *iPolyEdge ) );
		std::vector<CVec2> crossPoints;
		
		CRing<SEdge>::const_iterator iClipEdge = sClipPolygon.listEdges.begin();
		for ( int nClip = 0; nClip < sClipPolygon.listEdges.size(); nClip++, iClipEdge++ )
		{
			const CVec2 &vClipLineBeg = iClipEdge->iBegPoint->vPoint;
			const CVec2 &vClipLineEnd = iClipEdge->iEndPoint->vPoint;
			
			CVec2 vTemp;
			ELineDispos eDispos = IntersectLines( vPolyLineBeg, vPolyLineEnd, vClipLineBeg, vClipLineEnd, &vTemp );
			
			if ( eDispos == SAME )
			{
				// most complex case - up to 2 intersections
				CVec2 vDir( vPolyLineEnd - vPolyLineBeg );
				if ( Dot( vDir, vClipLineBeg - vPolyLineBeg ) > 0 && Dot( -vDir, vClipLineBeg - vPolyLineEnd ) > 0 )
					crossPoints.push_back( vClipLineBeg );
				if ( Dot( vDir, vClipLineEnd - vPolyLineBeg ) > 0 && Dot( -vDir, vClipLineEnd - vPolyLineEnd ) > 0 )
					crossPoints.push_back( vClipLineEnd );
				// 
			}
			if ( eDispos == INTERSECT )
			{
				// skip intersection if it falls exactly into one of edge points
				if ( vTemp == vPolyLineBeg || vTemp == vPolyLineEnd )
					continue;
				crossPoints.push_back( vTemp );
			}
		}
		InsertEdge( &sResPolygon, *iPolyEdge, &crossPoints );
	}
	
	psResult->bInverse = sPolygon.bInverse;
	psResult->listEdges = sResPolygon.listEdges;
}

bool CPolyClipper::EdgeRule( EOper eOp, EPolygon ePoly, const SEdge &sEdge, EDir *peDir )
{
	switch( eOp )
	{
		case OP_SUB:
			if ( ( ( sEdge.nFlags & CLIP_OUTSIDE ) || ( sEdge.nFlags & CLIP_SHARED_TOWARDS ) ) && ( ePoly == POLY_SOURCE ) )
			{
				*peDir = DIR_FORWARD;
				return true;
			}
			else if ( ( ( sEdge.nFlags & CLIP_INSIDE ) || ( sEdge.nFlags & CLIP_SHARED_TOWARDS ) ) && ( ePoly == POLY_CLIPPOLY ) )
			{
				*peDir = DIR_BACKWARD;
				return true;
			}
			break;
		case OP_XOR:
			if ( sEdge.nFlags & CLIP_OUTSIDE )
			{
				*peDir = DIR_FORWARD;
				return true;
			}
			else if ( sEdge.nFlags & CLIP_INSIDE )
			{
				*peDir = DIR_BACKWARD;
				return true;
			}
			break;
		case OP_UNION:
			if ( ( sEdge.nFlags & CLIP_OUTSIDE ) || ( sEdge.nFlags & CLIP_SHARED_SAME ) )
			{
				*peDir = DIR_FORWARD;
				return true;
			}
			break;
		case OP_INTERSECTION:
			if ( ( sEdge.nFlags & CLIP_INSIDE ) || ( sEdge.nFlags & CLIP_SHARED_SAME ) )
			{
				*peDir = DIR_FORWARD;
				return true;
			}
			break;
	}
	
	return false;
}

void CPolyClipper::JumpLink( EOper eOp, SPolygon *psPolygon, CRing<SEdge>::iterator *piEdge, EDir *pcDir )
{
	std::list<SPoint>::iterator iPoint;
	if ( *pcDir == DIR_FORWARD )
		iPoint = (*piEdge)->iEndPoint;
	else
		iPoint = (*piEdge)->iBegPoint;

	bool bSet = false;
	CRing<SLink>::iterator iEdgeLink = iPoint->listLinks.begin(), iTempLink = iPoint->listLinks.begin();
	do
	{
		if ( !bSet && ( iTempLink->iEdge == *piEdge ) )
		{
			bSet = true;
			iEdgeLink = iTempLink;
		}
		else if ( bSet && ( iTempLink->fAngle != iEdgeLink->fAngle ) )
		{
			EDir eTempDir;
			if ( EdgeRule( eOp, iTempLink->eType, *( iTempLink->iEdge ), &eTempDir ) )
			{
				*pcDir = eTempDir;
				*piEdge = iTempLink->iEdge;
				return;
			}
		}
		iTempLink--;
	}while ( iTempLink != iEdgeLink );


//// DEBUG
	/*switch( eOp )
	{
		case OP_SUB:
			DebugTrace( "Op SUB\n" );
			break;
		case OP_INTERSECTION:
			DebugTrace( "Op INT\n" );
			break;
	}*/

	if ( *pcDir == DIR_FORWARD )
		iPoint = (*piEdge)->iEndPoint;
	else
		iPoint = (*piEdge)->iBegPoint;

	//DebugTrace( "Links count %d:\n", iPoint->listLinks.size() );

	bSet = false;
	iEdgeLink = iPoint->listLinks.begin();
	iTempLink = iPoint->listLinks.begin();
	do
	{
		if ( !bSet && ( iTempLink->iEdge == *piEdge ) )
		{
			bSet = true;
			iEdgeLink = iTempLink;
			//DebugTrace( "Selected ");
		}
		else if ( bSet && ( iTempLink->fAngle != iEdgeLink->fAngle ) )
		{
			EDir eTempDir;
			if ( EdgeRule( eOp, iTempLink->eType, *( iTempLink->iEdge ), &eTempDir ) )
			{
				*pcDir = eTempDir;
				*piEdge = iTempLink->iEdge;

				//DebugTrace( "Found (%s) ", iTempLink->eType == POLY_SOURCE ? "source" : "clip" );
				DumpEdge( *( iTempLink->iEdge ) );
				return;
			}
			//DebugTrace( "Skiped (%s) ", iTempLink->eType == POLY_SOURCE ? "source" : "clip" );
		}

		DumpEdge( *( iTempLink->iEdge ) );
		iTempLink--;
	}while ( iTempLink != iEdgeLink );

	//DebugTrace( "Source:\n" );
	for ( std::list<SPolygon>::iterator iTemp = listPolygons.begin(); iTemp != listPolygons.end(); iTemp++ )
		DumpPolygon( *iTemp );
	//DebugTrace( "Clip:\n" );
	for ( std::list<SPolygon>::iterator iTemp = listClipPolygons.begin(); iTemp != listClipPolygons.end(); iTemp++ )
		DumpPolygon( *iTemp );
	//DebugTrace( "ERROR: Can't find jump-link for point %f %f\n", iPoint->vPoint.x, iPoint->vPoint.y );
	//ASSERT( 0 );
}

void CPolyClipper::CollectPolygons( EOper eOp, EPolygon eType, SPolygon *psPolygon, std::list< std::vector<CVec2> > *psList )
{
	CRing<SEdge>::iterator iEdge = psPolygon->listEdges.begin();
	for ( int nTemp = 0; nTemp < psPolygon->listEdges.size(); nTemp++, iEdge++ )
	{
		EDir eDir;
		if ( ( ( iEdge->nFlags & (CLIP_MARK|CLIP_DELETE)	) != 0 ) || !EdgeRule( eOp, eType, *iEdge, &eDir ) )
			continue;

		std::vector<CVec2> vResult;
		CRing<SEdge>::iterator iTemp = iEdge;
		
		do
		{
			if ( eDir == DIR_FORWARD )
				vResult.push_back( iTemp->iBegPoint->vPoint );
			else
				vResult.push_back( iTemp->iEndPoint->vPoint );
			
			iTemp->nFlags |= CLIP_MARK;
//			if ( ( iTemp->nFlags & CLIP_SHARED_SAME ) || ( iTemp->nFlags & CLIP_SHARED_TOWARDS ) )
//				iTemp->iSharedEdge->nFlags |= CLIP_MARK;

			JumpLink( eOp, psPolygon, &iTemp, &eDir );

		} while( ( iTemp->nFlags & CLIP_MARK ) == 0 );

		if ( vResult.size() > 2 )
			psList->push_back( vResult );
	}
}

void ClipPolygon(	const TPolygonsList &sourceList, const TPolygonsList &clipSourceList, TPolygonsList *pIntersRes, TPolygonsList *pSubRes )
{
	CPolyClipper polyClipper;
	
	polyClipper.SetPolygons( sourceList, clipSourceList );

	if ( pSubRes != 0 )
		polyClipper.Boolean( OP_SUB, pSubRes );
	if ( pIntersRes != 0 )
		polyClipper.Boolean( OP_INTERSECTION, pIntersRes );
}

