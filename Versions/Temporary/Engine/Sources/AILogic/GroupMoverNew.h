#pragma once

/*
#include "Stats_B2_M1/AIUnitCmd.h"
#include "Common_RTS_AI/AIClasses.h"

struct IStaticPath;
class CCommonUnit;

class CGroupMover : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CGroupMover )
	struct SSubGroupPathInfo
	{
		ZDATA
			CPtr<IStaticPath> pPath;
		int nBoundTileRadius;
		EAIClasses aiClass;
		NTimer::STime timeCalced;
		CVec2 vStartPoint;
		CVec2 vFinishPoint;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pPath); f.Add(3,&nBoundTileRadius); f.Add(4,&aiClass); f.Add(5,&timeCalced); f.Add(6,&vStartPoint); f.Add(7,&vFinishPoint); return 0; }
		SSubGroupPathInfo() : nBoundTileRadius( -1 ), aiClass( EAC_NONE ), timeCalced( 0 ), vStartPoint( VNULL2 ), vFinishPoint( VNULL2 ) {}
		SSubGroupPathInfo( const int _nBoundTileRadius, const EAIClasses _aiClass  ) : nBoundTileRadius( _nBoundTileRadius ), aiClass( _aiClass ), timeCalced( 0 ), vStartPoint( VNULL2 ), vFinishPoint( VNULL2 ) {}

		IStaticPath *CreatePathForUnit( CCommonUnit *pUnit, const CVec2 &vWantedPosition );
	};
	struct SUnitInfo
	{
		ZDATA
			CPtr<CCommonUnit> pUnit;
		CVec2 vFinistPoint;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&vFinistPoint); return 0; }
		SUnitInfo() : vFinistPoint( VNULL2 ) {}
		SUnitInfo( CCommonUnit *_pUnit ) : pUnit( _pUnit ), vFinistPoint( VNULL2 ) {}
		SUnitInfo( CCommonUnit *_pUnit, const CVec2 &_vFinishPoint ) : pUnit( _pUnit ), vFinistPoint( _vFinishPoint ) {}
	};
	struct SGroup
	{
		ZDATA
			vector<CPtr<CCommonUnit> > units;	// units in this group
		CVec2 vCenter;	// center of group
		CVec2 vDirection;	// direction to most distant unit
		float fArea;	// area occupied by group
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&units); f.Add(3,&vCenter); f.Add(4,&vDirection); f.Add(5,&fArea); return 0; }
		SGroup() : vCenter( VNULL2 ), vDirection( VNULL2 ) , fArea( 0.0f ) {}

		void AddUnit( CCommonUnit *pUnit ) { units.push_back( pUnit ); }
		void CalculateGroupInfo();
		const bool IsValid() const { return !units.empty(); }
		const bool operator>( const SGroup &group );
		const bool operator<( const SGroup &group );
	};
	typedef hash_map<int, SUnitInfo> TUnits;
	typedef vector<SSubGroupPathInfo> TSubGroupsPaths;
	ZDATA
		CVec2 vPosition;	// destination point
	bool bNeedCalcPositions;	// positions not defined
	TUnits units;
	TSubGroupsPaths subGroupsPaths;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPosition); f.Add(3,&bNeedCalcPositions); f.Add(4,&units); f.Add(5,&subGroupsPaths); return 0; }
private:
	const int BuildGroups( vector<SGroup> &groups );
	const bool CalcPositions();
public:
	CGroupMover() : vPosition( VNULL2 ), bNeedCalcPositions( true ) {}
	CGroupMover( const CVec2 &_vPosition ) : vPosition( _vPosition ), bNeedCalcPositions( true ) {}

	void AddUnit( CCommonUnit *pUnit );
	void DeleteUnit( const int nUnitID );

	IStaticPath* CreateStaticPath( CCommonUnit *pUnit );
};

CGroupMover *CreateGroupMover( const SAIUnitCmd &command );

*/
