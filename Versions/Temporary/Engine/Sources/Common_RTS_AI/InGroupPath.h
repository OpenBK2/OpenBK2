#pragma once

#include "Common_RTS_AI_export.h"


#include "GroupSmoothPath.h"

class CInGroupPathMemento : public IMemento
{
	OBJECT_BASIC_METHODS( CInGroupPathMemento );
	CPtr<CGroupSmoothPath> pGroupSmoothPath;
	CPtr<ISmoothPath> pUnitOwnPath;

public:
	CInGroupPathMemento() : pGroupSmoothPath() {}
	CInGroupPathMemento( CGroupSmoothPath *_pGroupSmoothPath, ISmoothPath *_pUnitOwnPath ) : pGroupSmoothPath( _pGroupSmoothPath ), pUnitOwnPath( _pUnitOwnPath ) {}

	CGroupSmoothPath* GetSmoothGroupPath() const { return pGroupSmoothPath; }
	ISmoothPath* GetUnitOwnPath() const { return pUnitOwnPath; }
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &pGroupSmoothPath );
		saver.Add( 2, &pUnitOwnPath );
		return 0;
	}
};

//! гладкий путь для движения юнита внутри группы. При инициализации пути необходимо указать CGroupSmoothPath
class COMMON_RTS_AI_EXPORT CInGroupPathBasis : public ISmoothPath
{
	CBasePathUnit *pUnit;
	CBasePathUnit *pFormation;
	ZDATA
		ZONSERIALIZE
		ZSKIP
		ZSKIP
		CPtr<CGroupSmoothPath> pSmoothGroupPath;
		CPtr<ISmoothPath> pUnitOwnPath;
		NTimer::STime timeToSearchPathToBack;

		bool bFinished;
		bool bGoByOwnPath;
		CPtr<CAIMap> pAIMap;
		public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(4,&pSmoothGroupPath); f.Add(5,&pUnitOwnPath); f.Add(6,&timeToSearchPathToBack); f.Add(7,&bFinished); f.Add(8,&bGoByOwnPath); f.Add(9,&pAIMap); return 0; }
		private:

	void OnSerialize( IBinSaver &f );
	void CantFindPathToFormation();
	void CutDriveToFormationPath( IStaticPath *pPath );
	bool CanGoToFormationPos( const CVec2 &newCenter, const CVec2 &vDesPos, const CVec2 &vFormPos );
	void ValidateCurPath( const CVec2 &newCenter );
	bool DriveToFormation( const CVec2 &newCenter, const bool bAnyPoint );
protected:
	const CVec2 MoveUnit( const NTimer::STime timeDiff, const float fSpeed );
public:
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *_pAIMap )
	{
		pAIMap = _pAIMap;
		return false;
	}

	virtual bool Init( IMemento *pMemento, CBasePathUnit *pUnit, CAIMap *pAIMap );
	virtual bool Init( CBasePathUnit *pUnit, CGroupSmoothPath *pGroupSmoothPath, const bool bSmoothTurn, const bool bCheckTurn, CAIMap *pAIMap );

	virtual bool IsFinished() const
	{
		if ( bGoByOwnPath )
			return pUnitOwnPath->IsFinished();
		else
			return pSmoothGroupPath->IsFinished() || bFinished;
	}

	virtual void Stop();

	virtual void Segment( const NTimer::STime timeDiff );

	virtual const bool CanGoBackward() const { return false; }
	virtual const bool CanGoForward() const { return true; }

	virtual const CVec2 PeekPathPoint( const int nShift ) const { return pUnit->GetCenterPlain(); }
	virtual IMemento* CreateMemento() const { return new CInGroupPathMemento( pSmoothGroupPath, pUnitOwnPath ); }
};

class CInGroupPath : public CInGroupPathBasis
{
	OBJECT_NOCOPY_METHODS( CInGroupPath );

	ZDATA_( CInGroupPathBasis )
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CInGroupPathBasis *)this); return 0; }
public:
};

