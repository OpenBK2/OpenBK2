#include "stdafx.h"

#include "GroupSmoothPath.h"
#include "HungarianMethod.h"
#include "System/Commands.h"
#include "DebugTools/DebugInfoManager.h"

#include <algorithm>

#include <fmt/format.h>

static bool s_bShowGroupMarker = false;

const CVec2 CGroupSmoothPath::GetUnitFormationShift( const CBasePathUnit *pUnit ) const
{
	CUnitsMap::const_iterator posUnit = units.find( pUnit->GetUniqueID() );
	NI_ASSERT( posUnit != units.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {}) [not in list]", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( posUnit == units.end() )
		return VNULL2;
	const int nPriority = posUnit->second.nPriority;
	const int nCell = posUnit->second.nCell;

	if ( geometries.size() <= GetCurrentGeometry() )
		return VNULL2;
		
	CPriorityCells::const_iterator posCell = geometries[GetCurrentGeometry()].geometry.find( nPriority );
	NI_ASSERT( posCell != geometries[GetCurrentGeometry()].geometry.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {}) [wrong priority]", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( posCell == geometries[GetCurrentGeometry()].geometry.end() )
		return VNULL2;

  return posCell->second[nCell].vUnitShift;
}

const float CGroupSmoothPath::GetUnitFormationProjection( const CBasePathUnit *pUnit ) const
{
	CUnitsMap::const_iterator posUnit = units.find( pUnit->GetUniqueID() );
	NI_ASSERT( posUnit != units.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {}) [not in list]", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( posUnit == units.end() )
		return 0;
	const int nPriority = posUnit->second.nPriority;
	const int nCell = posUnit->second.nCell;

	CPriorityCells::const_iterator posCell = geometries[GetCurrentGeometry()].geometry.find( nPriority );
	NI_ASSERT( posCell != geometries[GetCurrentGeometry()].geometry.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {}) [wrong priority]", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( posCell == geometries[GetCurrentGeometry()].geometry.end() )
		return 0;

	return posCell->second[nCell].fUnitProjShift;
}

