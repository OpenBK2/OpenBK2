#include "StdAfx.h"

#include "../Stats_B2_M1/AIUnitCmd.h"
#include "GroupMoveExecutor.h"
#include "CommonStates.h"
#include "ExecutorContainer.h"
#include "../Common_RTS_AI/AIMap.h"
#include "../Common_RTS_AI/HungarianMethod.h"
#include "../Common_RTS_AI/CommonPathFinder.h"
#include "../Common_RTS_AI/StaticPathInternal.h"
#include "../System/Commands.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXTERNVAR CExecutorContainer theExecutorContainer;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x31130B40, CGroupMoveExecutor );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// коэфициент для получения расстояния между юнитами в группе
static float AABB_HALF_SIZE_X_MULTIPLIER = 7.0f;
static float AABB_HALF_SIZE_Y_MULTIPLIER = 3.5f;
// расстояния между свигами бнитов вдоль перпендикуляра к пути, чтобы впереди идущий мешал сзади идущему
static float MAX_DIFF_BETWEEN_SHIFTS = 100.0f;
// квадрат расстояния между юнитами, чтобы впереди идущий мешал сзади идущему
static float MAX_DIFF_BETWEEN_UNITS = 200000.0f; 
// расстояние, на которое отъезжает юнит по направлению пути, чтобы пропустить сзади идущих
static float FORWARD_SHIFT = 256.0f;
// расстояние, с которого применяются построения
static float MIN_DISTANCE_FOR_FORMATION = 2560.0f;  
// количесвто юнитов в шеренге
static int COLUMN_SIZE = 5;	
// количество сегментов, через которые проверять состояние юнитов
static int SEGMENT_PERIOD_CHECK = 20;
// количество тайлов для определения направления пути
static int TILES_FOR_PATH_DIRECTION = 48;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline float GetUnitPosition( const CCommonUnit *pUnit, const CVec2 &vStartPoint, const CVec2 &vDirection )
{
	return ( pUnit->GetCenterPlain() - vStartPoint ) * vDirection;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitsSortByDirection
{
	CVec2 vStartPoint, vDirection;
	SUnitsSortByDirection( const CVec2 &_vStartPoint, const CVec2 &_vDirection )
		: vStartPoint( _vStartPoint ), vDirection( _vDirection ) {}
		bool operator()( const SUnitListInfo pUnitInfo1, const SUnitListInfo pUnitInfo2 ) const 
		{ 
			const float fValue1 = GetUnitPosition( pUnitInfo1.pUnit, vStartPoint, vDirection );
			const float fValue2 = GetUnitPosition( pUnitInfo2.pUnit, vStartPoint, vDirection );
			if ( fValue1 == fValue2 )
				return pUnitInfo2.pUnit->GetUniqueID() > pUnitInfo1.pUnit->GetUniqueID();
			else
				return fValue1 > fValue2;
		}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitsSortByPriority
{
	CVec2 vStartPoint, vDirection;

	SUnitsSortByPriority( const CVec2 &_vStartPoint, const CVec2 &_vDirection )
		: vStartPoint( _vStartPoint ), vDirection( _vDirection ) {}

		bool operator()( const SUnitListInfo pUnitInfo1, const SUnitListInfo pUnitInfo2 ) const 
		{ 
			const int nValue1 = pUnitInfo1.pUnit->GetUnitPriority();
			const int nValue2 = pUnitInfo2.pUnit->GetUnitPriority();
			if ( nValue1 == nValue2 )
			{
				const float fValue1 = GetUnitPosition( pUnitInfo1.pUnit, vStartPoint, vDirection );
				const float fValue2 = GetUnitPosition( pUnitInfo2.pUnit, vStartPoint, vDirection );
				if ( fValue1 == fValue2 )
					return pUnitInfo2.pUnit->GetUniqueID() > pUnitInfo1.pUnit->GetUniqueID();
				else
					return fValue1 > fValue2;
			}
			else
				return nValue1 > nValue2;
		}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DebugTraceMarkStaticPath( const IStaticPath *_pPath )
{
	const CCommonStaticPath *pPath = dynamic_cast<const CCommonStaticPath *>(_pPath);

	if ( pPath )
	{
		vector<SVector> tiles;
		tiles.reserve( pPath->GetLength() );
		for ( int i = 0; i < pPath->GetLength(); ++i )
			tiles.push_back( pPath->GetTile( i ) );
		DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_FORGET, tiles, NDebugInfo::RED );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::DeleteDeadUnits()
{
	CUnitsList::iterator it = units.begin();
	while ( it != units.end() )
	{
		if ( IsValidObj( it->pUnit ) )
			++it;
		else
			it = units.erase( it );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGroupMoveExecutor::SendUnit( CCommonUnit *pUnit )
{
	if ( pUnit->GetState()->GetName() == EUSN_MOVE_BY_FORMATION )
	{
//		DebugInfoManager()->CreateCircle( GetInstanceID(), CCircle( vFinishPoint, 24 ), NAIVisInfo::RED );

		CMoveByFormationState *pState = checked_cast<CMoveByFormationState *>( pUnit->GetState() );
		return pState->SendUnit( vFinishPoint );
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGroupMoveExecutor::SendUnitToPosition( CCommonUnit *pUnit, const CVec2 &vDestination )
{
	if ( pUnit->GetState()->GetName() == EUSN_MOVE_BY_FORMATION )
	{
//		DebugInfoManager()->CreateCircle( GetInstanceID(), CCircle( vFinishPoint, 24 ), NAIVisInfo::GREEN );
		
		CMoveByFormationState *pState = checked_cast<CMoveByFormationState *>( pUnit->GetState() );
		return pState->SendUnitToPosition( vDestination );
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::FinishState( const int nUnitID )
{
	CCommonUnit *pUnit = GetObjectByUniqueIdSafe<CCommonUnit>( nUnitID );

	if ( pUnit->GetState()->GetName() == EUSN_MOVE_BY_FORMATION )
	{
		CMoveByFormationState *pState = checked_cast<CMoveByFormationState *>( pUnit->GetState() );
		return pState->FinishState();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CGroupMoveExecutor::Init( const vector<CCommonUnit*> &_units, const struct SAIUnitCmd &cmd )
{
	vFinishPoint = cmd.vPos;

	vGroupCenter = CVec2( 0.0f, 0.0f );
	vFinishPathDirection = CVec2( 0.0f, 0.0f );
	vStartPathDirection = CVec2( 0.0f, 0.0f );
	nUnitsCount = 0;

	vCurrentFinishPoint = vFinishPoint;
	units.clear();
	nAtPlaceCount = 0;
	cells.clear();
	fMaxGroupSpeed = FP_MAX_VALUE;

	for ( vector<CCommonUnit*>::const_iterator it = _units.begin(); it != _units.end(); ++it )
	{
		CCommonUnit *pUnit = *it;
		vGroupCenter += pUnit->GetCenterPlain();
		units.push_back( SUnitListInfo( pUnit ) );
	}
	nUnitsCount = _units.size();

	if ( nUnitsCount > 1 )
	{
		eExecutorState = ES_BUILD_FORMATION;
		vGroupCenter /= nUnitsCount;

		if ( fabs( vGroupCenter - vFinishPoint ) > MIN_DISTANCE_FOR_FORMATION )
		{
			{
				STerrainModeSetter modeSetter( ELM_STATIC, GetTerrain() );

				const CCommonUnit *pUnit = units.front().pUnit;
				const EAIClasses aiClass = pUnit->GetAIPassabilityClass();
				const int nBoundTileRadius = pUnit->GetBoundTileRadius();
				const SVector vStartTile( GetAIMap()->GetTile( vGroupCenter ) );
				const SVector vFinishTile( GetAIMap()->GetTile( vFinishPoint ) );
				CCommonPathFinder *pPathFinder = Singleton<CCommonPathFinder>();
				//PATHFINDING{
				//pPathFinder->SetPathParameters( 1, aiClass, vGroupCenter, vFinishPoint, pUnit->GetProbablePlane( GetAIMap()->GetTile( vGroupCenter ) ) == PLANE_WATER, vStartTile );	
				pPathFinder->SetPathParameters( 1, aiClass, vGroupCenter, vFinishPoint, vStartTile, GetAIMap() );
				//PATHFINDING}
				CPtr<IStaticPath> pAveragePath = pPathFinder->CreatePath( true );

				if ( pAveragePath->GetLength() < TILES_FOR_PATH_DIRECTION )
					return false;

				pAveragePath->MoveStartTileTo( TILES_FOR_PATH_DIRECTION );
				const SVector vFinishGroupTile = pAveragePath->GetStartTile();
				const SVector vGroupDirection = vFinishGroupTile - vStartTile;
				vStartPathDirection = CVec2( vGroupDirection.x, vGroupDirection.y );

				pAveragePath->MoveFinishTileTo( pAveragePath->GetLength() - TILES_FOR_PATH_DIRECTION );
				const SVector vPreFinishTile = pAveragePath->GetFinishTile();
				const SVector vFinishDirection = vFinishTile - vPreFinishTile;
				vFinishPathDirection = CVec2( vFinishDirection.x, vFinishDirection.y );

				vCurrentFinishPoint = GetAIMap()->GetPointByTile( vFinishGroupTile );
			}

			Normalize( &vFinishPathDirection );
			Normalize( &vStartPathDirection );

			sort( units.begin(), units.end(), SUnitsSortByPriority( vGroupCenter, vStartPathDirection ) );

			CalcPositions( vFinishPathDirection );
			RecalcDestination();
			if ( !CheckPositions() )
				return false;

			int nPrevPriority = -1;
			CCellsMap::iterator pos = cells.end();
			for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )  
			{
				const int nPriority = it->pUnit->GetUnitPriority();
				if ( nPrevPriority != nPriority || pos == cells.end() )
				{
					pos = cells.find( nPriority );
					nPrevPriority = nPriority;
					fMaxGroupSpeed = Min( it->pUnit->GetMaxPossibleSpeed(), fMaxGroupSpeed );
				}

				it->pUnit->SetGroupShift( pos->second[ it->nCell ] );
			}
			nAtPlaceCount = 0;
			PeriodicalCheckForBuildFormation();
			theExecutorContainer.RemoveSleeping( this );
			return true;
		}
		else
			return false;
	}
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::CalcPositions( const CVec2 &vDirection )
{
	// подсчет количества юнитов в каждой приоритеной группе, предварительное формирование точек назначения
	cells.clear();
	const CVec2 vPerpDirection( vDirection.y, -vDirection.x );
	vector<int> priorityValues;
	float fGroupWidth = 0.0f;
	float fMaxHeight = 0.0f;
	int nMaxColumnSize = 1;
	CPriorityGroupMap priorityGroups;

	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		const int nPriority = it->pUnit->GetUnitPriority();
		CPriorityGroupMap::iterator pos = priorityGroups.find( nPriority );
		if ( pos == priorityGroups.end() )
		{
			priorityGroups[ nPriority ] = SPriorityGroupPosition( it->pUnit->GetAABBHalfSize().x*AABB_HALF_SIZE_X_MULTIPLIER, it->pUnit->GetAABBHalfSize().y*AABB_HALF_SIZE_Y_MULTIPLIER );
			pos = priorityGroups.find( nPriority );
			fMaxHeight = Max( pos->second.vUnitSize.x, fMaxHeight );
			priorityValues.push_back( nPriority );
		}
		else
			pos->second.nCount++;

		const int nOrder = (pos->second.nCount-1) % COLUMN_SIZE;
		if ( nOrder >= nMaxColumnSize )
			nMaxColumnSize = nOrder+1;
		float fYPosition = 0.0f;
		if ( nOrder == 0 )
		{
			fGroupWidth += pos->second.vUnitSize.y;
			if ( !pos->second.vCells.empty() )
				fYPosition = pos->second.vCells.back().y + pos->second.vUnitSize.y;
		}
		else
			fYPosition = pos->second.vCells.back().y;

		pos->second.vCells.push_back( CVec2( nOrder, fYPosition ) );
		pos->second.vUnits.push_back( it->pUnit );
	}
	// окончательное формирование точек назначения
	sort( priorityValues.begin(), priorityValues.end() );
	float fSubGroupPosition = -fGroupWidth/2;
	const CVec2 vUnitShift = fMaxHeight * vPerpDirection;
	const CVec2 vGroupShift = -0.5 * (fMaxHeight * nMaxColumnSize ) * vPerpDirection;
	for ( vector<int>::iterator it = priorityValues.begin(); it != priorityValues.end(); ++it )
	{
		vector<CVec2> vCells;
		CPriorityGroupMap::iterator pos = priorityGroups.find( *it );
		const float fSubGroupWidth = pos->second.vCells.back().y + pos->second.vUnitSize.y;
		fSubGroupPosition += fSubGroupWidth/2;
		const CVec2 vSubGroupDest = fSubGroupPosition * vDirection;
		for ( vector<CVec2>::iterator itUnitPos = pos->second.vCells.begin(); itUnitPos != pos->second.vCells.end(); ++itUnitPos )
		{
			const CVec2 vNewUnitPos( vSubGroupDest + vGroupShift + itUnitPos->x * vUnitShift - itUnitPos->y * vDirection );
			vCells.push_back( vNewUnitPos );
		}
		fSubGroupPosition += fSubGroupWidth/2;
		cells[*it] = vCells;
	}
	// все, структура CPriorityGroupMap заполнена
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CGroupMoveExecutor::CheckPositions() const
{
	// красивое распихивание юнитов по целевым точкам. во время движение может меняться
	CUnitsList::const_iterator itStart = units.begin();
	while ( itStart != units.end() )
	{
		const int nPriority = itStart->pUnit->GetUnitPriority();
		CUnitsList::const_iterator itEnd = itStart;
		for ( ; ( itEnd != units.end() && itEnd->pUnit->GetUnitPriority() == nPriority ); ++itEnd );

		CCellsMap::const_iterator pos = cells.find( nPriority );
		const int nSize = pos->second.size();

		for ( CUnitsList::const_iterator it = itStart; it != itEnd; ++it )
		{
			if ( GetTerrain()->CanUnitGo( it->pUnit->GetBoundTileRadius(), GetAIMap()->GetTile( it->pUnit->GetGroupShift() + vCurrentFinishPoint ), it->pUnit->GetAIPassabilityClass() ) == FREE_NONE )
				return false;
		}
		itStart = itEnd;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::RecalcDestination()
{
	// красивое распихивание юнитов по целевым точкам. во время движение может меняться
	CUnitsList::iterator itStart = units.begin();
	while ( itStart != units.end() )
	{
		const int nPriority = itStart->pUnit->GetUnitPriority();
		CUnitsList::iterator itEnd = itStart;
		for ( ; ( itEnd != units.end() && itEnd->pUnit->GetUnitPriority() == nPriority ); ++itEnd );

		CCellsMap::iterator pos = cells.find( nPriority );
		const int nSize = pos->second.size();

		CArray2D<float> distMatrix;
		distMatrix.SetSizes( nSize, nSize );
		{
			int i = 0;
			for ( CUnitsList::iterator it = itStart; it != itEnd; ++it )
			{
				for ( int j = 0; j < nSize; ++j )
					distMatrix[j][i] = fabs2( pos->second[j] + vCurrentFinishPoint - it->pUnit->GetCenterPlain() );
				i++;
			}
		}

		CHungarianMethod solve;
		solve.Init( distMatrix );
		solve.Solve();
		{
			int i = 0;
			for ( CUnitsList::iterator it = itStart; it != itEnd; ++it )
			{
				it->nCell = solve.GetResult( i );
				i++;
			}
		}
		itStart = itEnd;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::MoveToDestination()
{
	eExecutorState = ES_MOVE_TO_DESTINATION;
	vCurrentFinishPoint = vFinishPoint;
	CalcPositions( vStartPathDirection );
	RecalcDestination();
	int nPrevPriority = -1;
	CCellsMap::iterator pos = cells.end();
	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )  
	{
		const int nPriority = it->pUnit->GetUnitPriority();
		if ( nPrevPriority != nPriority || pos == cells.end() )
		{
			pos = cells.find( nPriority );
			nPrevPriority = nPriority;
		}

		it->pUnit->SetGroupShift( pos->second[ it->nCell ] );
		it->pUnit->SetDesirableSpeed( fMaxGroupSpeed );
		SendUnit( it->pUnit );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::PeriodicalCheckForBuildFormation()
{
	DeleteDeadUnits();
	if ( !units.empty() )
	{
		sort( units.begin(), units.end(), SUnitsSortByDirection( vGroupCenter, vStartPathDirection ) );
		const CVec2 vPerpPathDirection( vStartPathDirection.y, -vStartPathDirection.x );
		CUnitsList away_units;
		int nAwayUnitsCount = 0;
		{
			CUnitsList::iterator it = units.begin();
			if ( it->pUnit->IsIdle() && !it->bAtPlace )
				it->bCanGo = true;
			++it;
			while ( it != units.end() )
			{
				if ( it->pUnit->IsIdle() && !it->bAtPlace )  
				{
					const float fThisUnitShift = GetUnitPosition( it->pUnit, vGroupCenter, vPerpPathDirection );
					CUnitsList::iterator itFrwd = it;
					bool bLockFound = false;
					do 
					{
						--itFrwd;
						const float fFrwdUnitShift = GetUnitPosition( itFrwd->pUnit, vGroupCenter, vPerpPathDirection );
						if ( fabs( fFrwdUnitShift - fThisUnitShift ) < MAX_DIFF_BETWEEN_SHIFTS && fabs2( it->pUnit->GetCenter() - itFrwd->pUnit->GetCenter() ) < MAX_DIFF_BETWEEN_UNITS )
							bLockFound = true;
					} while( itFrwd != units.begin() && !bLockFound );
					if ( !bLockFound )
						it->bCanGo = true;
					else
					{
						it->bCanGo = false;
						if ( it->pUnit->GetUnitPriority() > itFrwd->pUnit->GetUnitPriority() )
							itFrwd->bGoAway = true;
					}
				}
				++it;
			}
		}

		for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
		{
			if ( it->bCanGo )
			{
				if ( it->bGoAway )
					SendUnitToPosition( it->pUnit, it->pUnit->GetCenterPlain() + vStartPathDirection*FORWARD_SHIFT );
				else
				{
					CUnitsList::iterator it2 = it;
					bool bCanGo = true;
					const int nPriority = it->pUnit->GetUnitPriority();
					const float fShift = GetUnitPosition( it->pUnit, vGroupCenter, vPerpPathDirection );
					++it2;
					while ( it2 != units.end() && bCanGo )
					{
						const int nPriority2 = it2->pUnit->GetUnitPriority();
						const float fShift2 = GetUnitPosition( it2->pUnit, vGroupCenter, vPerpPathDirection );
						if ( nPriority2 > nPriority && ( fabs( fShift - fShift2 ) < 3 * MAX_DIFF_BETWEEN_SHIFTS || abs( it->nCell%COLUMN_SIZE - it2->nCell%COLUMN_SIZE ) < 2 ) )
							bCanGo = false;

						++it2;
					}
					if ( bCanGo )
					{
						CVec2 vUnitShift = it->pUnit->GetGroupShift();
						SendUnitToPosition( it->pUnit, vCurrentFinishPoint + vUnitShift );
					}
				}
				it->bCanGo = false;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::PeriodicalCheckForMove()
{
	DeleteDeadUnits();
	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		if ( it->pUnit->GetSpeed() > fMaxGroupSpeed )
			it->pUnit->SetSpeed( ADJUST_SET, fMaxGroupSpeed );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::DeleteUnitFromList( const int nUnitID )
{
	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		if ( it->pUnit->GetUniqueID() == nUnitID )
		{
			if ( it->bAtPlace )
				nAtPlaceCount--;
			it->pUnit->Stop();
			nUnitsCount--;
			units.erase( it );
//			DebugTrace( "unit %d: deleted", nUnitID );
			return;
		}
	}
//	DebugTrace( "unit %d: no found", nUnitID );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGroupMoveExecutor::TryMoveUnit( const int nUnitID )
{
	sort( units.begin(), units.end(), SUnitsSortByPriority( vGroupCenter, vStartPathDirection ) );
	for ( CUnitsList::iterator it = units.begin(); it != units.end(); ++it )
	{
		if ( it->pUnit->GetUniqueID() == nUnitID )
		{
			const int nCell = it->nCell;
			const int nPriority = it->pUnit->GetUnitPriority();
			CCellsMap::iterator pos = cells.find( nPriority );
			if ( it->bGoAway )
			{
				it->bGoAway = false;
				return true;
			}
			else if ( nCell < COLUMN_SIZE || pos->second[ nCell - COLUMN_SIZE ] == CVec2( -1.0f, -1.0f ) )
			{
				if ( !it->bAtPlace )
				{
					pos->second[ nCell ] = CVec2( -1.0f, -1.0f );
					it->bAtPlace = true;
					it->bCanGo = false;
					it->bGoAway = false;

					nAtPlaceCount++;
					if ( nAtPlaceCount == nUnitsCount )
						MoveToDestination();
					return true;
				}
				else 
				{
					nAtPlaceCount--;
					nUnitsCount--;
					units.erase( it );
					return false;
				}
			}
			else
			{
				CUnitsList::iterator itFrwd = units.begin();
				for ( ; ( itFrwd != units.end() && itFrwd->pUnit->GetUnitPriority() != nPriority ); ++itFrwd );
				for ( ; ( itFrwd != units.end() && itFrwd->nCell != nCell - COLUMN_SIZE ); ++itFrwd );
				if ( itFrwd != units.end() )
				{
					const CVec2 vDestCellPoint = vCurrentFinishPoint + pos->second[ nCell-COLUMN_SIZE ];
					if ( fabs2( itFrwd->pUnit->GetCenterPlain() - vDestCellPoint ) < fabs2( it->pUnit->GetCenterPlain() - vDestCellPoint ) )
					{
						if ( !it->bAtPlace )
						{
							pos->second[ nCell ] = CVec2( -1.0f, -1.0f );
							it->bAtPlace = true;
							it->bCanGo = false;
							it->bGoAway = false;

							nAtPlaceCount++;
							if ( nAtPlaceCount == nUnitsCount )
								MoveToDestination();
						}
						return true;
					}
					itFrwd->nCell = nCell;
					itFrwd->pUnit->SetGroupShift( pos->second[ itFrwd->nCell ] );
					SendUnitToPosition( itFrwd->pUnit, vCurrentFinishPoint + pos->second[ itFrwd->nCell ] );
				}
				it->nCell = nCell - COLUMN_SIZE;
				it->pUnit->SetGroupShift( pos->second[ it->nCell ] );
				SendUnitToPosition( it->pUnit, vCurrentFinishPoint + pos->second[ it->nCell ] );
				return true;
			}
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CGroupMoveExecutor::Segment()
{
	if ( units.empty() )
	{
//		DebugTrace( "CGroupMoveExecutor::Segment() returns -1, executor's id: %d", GetInstanceID() );
		return -1;
	}

	switch( eExecutorState ) 
	{
	case ES_BUILD_FORMATION:
		PeriodicalCheckForBuildFormation();
		break;
	case ES_MOVE_TO_DESTINATION:
		PeriodicalCheckForMove();
		break;
	}

	return SEGMENT_PERIOD_CHECK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CGroupMoveExecutor::RegisterOnEvents( IExecutorContainer *pContainer )
{
	SExecutorEventParam par1( EID_START_IDLE, GetInstanceID(), 0 );
	pContainer->RegisterOnEvent( this, par1 );

	SExecutorEventParam par2( EID_TERMINATE_STATE, GetInstanceID(), 0 );
	pContainer->RegisterOnEvent( this, par2 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CGroupMoveExecutor::NotifyEvent( const CExecutorEvent &_event )
{
	const CGroupMoveExecutorEvent *event = static_cast<const CGroupMoveExecutorEvent *>( &_event );
	const SExecutorEventParam param = event->GetParam();
	if ( param.eEventID == EID_START_IDLE )
	{
		switch( eExecutorState ) 
		{
		case ES_MOVE_TO_DESTINATION:
		case ES_DEFAULT_MOVE:
			DeleteUnitFromList( event->GetUnitID() );
			FinishState( event->GetUnitID() );
			break;
		case ES_BUILD_FORMATION:
			if ( !TryMoveUnit( event->GetUnitID() ) )
			{
				DeleteUnitFromList( event->GetUnitID() );
				FinishState( event->GetUnitID() );
			}
			break;
		}
	}
	else
		DeleteUnitFromList( event->GetUnitID() );

	return ( units.empty() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
START_REGISTER( GroupMoveExecutorVars )
REGISTER_VAR_EX( "aabb_half_size_x_multiplier", NGlobal::VarFloatHandler, &AABB_HALF_SIZE_X_MULTIPLIER, 7.0f, STORAGE_NONE );
REGISTER_VAR_EX( "aabb_half_size_y_multiplier", NGlobal::VarFloatHandler, &AABB_HALF_SIZE_Y_MULTIPLIER, 3.5f, STORAGE_NONE );
REGISTER_VAR_EX( "max_diff_between_shifts", NGlobal::VarFloatHandler, &MAX_DIFF_BETWEEN_SHIFTS, 100.0f, STORAGE_NONE );
REGISTER_VAR_EX( "max_diff_between_units", NGlobal::VarFloatHandler, &MAX_DIFF_BETWEEN_UNITS, 200000.0f, STORAGE_NONE );
REGISTER_VAR_EX( "forward_shift", NGlobal::VarFloatHandler, &FORWARD_SHIFT, 256.0f, STORAGE_NONE );
REGISTER_VAR_EX( "min_distance_for_formation", NGlobal::VarFloatHandler, &MIN_DISTANCE_FOR_FORMATION, 2560.0f, STORAGE_NONE );
REGISTER_VAR_EX( "column_size", NGlobal::VarIntHandler, &COLUMN_SIZE, 5, STORAGE_NONE );
REGISTER_VAR_EX( "segment_period_check", NGlobal::VarIntHandler, &SEGMENT_PERIOD_CHECK, 20, STORAGE_NONE );
REGISTER_VAR_EX( "tiles_for_path_direction", NGlobal::VarIntHandler, &TILES_FOR_PATH_DIRECTION, 48, STORAGE_NONE );
FINISH_REGISTER
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
