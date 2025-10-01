#pragma once

#include "Common_RTS_AI_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Path.h"
#include "Tools.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define STANDART_SMOOTH_SOLDIER_PATH 0x311133C0
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! запоминание состояние для CStandartSmoothPath
class CStandartSmoothPathMemento : public IMemento
{
	OBJECT_BASIC_METHODS( CStandartSmoothPathMemento );
	CPtr<IPath> pPath;
public:
	CStandartSmoothPathMemento() : pPath( 0 ) {}
	CStandartSmoothPathMemento( IPath *_pPath ) : pPath( _pPath ) {}

	IPath* GetPath() const { return pPath; }
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pPath );
		return 0;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! //! сглаженный путь для разных солдатиков, сглаживание IPath при помощи сплайнов, упрощенные повороты
class COMMON_RTS_AI_EXPORT CStandartSmoothPathBasis : public ISmoothPath
{
	// владелец пути
	CBasePathUnit *pUnit;
	ZDATA
		ZONSERIALIZE
		ZSKIP

		CPtr<IPath> pPath;
		CBSpline spline;

		bool bFinished, bStopped;

		CVec2 p0, p1, p2, p3;
		CVec2 predPoint;
		int nIter;
		int nPoints;
		CPtr<CAIMap> pAIMap;
		public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(3,&pPath); f.Add(4,&spline); f.Add(5,&bFinished); f.Add(6,&bStopped); f.Add(7,&p0); f.Add(8,&p1); f.Add(9,&p2); f.Add(10,&p3); f.Add(11,&predPoint); f.Add(12,&nIter); f.Add(13,&nPoints); f.Add(14,&pAIMap); return 0; }
		private:

	void OnSerialize( IBinSaver &f );
	// use -1 for checking path from predPoint
	const bool IsPathBroken( const int nStartPoint ) const;
protected:
	virtual const CVec2 MoveUnit( const NTimer::STime timeDiff, const float fSpeed );
	virtual const bool ValidateCurrentPath( const CVec2 &vCenter, const CVec2 &vNewPoint );

	void SetSplineLastPoint( const CVec2 &vPoint ) { p3 = vPoint; }

	const CVec2 GetSplineFirstPoint() const { return p0; }
	const CVec2 GetSplineLastPoint() const { return p3; }
	const CVec2 GetSplineDX() const { return spline.GetDX(); }
	const CVec2 GetSplinePoint() const { return spline.GetPoint(); }
	const float GetSplineRadius() const { return spline.GetReverseR(); }
	const bool IsSplinePointsEqual() const { return ( p0 == p1 && p1 == p2 && p2 == p3 ); } 

	void StartForwardIterating( CBSpline::SForwardIter *pIter ) { spline.StartForwardIterating( pIter ); }
	void IterateSpline() { spline.Iterate(); ++nIter; } 
	const int GetNIter() { return nIter; }
	void IterateForwardSpline( CBSpline::SForwardIter *pIter ) { spline.IterateForward( pIter ); }
	void SetSplinePoints( const CVec2 &vPoint ) { p0 = p1 = p2 = p3 = predPoint = vPoint; }
	void RecalcSpline() { spline.Init( p0, p1, p2, p3 ); }
	int InitSpline();
	
	void FinishPath();
	CAIMap* GetAIMap() const { return pAIMap; }

public:
	CStandartSmoothPathBasis() : pPath( 0 ), pUnit( 0 ), bFinished( true ), bStopped( true ), 
		p0( VNULL2 ), p1( VNULL2 ), p2( VNULL2 ), p3( VNULL2 ), predPoint( VNULL2 ),
		nIter( 0 ), nPoints( 0 ) {}
	// возвращает - пошёл юнит по пути или нет
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap );
	virtual bool Init( IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap );

	virtual bool IsFinished() const { return ( GetPath() == 0 || bFinished ); }

	virtual void Stop();

	virtual void Segment( const NTimer::STime timeDiff );

	virtual const bool CanGoBackward() const { return true; }
	virtual const bool CanGoForward() const { return true; }
	virtual const CVec2 PeekPathPoint( const int nShift ) const;

	virtual IMemento* CreateMemento() const
	{ 
		return new CStandartSmoothPathMemento( GetPath() );
	}

	IPath *GetPath() const { return pPath; }
	CBasePathUnit *GetUnit() const { return pUnit; }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStandartSmoothPath : public CStandartSmoothPathBasis
{
	OBJECT_NOCOPY_METHODS( CStandartSmoothPath );

	ZDATA_( CStandartSmoothPathBasis )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStandartSmoothPathBasis *)this); return 0; }
public:
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