bool CGroupSmoothPath::AddUnit( CBasePathUnit *pUnit, const int nPriority )
{
	//! больше нет мест в формации
	NI_ASSERT( GetUnitsCount() < GetCellsCount(), "Cannot add more units to formation" );  
	if ( GetUnitsCount() >= GetCellsCount() )
		return false;

	//! юнит уже в формации
	CUnitsMap::const_iterator pos = units.find( pUnit->GetUniqueID() );
	NI_ASSERT( pos == units.end(), fmt::format( "Unit (ID: {}) already in formation (ID: {})", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( pos != units.end() )
		return false;

	units[ pUnit->GetUniqueID() ] = SUnitInfo( pUnit, nPriority );
	nUnitsCount++;

	RecalcCells();
	return true;
}

bool CGroupSmoothPath::DeleteUnit( CBasePathUnit *pUnit )
{	
	CUnitsMap::iterator pos = units.find( pUnit->GetUniqueID() );
	NI_ASSERT( pos != units.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {})", pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
	if ( pos == units.end() )
		return false;
	units.erase( pos );
	nUnitsCount--;

	RecalcCells();
	return true;
}

void CGroupSmoothPath::RecalcCells()
{
	if ( GetPath() != 0 )
	{
		CVec2 vDir( GetPath()->GetFinishPoint() - GetPath()->GetStartPoint() );
		Normalize( &vDir );
		RecalcCells( GetPath()->GetFinishPoint(), vDir );
	}
	else
		RecalcCells( GetUnit()->GetCenterPlain(), GetUnit()->GetDirectionVector() );
}

void CGroupSmoothPath::RecalcCells( const CVec2 &vPosition, const CVec2 &vDirection )
{
	std::vector<SUnitInfo> unitsList;
	unitsList.reserve( GetUnitsCount() );
	for ( CUnitsMap::iterator it = units.begin(); it != units.end(); ++it )
		unitsList.push_back( it->second );
	std::sort( unitsList.begin(), unitsList.end(), SUnitsInfoSortByPriority() );

	std::vector<SUnitInfo>::iterator itStart = unitsList.begin();
	while ( itStart != unitsList.end() )
	{
		const int nPriority = itStart->nPriority;
		std::vector<SUnitInfo>::iterator itEnd = itStart;
		int nCount = 0;
		for ( ; ( itEnd != unitsList.end() && itEnd->nPriority == nPriority ); ++itEnd, ++nCount );

		CPriorityCells::iterator pos = geometries[GetCurrentGeometry()].geometry.find( nPriority );
    NI_ASSERT( pos != geometries[GetCurrentGeometry()].geometry.end(), fmt::format( "Priority group ({}) not found in cells", nPriority ) );
		if ( pos != geometries[GetCurrentGeometry()].geometry.end() )
		{
			NI_ASSERT( nCount <= pos->second.size(), fmt::format( "Too much units with priority {} ({} > {})", nPriority, nCount, pos->second.size() ) );
			if ( nCount <= pos->second.size() ) 
			{
				const int nSize = (std::min<int>)( nCount, pos->second.size() );
				CArray2D<float> distMatrix;
				distMatrix.SetSizes( nSize, nSize );
				{
					int i = 0;
					for ( std::vector<SUnitInfo>::iterator it = itStart; it != itEnd; ++it )
					{
						for ( int j = 0; j < nSize; ++j )
						{
//							const CVec2 vDestUnitShift = vDirection ^ pos->second[j].vUnitShift;
//							const CVec2 vCellFinishPoint = vPosition + vDestUnitShift;
							const CVec2 vCellFinishPoint(vPosition + ( vDirection ^ pos->second[j].vUnitShift ) );
							distMatrix[j][i] = fabs2( vCellFinishPoint - it->pUnit->GetCenterPlain() );
						}
						i++;
					}
				}

				CHungarianMethod hungMethod;
				hungMethod.Init( distMatrix );
				hungMethod.Solve();
				{
					int i = 0;
					for ( std::vector<SUnitInfo>::iterator it = itStart; it != itEnd; ++it )
					{
						it->nCell = hungMethod.GetResult( i );
						i++;
					}
				}
				itStart = itEnd;
			}
		}
	}
	for ( std::vector<SUnitInfo>::iterator it = unitsList.begin(); it != unitsList.end(); ++it )
	{
		CUnitsMap::iterator pos = units.find( it->pUnit->GetUniqueID() );
		if ( pos != units.end() )
			pos->second.nCell = it->nCell;
	}
}

void CGroupSmoothPath::AddGeometry( const std::vector<SGeometryCellInfo> &cells )
{
	const CVec2 vDir1( 0, 1 );

	NI_ASSERT( ( GetCellsCount() == 0 || cells.size() == GetCellsCount() ), "Wrong geometry." );
	if ( GetCellsCount() != 0 && cells.size() != GetCellsCount() )
		return;
	
	if ( GetCellsCount() == 0 )
		nCellsCount = cells.size();

	CPriorityCells priorityCells;

	float fMaxProjection = 0.0f;
	float fRadius = 0.0f;
	for ( int i = 0; i < cells.size(); ++i )
	{
		CPriorityCells::iterator pos = priorityCells.find( cells[i].nPriority );
		if ( pos == priorityCells.end() )
		{
			CCells newCells;
			newCells.push_back( SCellInfo( cells[i].vCellPosition ) );
			fMaxProjection = (std::max)( newCells.back().fUnitProjShift, fMaxProjection );
			fRadius = (std::max)( fabs( vDir1 ^ newCells.back().vUnitShift ), fRadius );
			priorityCells[ cells[i].nPriority ] = newCells;
		}
		else
		{
			pos->second.push_back( SCellInfo( cells[i].vCellPosition ) );
			fMaxProjection = (std::max)( pos->second.back().fUnitProjShift, fMaxProjection );
			fRadius = (std::max)( fabs( vDir1 ^ pos->second.back().vUnitShift ), fRadius );
		}
	}
	geometries.push_back( SGeometry( priorityCells, fMaxProjection, fRadius ) );
}

void CGroupSmoothPath::AlignGeometriesToCenter()
{
	const CVec2 vDir1( 0, 1 );
	const CVec2 vDir_1( 0, -1 );

	for ( int i = 0; i < geometries.size(); ++i )
	{
		CVec2 vNewCenter( VNULL2 );
		for ( CUnitsMap::iterator it = units.begin(); it != units.end(); ++it )
		{
			const CVec2 vUnitShift( GetUnitFormationShift( it->second.pUnit ) );
			vNewCenter += vDir1 ^ vUnitShift;
		}

		vNewCenter /= GetUnitsCount();
		float fMaxProjection = 0.0f;
		float fRadius = 0.0f;

		for ( CUnitsMap::iterator it = units.begin(); it != units.end(); ++it )
		{
			CPriorityCells::iterator posCell = geometries[i].geometry.find( it->second.nPriority );
			NI_ASSERT( posCell != geometries[i].geometry.end(), fmt::format( "Unit (ID: {}) not found in formation (ID: {}) [wrong priority]", it->second.pUnit->GetUniqueID(), GetUnit()->GetUniqueID() ) );
			if ( posCell != geometries[i].geometry.end() )
			{
				const CVec2 vShift = -1.0f * vNewCenter + ( vDir1 ^ posCell->second[it->second.nCell].vUnitShift );

				posCell->second[it->second.nCell].vUnitShift = vShift ^ vDir_1;

				const float fProjection = vShift * vDir1;
				posCell->second[it->second.nCell].fUnitProjShift = fProjection;

				fMaxProjection = (std::max)( fMaxProjection, fProjection );
			}
			geometries[i].fMaxProjection = fMaxProjection;
		}
	}

	RecalcCells();
}

void CGroupSmoothPath::Segment( const NTimer::STime timeDiff )
{
	if ( fMaxPathShift > 0.0f )
		fSpeedCoeff = (std::min)( 4.0f, (std::max)( 1.0f, fMaxPathShift/GetAIMap()->GetTileSize() ) );

	CVec2 vOldDirection = GetDirection();
	CStandartSmoothPathBasis::Segment( timeDiff );

#ifndef _FINALRELEASE
	if ( s_bShowGroupMarker )
	{
		DebugInfoManager()->CreateCircle( GetUnit()->GetUniqueID(), CCircle( GetUnit()->GetCenterPlain(), 24 ), NDebugInfo::RED );
		for ( CUnitsMap::iterator it = units.begin(); it != units.end(); ++it )
			DebugInfoManager()->CreateCircle( it->second.pUnit->GetUniqueID(), CCircle( GetValidUnitCenter( it->second.pUnit ), 8 ), NDebugInfo::GREEN );
	}
#endif

	const float fSpeedHere = GetUnit()->GetMaxSpeedHere();
	GetUnit()->SetSpeed( fSpeedHere/fSpeedCoeff );
	fMaxPathShift = -1.0f;
}

const CVec2 CGroupSmoothPath::GetValidUnitCenter( const CBasePathUnit *pUnit ) const
{
	return GetAIMap()->GetTerrain()->GetValidPoint( pUnit->GetBoundTileRadius(), GetCenter(), GetUnitCenter( pUnit ), pUnit->GetAIPassabilityClass(), false, GetAIMap() );
}

bool CGroupSmoothPath::Init( CBasePathUnit *pUnit, IPath *pPath, bool bSmoothTurn, bool bCheckTurn, CAIMap *pAIMap )
{
	if ( CStandartSmoothPathBasis::Init( pUnit, pPath, bSmoothTurn, bCheckTurn, pAIMap ) )
	{
		RecalcCells();
		return true;
	}
	return false;
}

START_REGISTER( GroupSmoothPathVars )
REGISTER_VAR_EX( "show_group_marker", NGlobal::VarBoolHandler, &s_bShowGroupMarker, false, STORAGE_NONE );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x310C9B00, CGroupSmoothPath );


