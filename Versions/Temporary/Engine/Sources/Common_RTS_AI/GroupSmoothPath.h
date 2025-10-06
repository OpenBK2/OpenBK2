#pragma once

#include "Common_RTS_AI_export.h"


#include "BasePathUnit.h"
#include "StandartSmoothPath.h"

struct SGeometryCellInfo
{
	CVec2 vCellPosition;
	int nPriority;

	SGeometryCellInfo() : vCellPosition( VNULL2 ), nPriority( -1 ) {}
	SGeometryCellInfo( const CVec2 &_vCellPosition, const int _nPriority ) : vCellPosition( _vCellPosition ), nPriority( _nPriority ) {}
};

//! гладкий путь для группы (формации), позиции не привязаны (!!!) к юнитам, позиции привязаны к некоему приоритету (индексу),
//! который определяет класс юнита (т.е. офицеры могут стоять на таких-то позяциях, а солдаты на таких)
class COMMON_RTS_AI_EXPORT CGroupSmoothPath : public CStandartSmoothPathBasis
{
	struct SCellInfo
	{
		//! поворот от направления формации к юниту
		CVec2 vUnitShift;
		//! проекция смещения юнита относительно центра формации на направление формации
		float fUnitProjShift;
		SCellInfo() : vUnitShift( VNULL2 ), fUnitProjShift( 0.0f ) {}
		SCellInfo( const CVec2 &vUnitPosition )
		{
			vUnitShift = vUnitPosition ^ CVec2( 0, -1 );
			fUnitProjShift = vUnitShift * CVec2( 0, 1 );
		}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &vUnitShift );
			saver.Add( 2, &fUnitProjShift );
			return 0;
		}
	};
	//! набор положений внутри группы
	typedef std::vector<SCellInfo> CCells;
	//! связь между набором положений и типом (priority) юнитов
	typedef std::unordered_map< int, CCells > CPriorityCells;
	//
	struct SGeometry
	{
		CPriorityCells geometry;
    float fMaxProjection;
		float fRadius;
		
		SGeometry() : fMaxProjection( 0.0f ), fRadius( 0.0f ) {}
		SGeometry( const CPriorityCells &_geometry, const float _fMaxProjection, const float _fRadius ) : geometry( _geometry ), fMaxProjection( _fMaxProjection ), fRadius( _fRadius ) {}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &geometry );
			saver.Add( 2, &fMaxProjection );
			saver.Add( 3, &fRadius );

			return 0;
		}
	};
	//
	struct SUnitInfo 
	{
		CBasePathUnit *pUnit;
		int nCell;
		int nPriority;

		SUnitInfo(): pUnit( 0 ), nCell( -1 ), nPriority( -1 ) {}
		SUnitInfo( CBasePathUnit *_pUnit, const int _nPriority ): pUnit( _pUnit ), nCell( -1 ), nPriority( _nPriority ) {}

		int operator&( IBinSaver &saver )
		{
			SerializeBasePathUnit( saver, 1, &pUnit );
			saver.Add( 2, &nCell );
			saver.Add( 3, &nPriority );
			return 0;
		}
	};
	//
	struct SUnitsInfoSortByPriority
	{
		bool operator()( const SUnitInfo unitInfo1, const SUnitInfo unitInfo2 ) const 
		{ 
			if ( unitInfo1.nPriority == unitInfo2.nPriority )
				return unitInfo1.pUnit->GetUniqueID() > unitInfo2.pUnit->GetUniqueID();
			else
				return unitInfo1.nPriority > unitInfo2.nPriority;
		}
	};
	typedef std::unordered_map< int, SUnitInfo > CUnitsMap;
  //
	OBJECT_BASIC_METHODS( CGroupSmoothPath )
	ZDATA_( CStandartSmoothPathBasis )
		std::vector<SGeometry> geometries;
		CUnitsMap units;
		int nCurrentGeometry;
		int nUnitsCount;
		int nCellsCount;
		float fMaxPathShift;
		float fSpeedCoeff;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CStandartSmoothPathBasis *)this); f.Add(2,&geometries); f.Add(3,&units); f.Add(4,&nCurrentGeometry); f.Add(5,&nUnitsCount); f.Add(6,&nCellsCount); f.Add(7,&fMaxPathShift); f.Add(8,&fSpeedCoeff); return 0; }
protected:
	const CVec2 GetDirection() const { return GetUnit()->GetDirectionVector(); }
	//! получить смещение юнита в формации
	const CVec2 GetUnitFormationShift( const CBasePathUnit *pUnit ) const;
	//! получить проекцию смещения юнита в формации
	const float GetUnitFormationProjection( const CBasePathUnit *pUnit ) const;
	//! вычислить ячейки для каждого юнита, с учетом наиболее выгодного совместного пути в точку vPosition
	//! с достижением направления vDirection
	void RecalcCells( const CVec2 &vPosition, const CVec2 &vDirection );
	//! вычислить ячейки с учетом текущего пути (в финальной точке, направление finish - start) или
	//! (если нет пути) по текущему положению центрального (виртуального) юнита
	void RecalcCells();

public:
	CGroupSmoothPath() : nCurrentGeometry( 0 ), nUnitsCount( 0 ), nCellsCount( 0 ), fMaxPathShift( 0.0f ), fSpeedCoeff( 1.0f ) {}
	
	virtual bool Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap );

	void ChangeGeometry( const int nGeometry ) { nCurrentGeometry = nGeometry; }
	const int GetCurrentGeometry() const { return nCurrentGeometry; }
	const int GetGeometriesCount() const { return geometries.size(); }

	bool AddUnit( CBasePathUnit *pUnit, const int nPriority );
	bool DeleteUnit( CBasePathUnit *pUnit );

	void AddGeometry( const std::vector<SGeometryCellInfo> &cells );
	void AlignGeometriesToCenter();

	const int GetUnitsCount() const { return nUnitsCount; }
	const int GetCellsCount() const { return nCellsCount; }

	const CVec2 GetCenter() const { return GetUnit()->GetCenterPlain(); }
	const CVec2 GetFarCenter() const { return PeekPathPoint( SPLINE_STEP ); }

	const float GetMaxPathShift() const { return fMaxPathShift; }
	const float GetMaxProjection() const { return geometries[GetCurrentGeometry()].fMaxProjection; }
	const float GetRadius() const { return geometries[GetCurrentGeometry()].fRadius; }
	void NotifyPathShift( const float fPathShift ) { fMaxPathShift = (std::max)( fPathShift, fMaxPathShift ); }

	virtual void Segment( const NTimer::STime timeDiff );

	//! получить смещение юнита относительно текущего центра формации, с учетом текущего направления формации
	virtual const CVec2 GetUnitShift( const CBasePathUnit *pUnit ) const { return GetDirection() ^ GetUnitFormationShift( pUnit ); };
	//! получить текущие координаты юнита
	virtual const CVec2 GetUnitCenter( const CBasePathUnit *pUnit ) const { return GetCenter() + GetUnitShift( pUnit ); };
	//! получить центр юнита, где юнит может встать на прямой между центром формации и юнитом
	virtual const CVec2 GetValidUnitCenter( const CBasePathUnit *pUnit ) const;
	//! получить текущее направление юнита
	virtual const CVec2 GetUnitDirection( const CBasePathUnit *pUnit ) const { return GetUnit()->GetDirectionVector(); }
	//! получить проекцию позиции юнита на путь (ушел вперед/назад)
	virtual const float GetUnitPathShift( const CBasePathUnit *pUnit ) const { return GetDirection() * ( pUnit->GetCenterPlain() - GetCenter() ) - GetUnitFormationProjection( pUnit ); }
};


