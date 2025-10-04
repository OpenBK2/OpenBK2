#include "stdafx.h"
#include "./pathfractionarclinearc.h"
#include "PathFractionArc.h"
#include "PathFractionLine.h"

REGISTER_SAVELOAD_CLASS(0x11095C80, CPahtFractionArcLineArc)


//	CPahtFractionArcLineArc


void CPahtFractionArcLineArc::Init( const SPrevPathParams &prevPath, 
																		const CVec3 &x1, const CVec3 &v1,
																		const float fR1, const float fR2 )
{
	Init( prevPath.p2, x1, prevPath.vSpeed, v1, fR1, fR2 );
}

void CPahtFractionArcLineArc::Init( const CVec3 &x0, const CVec3 &x1,
					const CVec3 &v0, const CVec3 &v1,
					const float fR0, const float fR1 )
{
	Init( CVec2(x0.x, x0.y), CVec2(x1.x, x1.y),
		CVec2(v0.x, v0.y), CVec2(v1.x, v1.y), 
		fR0, fR1,
		x0.z );
}

float CPahtFractionArcLineArc::GetLength() const
{ 
	return pStart->GetLength() + pLine->GetLength() + pFinish->GetLength(); 
}

void CPahtFractionArcLineArc::GetSimplePath( CPathList *pPaths )
{
	if ( pStart )
		pPaths->push_back( pStart.GetPtr() );
	if ( pLine )
		pPaths->push_back( pLine.GetPtr() );
	if ( pFinish )
		pPaths->push_back( pFinish.GetPtr() );
}

void CPahtFractionArcLineArc::Init( const CVec2 &x0, const CVec2 &x1, const CVec2 &v0, const CVec2 &v1, const float fR0, const float fR1, const float _fZ )
{
	// create 2 circles, tangent to initial direction
	CDirectedCircle r1, r2;												// circles, tangent to initial direction
	CDirectedCircle o1, o2;												// circles, tangent to final direction

	CVec2 v0Normalized = v0;
	Normalize( &v0Normalized );
	GetDirectedCirclesByTangent( v0Normalized , x0, fR0, &r1, &r2 );
	//GetCirclesByTangent( v0Normalized , x0, fStartRadius, &r1, &r2 );

	// to filal direction
	if ( VNULL2 != v1 )
	{
		CVec2 v1Normalized =	v1;
		Normalize( &v1Normalized );
		GetDirectedCirclesByTangent( v1Normalized , x1, fR1, &o1, &o2 );
	}
	else
	{
		o1.r = o2.r = 0;
		o1.center = o2.center = x1;
		o1.nDir = -1;
		o2.nDir = 1;
	}

	CPtr<CPahtFractionArcLineArc> pPath;

	bool bInitted = false;
	pPath = new CPahtFractionArcLineArc;
	TryPath( x0, x1, r1, o1, pPath, this, &bInitted, _fZ );
	pPath = new CPahtFractionArcLineArc;
	TryPath( x0, x1, r1, o2, pPath, this, &bInitted, _fZ );

	pPath = new CPahtFractionArcLineArc;
	TryPath( x0, x1, r2, o1, pPath, this, &bInitted, _fZ );
	pPath = new CPahtFractionArcLineArc;
	TryPath( x0, x1, r2, o2, pPath, this, &bInitted, _fZ );
}

void CPahtFractionArcLineArc::TryPath( const CVec2 &x0, const CVec2 &x1, 
																						const CDirectedCircle &r1, const CDirectedCircle &o1, 
																						CPahtFractionArcLineArc *pPath, CPahtFractionArcLineArc *pBest,
																						bool *bInitted, const float _fZ ) const
{
	CVec2 v0, v1;

	if ( GetDirectedCirclesTangentPoints( r1, o1, &v0, &v1 ) )
	{
		pPath->Init( r1, x0, v0, o1, v1, x1, _fZ );
		if ( !*bInitted )
			*pBest = *pPath;
		else 
		{
			const float fL1 = pPath->GetLength();
			const float fL2 = pBest->GetLength();
			if ( fL1 < fL2 )
				*pBest = *pPath;
		}

		*bInitted = true;
	}
}

void CPahtFractionArcLineArc::Init( const CDirectedCircle &_start, const CVec2 &_vStart1, const CVec2 &_vStart2,
																				 const CDirectedCircle &_finish, const CVec2 &_vFinish1, const CVec2 &_vFinish2,
																				 const float _fZ )
{
	if ( !pStart || !pFinish || !pLine )
	{
		pStart = new CPathFractionArc;
		pFinish = new CPathFractionArc;
		pLine = new CPathFractionLine;
	}

	pStart->Init( _start, _vStart1, _vStart2, _fZ );
	pFinish->Init( _finish, _vFinish1, _vFinish2, _fZ );
	pLine->Init( CVec3(_vStart2, _fZ), CVec3(_vFinish1, _fZ ) );
}

