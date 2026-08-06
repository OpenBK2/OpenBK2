#pragma once

#include "Stats_B2_M1/AIUnitCmd.h"
#include "Common_RTS_AI/AIClasses.h"

struct IStaticPath;
class CCommonUnit;

class CGroupMover : public CAIObjectBase
{
	struct SSubGroup
	{
		struct SSubGroupUnitInfo
		{
			ZDATA
				CPtr<CCommonUnit> pUnit;
				CVec2 vPosition;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&vPosition); return 0; }
			SSubGroupUnitInfo() : vPosition( VNULL2 ) {}
			SSubGroupUnitInfo( CCommonUnit *_pUnit ) : pUnit( _pUnit ), vPosition( VNULL2 ) {}
		};
		typedef det_map<int, SSubGroupUnitInfo> TSubGroupUnits;
		struct SSubGroupPathInfo
		{
			ZDATA
				CPtr<IStaticPath> pStaticPath;
				NTimer::STime timeCalced;
				CVec2 vStartPoint;
				CVec2 vFinishPoint;
				int nBoundTileRadius;
			ZEND int operator&( IBinSaver &f ) { f.Add(2,&pStaticPath); f.Add(3,&timeCalced); f.Add(4,&vStartPoint); f.Add(5,&vFinishPoint); f.Add(6,&nBoundTileRadius); return 0; }
			SSubGroupPathInfo() : timeCalced( 0 ), vStartPoint( VNULL2 ), vFinishPoint( VNULL2 ), nBoundTileRadius( 0 ) {}
			IStaticPath* CreateStaticPath( CCommonUnit *pUnit, const CVec2 &vPoint );
		};
		typedef det_map<EAIClasses, SSubGroupPathInfo, SEnumHash> TSubGroupsPaths;
		ZDATA
			TSubGroupUnits units; // units's ID -> SSubGroupUnitInfo
			TSubGroupsPaths paths;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&units); f.Add(3,&paths); return 0; }

		SSubGroup() {}
		void Add( CCommonUnit *pUnit );
		void Delete( const int nUnitID );
		void BalanceGroup( const CVec2 &vPoisiton );
		const bool IsEmpty() const { return units.empty(); }
		IStaticPath* CreateStaticPath( CCommonUnit *pUnit, const CVec2 &vDefaultPoint );
	};
	
	typedef det_map<int, CPtr<CCommonUnit> > TGroup; // unit's ID -> unit
	typedef std::vector<SSubGroup> TSubGroups;
	OBJECT_NOCOPY_METHODS( CGroupMover )
	ZDATA
		CVec2 vPosition;
		TGroup group;					// собственно юниты которых надо двигать
		TSubGroups subGroups;	// подгруппы по котором строятся пути, необходимо чтобы одинаковые (aiClass, nBoundTileRadius) ехали по одному пути
		bool bNeedCalcPositions;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPosition); f.Add(3,&group); f.Add(4,&subGroups); f.Add(5,&bNeedCalcPositions); return 0; }
private:
	const bool CalcPositions();
public:
	CGroupMover() : vPosition( VNULL2 ), bNeedCalcPositions( true ) {}
	CGroupMover( const CVec2 &_vPosition ) : vPosition( _vPosition ), bNeedCalcPositions( true ) {}
	
	void AddUnit( CCommonUnit *pUnit );
	void DeleteUnit( const int nUnitID );

	// Return the same deterministic per-unit destination that path-based units receive.
	CVec2 GetMoveTarget( CCommonUnit *pUnit );
	IStaticPath* CreateStaticPath( CCommonUnit *pUnit );
};

CGroupMover *CreateGroupMover( const SAIUnitCmd &command );


