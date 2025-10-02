#include "StdAfx.h"
#include "./pathfractionarclinearc3d.h"
#include "PathFractionArcLine3D.h"
#include "PathFractionArc3D.h"
#include "../DebugTools/DebugInfoManager.h"
#include "../misc/StrProc.h"

REGISTER_SAVELOAD_CLASS(0x11095C81, CPahtFractionArcLineArc3D)


//	CPahtFractionArcLineArc


void CPahtFractionArcLineArc3D::GetSimplePath( CPathList *pPaths ) 
{
	CPathFractionArc3D *pStartArc = pStart->GetArc();
	CPathFractionArc3D *pFinishArc = pFinish->GetArc();

	if ( pStartArc != 0 )
		pPaths->push_back( pStartArc );
	if ( pStart )
		pPaths->push_back( pStart->GetLine() );
	if ( pFinishArc != 0 )
	{
		CPathFractionArc3D *pFinishArcCopy = new CPathFractionArc3D( *pFinishArc );
		pFinishArcCopy->Negate();
		pPaths->push_back( pFinishArcCopy );
	}
}

void CPahtFractionArcLineArc3D::Init( const SPrevPathParams &prevPath, 
																	 const CVec3 &x1, const CVec3 &v1,
																	 const float fR0, const float fR1 )
{
	Init( prevPath.p2, x1, prevPath.vSpeed, v1, fR0, fR1 );
}

void CPahtFractionArcLineArc3D::Init( const CVec3 &x0, const CVec3 &x1,			// coordinates
					 const CVec3 &v0, const CVec3 &v1,			// directions
					 const float r0, const float r1 )			// turn radii
{
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "show_plane_path", 0 ) == 1 )
	{
		CSegment segm;
		segm.p1 = CVec2( x0.x, x0.y );
		segm.p2 = CVec2( x1.x, x1.y );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::RED );
	}
#endif
	pStart = new CPathFractionArcLine3D;
	pFinish = new CPathFractionArcLine3D;

	// iterative search.
	pStart->Init( x0, v0, x1, r0 ); // forward direction, ignore v1
	CVec3 vForwardArcFinish = pStart->GetArc() ? pStart->GetArc()->GetEndPoint() : pStart->GetLine()->GetStartPoint();

	pFinish->Init( x1, -v1, vForwardArcFinish, r1 );	// backward, ignore speed on arc vForwardArcFinish
	CVec3 vBackArcFinish = pFinish->GetArc() ? pFinish->GetArc()->GetEndPoint() : pFinish->GetLine()->GetStartPoint();

	// improved forward 
	pStart->Init( x0, v0, vBackArcFinish, r0 );

	for ( int a = 0; a < 3;  ++a )
	{
		// second iteration
		vForwardArcFinish = pStart->GetArc() ? pStart->GetArc()->GetEndPoint() : pStart->GetLine()->GetStartPoint();
		pFinish->Init( x1, -v1, vForwardArcFinish, r1 );	// backward, ignore speed on arc vForwardArcFinish
		vBackArcFinish = pFinish->GetArc() ? pFinish->GetArc()->GetEndPoint() : pFinish->GetLine()->GetStartPoint();
		pStart->Init( x0, v0, vBackArcFinish, r0 );
	}
}

void PlanePathTest( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	CVec3 x0( 7073.2544f, 1606.7415f, 1149.9806f );
	CVec3 x1( 7313.3174f, 3108.7656f, 1150.0000f );
	CVec3 v0( 81.118164f, -28.335449f, -0.0026855469f );
	CVec3 v1( 0.31289077f, 0.94978905f, 2.8692173e-005f );
	float r0( 522.68530f );
	float r1(	522.68530f	);

	if ( paramsSet.size() == 2 ) 
	{
		x1.x = NStr::ToFloat( NStr::ToMBCS( paramsSet[0] ) );
		x1.y = NStr::ToFloat( NStr::ToMBCS( paramsSet[1] ) );
	}

	CPtr<CPahtFractionArcLineArc3D> pPath = new CPahtFractionArcLineArc3D;
	pPath->Init( x0, x1, v0, v1, r0, r1 );

	CPathList pathList;
	pPath->GetSimplePath( &pathList );
	CPtr<CPathFractionComplex> pCP = new CPathFractionComplex( pathList );
	
	const float fAtom = 30;
	float fPathLenght = pCP->GetLength();
	vector<CVec3> points;
	for ( float fCur = 0; fCur < fPathLenght; fCur += fAtom )
		points.push_back( pCP->GetPoint( fCur ) );

	for ( int i = 0; i < points.size() - 1; ++i )
	{
		CSegment segm;
		segm.p1 = CVec2( points[i].x, points[i].y );
		segm.p2 = CVec2( points[1+i].x, points[1+i].y );
		segm.dir = segm.p2 - segm.p1;
		DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::BLUE );
	}
}

START_REGISTER(CPlanePaths)
REGISTER_CMD( "PlanePathTest", PlanePathTest );
FINISH_REGISTER
