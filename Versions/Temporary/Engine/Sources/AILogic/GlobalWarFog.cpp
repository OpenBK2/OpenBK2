#include "stdafx.h"

#include "Cheats.h"
#include "Diplomacy.h"
#include "GlobalWarFog.h"
#include "Graveyard.h"
#include "NewUpdater.h"

#include "Spiral.hpp"
#include "Tracer.hpp"
#include "Visibility.hpp"

#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/StaticMapHeights.h"
//#include "../Image/Image.h"
//#include "../Image/ImageTGA.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "System/Commands.h"

#include "AILogic_export.h"

#include <cstdint>

static const int MAX_UPDATE_AT_SEGMENT = 100;
static const int MAX_AREAS_CALCULATED_AT_SEGMENT = 50;
static const int MAX_SMOOTH_TILES_AT_SEGMENT = 1000;
static const int HEIGHT_MULTIPLYER = 100;

static int CAMOUFLAGE_RADIUS = 8;

extern CEventUpdater updater;
extern CDiplomacy theDipl;

extern CDiplomacy theDipl;
extern SCheats theCheats;
extern CGraveyard theGraveyard;
CGlobalWarFog theWarFog;

// for warfog calculation
static CArray2D1Bit mask;
static CArray2D1Bit visit;

struct SMaskVisitor
{
	int nMaxRadius;
	SMaskVisitor() : nMaxRadius( theWarFog.GetMaxRadius() ) {}
	
	bool operator()( const SVector &vOffset, const bool bVisible )
	{
		const int nX = vOffset.x + nMaxRadius;
		const int nY = vOffset.y + nMaxRadius;

		if ( !bVisible )
			mask.RemoveData( nX, nY );
		else if ( !visit.GetData( nX, nY ) )
			mask.SetData( nX, nY );
		
		visit.SetData( nX, nY );
		return true;
	}
	const bool GetReturnValue() const { return true; }
};

struct SGetLastVisitor
{
	bool bLastVisit;
	SGetLastVisitor() : bLastVisit( false ) {}

	bool operator()( const SVector &vOffset, const bool bVisible )
	{
		bLastVisit = bVisible;
		return true;
	}
	const bool GetReturnValue() const { return bLastVisit; }
};

CGlobalWarFog::SWarForFullUnitInfo::SWarForFullUnitInfo( const SWarFogUnitInfo &unitInfo, const int _nParty, const int nSpiralLength ) : SWarFogUnitInfo( unitInfo ),
	vOldPos( unitInfo.vPos ), nOldRadius( unitInfo.nRadius ), nParty( _nParty ), updateFlag( UPD_CREATE_NEW_UNIT )
{
	visValues.SetSize( nSpiralLength );
	visValues.FillZero();
}

void CGlobalWarFog::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() )
		ReInit();
}

void CGlobalWarFog::ReInit()
{
	miniMapWarFog.SetSizes( GetSizeX(), GetSizeY() );
	miniMapWarFog.FillZero();
	//miniMapSmoothTiles.clear();

	valid.SetSizes( GetSizeX(), GetSizeY() );
	valid.FillZero();

	calced.SetSizes( GetSizeX(), GetSizeY() );
	calced.FillZero();
	nAreasCalced = 0;

	heights.SetSizes( GetSizeX(), GetSizeY() );
	heights.FillZero();

	mask.SetSizes( 2 * GetMaxRadius() + 1, 2 * GetMaxRadius() + 1 );
	visit.SetSizes( 2 * GetMaxRadius() + 1, 2 * GetMaxRadius() + 1 );

	bInitialization = true;
	bHasInvalidTile = true;

	bNeedFullCalc = true;
	nLastFogCalcTime = 0;
	miniMapSums.resize( GetSizeX(), 0 );
	miniMapWarFog.FillZero();

	GenerateSpiral( spiral, spiralCoords, lengths, nMaxRadius );

	areas.SetSizes( GetSizeX(), GetSizeY() );
	for ( int y = 0; y < GetSizeY(); ++y )
	{
		for ( int x = 0; x < GetSizeX(); ++x )
		{
			areas[y][x].SetSize( GetSpiralLength() );
			areas[y][x].FillZero();
		}
	}
	/*
	bFullSmooth = true;
	bMiniMapWarFogReady = false;
	cMiniMapWarFogParty = 0xFF;
	*/
}

void CGlobalWarFog::Init( const int _nSizeX, const int _nSizeY, const int _nMaxRadius, const float _fUnitHeight )
{
	nUnitHeight = _fUnitHeight * HEIGHT_MULTIPLYER;
	nMaxRadius = _nMaxRadius/AI_TILES_IN_VIS_TILE;

	nSizeX = _nSizeX/AI_TILES_IN_VIS_TILE;
	nSizeY = _nSizeY/AI_TILES_IN_VIS_TILE;

	warFog.resize( 2 ); // only two parties, see comment for CGlobalWarFog::SWarForFullUnitInfo.nParty !!!
	for ( int i = 0; i < warFog.size(); ++i )
		warFog[i].SetSizes( GetSizeX(), GetSizeY() );

	areasOpenTiles.SetSizes( GetSizeX() * AI_TILES_IN_VIS_TILE, GetSizeY() * AI_TILES_IN_VIS_TILE );
	areasOpenTiles.FillZero();

	staticObjects.SetSizes( GetSizeX(), GetSizeY() );
	staticObjects.FillEvery( -1 );

	updateUnitList.Init( GetMaxRadius() );

	ReInit();
}

void CGlobalWarFog::Clear()
{
	spiral.clear();
	spiralCoords.Clear();
	lengths.clear();
	miniMapWarFog.Clear();
	valid.Clear();
	areas.Clear();
	areas.Clear();
	warFog.clear();
	areasOpenTiles.Clear();
	staticObjects.Clear();
	updateUnitList.Clear();
	mask.Clear();
	visit.Clear();
	units.clear();
}

void CGlobalWarFog::RecalculateArea( const SVector &vTile )
{
	if ( !calced.GetData( vTile.x, vTile.y ) || nAreasCalced < MAX_AREAS_CALCULATED_AT_SEGMENT )
	{
		mask.FillZero();
		visit.FillZero();

		CWarFogTracerInternal<SMaskVisitor> areaTracer( this, SMaskVisitor() );
		areaTracer.TraceCircle( vTile, GetMaxRadius() );
		mask.SetData( GetMaxRadius(), GetMaxRadius() );
		areas[vTile.y][vTile.x].FillZero();
		for ( int i = 0; i < GetSpiralLength(); ++i )
		{
			if ( mask.GetData( GetSpiralPoint( i ).vOffset.x + GetMaxRadius(), GetSpiralPoint( i ).vOffset.y + GetMaxRadius() ) )
				areas[vTile.y][vTile.x].SetData( i );
		}

		valid.SetData( vTile.x, vTile.y );

		calced.SetData( vTile.x, vTile.y );
		++nAreasCalced;
	}
}

void CGlobalWarFog::RemoveTileValid( const int x, const int y )
{
	if ( !bInitialization )
	{
		for ( int i = 0; i < GetSpiralLength(); ++i ) 
		{
			const int x1 = GetSpiralPoint( i ).vOffset.x + x;
			const int y1 = GetSpiralPoint( i ).vOffset.y + y;
			if ( IsTileInside( x1, y1 ) )
				valid.RemoveData( x1, y1 );
		}
		bHasInvalidTile = true;
	}
}

void CGlobalWarFog::SetTileHeight( const int x, const int y, const float fHeight )
{
	NI_VERIFY( IsTileInside( x, y ), fmt::format( "Tile ({} x {}) outside heights map", x, y ), return );

	heights[y][x] = fHeight * HEIGHT_MULTIPLYER;
	RemoveTileValid( x, y );
}

void CGlobalWarFog::AddUnit( const int nID, const SWarFogUnitInfo &updateInfo, const int nParty )
{
	NI_ASSERT( updateInfo.bPlane || IsTileInside( updateInfo.vPos ), fmt::format( "Trying add unit {} with wrong position ({} x {}), size of map: {} x {}", nID, updateInfo.vPos.x, updateInfo.vPos.y, GetSizeX(), GetSizeY() ) );
	//NI_ASSERT( updateInfo.nRadius <= GetMaxRadius(), fmt::format( "WARNING: nMaxRadius exceed (nMaxRadius = {}, updateInfo.nRadius = {})", GetMaxRadius(), updateInfo.nRadius ) );
	NI_ASSERT( units.find( nID ) == units.end(), fmt::format( "Unit with id = {} already exists in warfog.", nID ) );

	if ( !IsValidParty( nParty ) )
		return;

	units[nID] = SWarForFullUnitInfo( updateInfo, nParty, GetSpiralLength() );
	units[nID].nRadius = (std::min)( units[nID].nRadius, GetMaxRadius() );
	updateUnitList.Push( nID, UPD_CREATE_NEW_UNIT, 0.0f );
}

void CGlobalWarFog::DeleteUnit( const int nID )
{
	TWarFogUnitsMap::iterator pos = units.find( nID );
	if ( pos != units.end() )
	{
		if ( pos->second.updateFlag == UPD_CREATE_NEW_UNIT )
			units.erase( pos );
		else
		{
			pos->second.updateFlag = UPD_DELETE_UNIT;
			updateUnitList.Push( nID, UPD_DELETE_UNIT, 0.0f );
		}
	}
}

void CGlobalWarFog::UpdateUnit( const int nID, const SWarFogUnitInfo &updateInfo )
{
	//NI_ASSERT( updateInfo.bPlane || IsTileInside( updateInfo.vPos ), StrFmt( "Trying update unit %d to wrong position (%d x %d), size of map: %d x %d", nID, updateInfo.vPos.x, updateInfo.vPos.y, GetSizeX(), GetSizeY() ) );
	//NI_ASSERT( updateInfo.nRadius <= GetMaxRadius(), StrFmt( "WARNING: nMaxRadius exceed (nMaxRadius = %d, updateInfo.nRadius = %d)", GetMaxRadius(), updateInfo.nRadius ) );
	
	TWarFogUnitsMap::iterator pos = units.find( nID );
#ifdef _FINALRELEASE
	if ( pos == units.end() )
		DebugTrace( "CCommonWarFog::UpdateUnit( %d ) : unit not found in warfog !!!", nID );
#endif

	if ( pos != units.end() && pos->second.updateFlag != UPD_DELETE_UNIT )
	{
		if ( pos->second.nRadius != updateInfo.nRadius || pos->second.sector != updateInfo.sector )
		{
			if ( pos->second.updateFlag == UPD_UPDATED )
				pos->second.updateFlag = UPD_UPDATE_PROPERTIES;

			pos->second.nRadius = (std::min)( updateInfo.nRadius, GetMaxRadius() );
			pos->second.sector = updateInfo.sector;
			pos->second.vPos = updateInfo.vPos;
			updateUnitList.Push( nID, pos->second.updateFlag, mDistance( pos->second.vOldPos, pos->second.vPos ) );
		}
		else if ( mDistance( updateInfo.vPos, pos->second.vOldPos ) >= 1 )
		{
			if ( pos->second.updateFlag == UPD_UPDATED )
				pos->second.updateFlag = UPD_UPDATE_POSITION;

			pos->second.nRadius = (std::min)( updateInfo.nRadius, GetMaxRadius() );
			pos->second.sector = updateInfo.sector;
			pos->second.vPos = updateInfo.vPos;
			updateUnitList.Push( nID, pos->second.updateFlag, mDistance( pos->second.vOldPos, pos->second.vPos ) );
		}
	}
}

void CGlobalWarFog::ChangeParty( const int nID, const SWarFogUnitInfo &unitInfo, const int nNewParty )
{
	TWarFogUnitsMap::iterator pos = units.find( nID );
	if ( pos == units.end() )
	{
		if ( IsValidParty( nNewParty ) )
			AddUnit( nID, unitInfo, nNewParty );
	}
	else if ( pos->second.updateFlag != UPD_DELETE_UNIT && pos->second.nParty != nNewParty )
	{
		if ( !IsValidParty( nNewParty ) )
			DeleteUnit( nID );
		else if ( pos->second.updateFlag == UPD_CREATE_NEW_UNIT )
			pos->second.nParty = nNewParty;
		else
		{
			pos->second.updateFlag = UPD_CHANGE_PARTY;
			updateUnitList.Push( nID, pos->second.updateFlag, 0.0f );
		}
	}
}

void CGlobalWarFog::SynchronizeHeights( const SVector &vUpLeftTile, const SVector &vBottomRightTile )
{
	const int nX1 = vUpLeftTile.x;
	const int nY1 = vUpLeftTile.y;
	const int nX2 = vBottomRightTile.x;
	const int nY2 = vBottomRightTile.y;
	
	for ( int y = nY1; y < nY2-1; ++y )
	{
		for ( int x = nX1; x < nX2-1; ++x )
		{
			SetTileHeight( x, y, ::GetHeights()->GetTileHeight( x * AI_TILES_IN_VIS_TILE, y * AI_TILES_IN_VIS_TILE ) );
			staticObjects[y][x] = -1;
		}
	}
}

const CArray1Bit &CGlobalWarFog::GetVisibleInfoForTile( const SVector &tile )
{
	if ( !IsTileValid( tile ) )
		RecalculateArea( tile );

	return areas[tile.y][tile.x];
}

void CGlobalWarFog::OnTileChangeVisibility( const SVector &vTile, const bool bVisible, const int nParty )
{
	if ( bVisible )
		updater.TileBecameVisibleFromWarFog( vTile, nParty );
}

void CGlobalWarFog::AddVisibleTiles( const int nID, SWarForFullUnitInfo &unitInfo )
{
	if ( !unitInfo.bPlane )
	{
		unitInfo.vPos.x = Clamp( unitInfo.vPos.x, 0, GetSizeX() - 1 );
		unitInfo.vPos.y = Clamp( unitInfo.vPos.y, 0, GetSizeY() - 1 );
	}

	TWarFog &thisPartyWarFog = warFog[unitInfo.nParty];

	CPtr<CWarFogVisibility> pVisibility = CreateWarFogVisibility( this, unitInfo );

	const int nClsLength = lengths[ (std::min)( unitInfo.nRadius, CAMOUFLAGE_RADIUS ) ];
	for ( int i = 0; i < nClsLength; ++i )
	{
		const SVector vTile = unitInfo.vPos + GetSpiralPoint( i ).vOffset;
		if ( pVisibility->IsVisible( vTile, i ) )
		{
			SWarFogTileInfo &thisTileWarFog = thisPartyWarFog[vTile.y][vTile.x];
			if ( ++thisTileWarFog.nVisible == 1 )
				OnTileChangeVisibility( vTile, true, unitInfo.nParty );
			++thisTileWarFog.nCloseVisible;
			unitInfo.visValues.SetData( i );
		}
		else
			unitInfo.visValues.RemoveData( i );
	}

	const int nLength = lengths[ unitInfo.nRadius ];
	for ( int i = nClsLength; i < nLength; ++i )
	{
		const SVector vTile = unitInfo.vPos + GetSpiralPoint( i ).vOffset;
		if ( pVisibility->IsVisible( vTile, i ) )
		{
			SWarFogTileInfo &thisTileWarFog = thisPartyWarFog[vTile.y][vTile.x];
			if ( ++thisTileWarFog.nVisible == 1 )
				OnTileChangeVisibility( vTile, true, unitInfo.nParty );
			unitInfo.visValues.SetData( i );
		}
		else
			unitInfo.visValues.RemoveData( i );
	}

	unitInfo.Update();
}

void CGlobalWarFog::RemoveVisibleTiles( const int nID, SWarForFullUnitInfo &unitInfo )
{
	TWarFog &thisPartyWarFog = warFog[unitInfo.nParty];

	const int nClsOldLength = GetSpiralLength( (std::min)( unitInfo.nOldRadius, CAMOUFLAGE_RADIUS ) );
	for ( int i = 0; i < nClsOldLength; ++i )
	{
		if ( unitInfo.visValues.GetData( i ) )
		{
			const SVector vTile = unitInfo.vOldPos + GetSpiralPoint( i ).vOffset;
			SWarFogTileInfo &thisTileWarFog = thisPartyWarFog[vTile.y][vTile.x];
			if ( --thisTileWarFog.nVisible == 0 )
				OnTileChangeVisibility( vTile, false, unitInfo.nParty );
			--thisTileWarFog.nCloseVisible;
		}
	}
	const int nOldLength = GetSpiralLength( unitInfo.nOldRadius );
	for ( int i = nClsOldLength; i < nOldLength; ++i )
	{
		if ( unitInfo.visValues.GetData( i ) )
		{
			const SVector vTile = unitInfo.vOldPos + GetSpiralPoint( i ).vOffset;
			SWarFogTileInfo &thisTileWarFog = thisPartyWarFog[vTile.y][vTile.x];
			if ( --thisTileWarFog.nVisible == 0 )
				OnTileChangeVisibility( vTile, false, unitInfo.nParty );
		}
	}
}

void CGlobalWarFog::Segment()
{
	// update invalid tiles
	if ( bHasInvalidTile )
	{
		for ( TWarFogUnitsMap::iterator it = units.begin(); it != units.end(); ++it )
		{
			if ( IsTileInside( it->second.vPos ) && !IsTileValid( it->second.vPos ) )
			{
				if ( it->second.updateFlag == UPD_UPDATED )
					it->second.updateFlag = UPD_UPDATE_VISIBILITY;
				updateUnitList.Push( it->first, UPD_UPDATE_VISIBILITY, 0.0f );
			}
		}
		bHasInvalidTile = false;
	}

	// update units
	for ( int i = 0; i < MAX_UPDATE_AT_SEGMENT; ++i )
	{
		TWarFogUnitsMap::iterator pos = units.find( updateUnitList.Pop() );
		if ( pos == units.end() )
			break;
    else if ( pos->second.updateFlag != UPD_UPDATED )
		{
			switch ( pos->second.updateFlag )
			{
			case UPD_DELETE_UNIT:
				RemoveVisibleTiles( pos->first, pos->second );
				units.erase( pos );
				break;
			case UPD_CREATE_NEW_UNIT:
				AddVisibleTiles( pos->first, pos->second );
				break;
			case UPD_CHANGE_PARTY:
				RemoveVisibleTiles( pos->first, pos->second );
				pos->second.nParty = 1 - pos->second.nParty;
				AddVisibleTiles( pos->first, pos->second );
				break;
			default:
				RemoveVisibleTiles( pos->first, pos->second );
				AddVisibleTiles( pos->first, pos->second );
			}
		}
	}

	nAreasCalced = 0;
}

bool CGlobalWarFog::IsTileVisible( const SVector &tile, const int nParty ) const
{
	if ( theCheats.GetTurnOffWarFog() )
		return true;
	if ( theDipl.GetNeutralParty() == nParty || !GetAIMap()->IsTileInside( tile ) )
		return false;
	if ( !theDipl.IsNetGame() && IsOpenBySriptArea( tile ) && theDipl.GetMyParty() == nParty )
		return true;

	return warFog[nParty][tile.y/AI_TILES_IN_VIS_TILE][tile.x/AI_TILES_IN_VIS_TILE].nVisible != 0; 
}

bool CGlobalWarFog::IsUnitVisible( const SVector &tile, const int nParty, bool bCamouflated ) const
{
	if ( theCheats.GetTurnOffWarFog() )
		return true;
	if ( theDipl.GetNeutralParty() == nParty || !GetAIMap()->IsTileInside( tile ) )
		return false;
	if ( !theDipl.IsNetGame() && IsOpenBySriptArea( tile ) && theDipl.GetMyParty() == nParty )
		return true;

	if ( bCamouflated )
		return warFog[nParty][tile.y/AI_TILES_IN_VIS_TILE][tile.x/AI_TILES_IN_VIS_TILE].nCloseVisible != 0; 
	else
		return warFog[nParty][tile.y/AI_TILES_IN_VIS_TILE][tile.x/AI_TILES_IN_VIS_TILE].nVisible != 0; 
}

void CGlobalWarFog::ToggleOpenForScriptAreaTiles( const NDb::SScriptArea &scriptArea, bool bOpen )
{
	if ( bOpen == ( scriptAreas.find( scriptArea.szName ) == scriptAreas.end() ) )
	{
		if ( bOpen )
			scriptAreas.insert( scriptArea.szName );
		else
			scriptAreas.erase( scriptArea.szName );

		std::vector<SVector> tiles;
		if ( scriptArea.eType == EAT_RECTANGLE )
		{
			SRect rect;
			rect.InitRect( scriptArea.vCenter, CVec2( 1.0f, 0.0f ), scriptArea.vAABBHalfSize.x, scriptArea.vAABBHalfSize.y );
			GetAIMap()->GetTilesCoveredByRect( rect, &tiles );
		}
		else
			GetAIMap()->GetTilesCoveredByLargeCircle( scriptArea.vCenter, scriptArea.fR, &tiles );

		if ( bOpen )
		{
			for ( std::vector<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
				areasOpenTiles.SetData( it->x, it->y );
		}
		else
		{
			for ( std::vector<SVector>::const_iterator it = tiles.begin(); it != tiles.end(); ++it )
				areasOpenTiles.RemoveData( it->x, it->y );
		}
	}
}

void CGlobalWarFog::AddStaticObjectTile( const SVector &vTile, const int nObjectID, const float fHeight )
{
	const SVector vVisTile( vTile.x/AI_TILES_IN_VIS_TILE, vTile.y/AI_TILES_IN_VIS_TILE );
	if ( GetStaticObjectAtTile( vVisTile ) == -1 )
	{
		SetTileHeight( vVisTile, fHeight + ::GetHeights()->GetTileHeight( vTile.x, vTile.y ) );
		staticObjects[vVisTile.y][vVisTile.x] = nObjectID;
	}
}

void CGlobalWarFog::RemoveStaticObjectTile( const SVector &vTile, const int nObjectID )
{
	const SVector vVisTile( vTile.x/AI_TILES_IN_VIS_TILE, vTile.y/AI_TILES_IN_VIS_TILE );
	if ( GetStaticObjectAtTile( vVisTile ) == nObjectID )
	{
		SetTileHeight( vVisTile, ::GetHeights()->GetTileHeight( vTile.x, vTile.y ) );
		staticObjects[vVisTile.y][vVisTile.x] = -1;
	}
}

void CGlobalWarFog::ReplaceStaticObjects( const int nNewID, const det_set<int> &oldIDs )
{
	for ( int x = 0; x < GetSizeX(); ++x )
	{
		for ( int y = 0; y < GetSizeY(); ++y )
		{
			if ( oldIDs.find( staticObjects[y][x] ) != oldIDs.end() )
				staticObjects[y][x] = nNewID;
		}
	}
}

bool CGlobalWarFog::IsTraceable( const SVector &tile1, const SVector &tile2 )
{
	if ( !IsTileInside( tile1 ) || !IsTileInside( tile2 ) )
		return false;
	const SVector vOffset = tile2 - tile1;
	if ( abs( vOffset.x ) < 2 && abs( vOffset.y ) < 2 )
		return true;
	else if ( abs( vOffset.x ) > GetMaxRadius() || abs( vOffset.y ) > GetMaxRadius() )
	{
		CWarFogTracerInternal<SGetLastVisitor> traceableTracer( this, SGetLastVisitor() );
		return traceableTracer.TraceRay( tile1, vOffset );
	}
	else
	{
		const int nSpiralPointIndex = spiralCoords[vOffset.y + GetMaxRadius() ][vOffset.x + GetMaxRadius()];
		if ( nSpiralPointIndex == -1 )
		{
			CWarFogTracerInternal<SGetLastVisitor> traceableTracer( this, SGetLastVisitor() );
			return traceableTracer.TraceRay( tile1, vOffset );
		}
		else
		{
			if ( !IsTileValid( tile1 ) )
				RecalculateArea( tile1 );
			return areas[tile1.y][tile1.x].GetData( nSpiralPointIndex );
		}
	}
}

bool CGlobalWarFog::GetWarForInfo( CArray2D<uint8_t> **pWarFogInfo, const int nParty, const bool bFirstTime )
{
	*pWarFogInfo = &miniMapWarFog;
	const NTimer::STime nFogCalcTime = GameTimer()->GetGameTime();
	const int nShift = (bNeedFullCalc || bFirstTime) ? ( miniMapWarFog.GetSizeY() ) : ( miniMapWarFog.GetSizeY() * ( nFogCalcTime - nLastFogCalcTime ) / SConsts::WAR_FOG_FULL_UPDATE + 1);
	if ( bFirstTime )
		nMiniMapY = 0;
	nLastFogCalcTime = nFogCalcTime;
	bNeedFullCalc = false;
	const int nLastY = (std::min)( nMiniMapY + nShift, miniMapWarFog.GetSizeY() );
	for ( int y = nMiniMapY; y < nLastY; ++y )
	{
		if ( y == 0 )
		{
			miniMapSums[0] = GetClientTileVis( 0, AI_TILES_IN_VIS_TILE, nParty ) + GetClientTileVis( AI_TILES_IN_VIS_TILE, AI_TILES_IN_VIS_TILE, nParty );
			miniMapSums[1] = miniMapSums[0];
			miniMapWarFog[0][0] = ( GetClientTileVis( 0, 0, nParty ) + GetClientTileVis( AI_TILES_IN_VIS_TILE, 0, nParty ) +
				miniMapSums[0] ) / 4;
			miniMapWarFog[0][1] = miniMapWarFog[0][0];
			miniMapWarFog[1][0] = miniMapWarFog[0][0];
			miniMapWarFog[1][1] = miniMapWarFog[0][0];
			for ( int x = 2; x < miniMapWarFog.GetSizeX(); ++x )
			{
				miniMapSums[x] = GetClientTileVis( x*AI_TILES_IN_VIS_TILE, AI_TILES_IN_VIS_TILE, nParty ) + GetClientTileVis( (x - 1)*AI_TILES_IN_VIS_TILE, AI_TILES_IN_VIS_TILE, nParty );
				miniMapWarFog[0][x] = ( GetClientTileVis( x*AI_TILES_IN_VIS_TILE, 0, nParty ) + GetClientTileVis( (x - 1)*AI_TILES_IN_VIS_TILE, 0, nParty ) +
					miniMapSums[x] ) / 4;
				miniMapWarFog[1][x] = miniMapWarFog[0][x];
			}
		}
		else if ( y > 1 ) 
		{
			int nNewSum;
			nNewSum = GetClientTileVis( 0, y*AI_TILES_IN_VIS_TILE, nParty ) + GetClientTileVis( AI_TILES_IN_VIS_TILE, y*AI_TILES_IN_VIS_TILE, nParty );
			miniMapWarFog[y][0] = ( miniMapSums[0] + nNewSum ) / 4;
			miniMapWarFog[y][1] = ( miniMapSums[0] + nNewSum ) / 4;
			miniMapSums[0] = nNewSum;
			miniMapSums[1] = miniMapSums[0];
			for ( int x = 2; x < miniMapWarFog.GetSizeX(); ++x )
			{
				nNewSum = GetClientTileVis( x*AI_TILES_IN_VIS_TILE, y*AI_TILES_IN_VIS_TILE, nParty ) + GetClientTileVis( (x - 1)*AI_TILES_IN_VIS_TILE, y*AI_TILES_IN_VIS_TILE, nParty );
				miniMapWarFog[y][x] = ( miniMapSums[x] + nNewSum ) / 4;
				miniMapSums[x] = nNewSum;
			}
		}
	}
	nMiniMapY = nLastY;
	if ( nMiniMapY == miniMapWarFog.GetSizeY() )
		nMiniMapY = 0;
	return nMiniMapY == 0;
}

void CGlobalWarFog::DumpWarFog()
{
	int nMinHeight = heights[0][0];
	int nMaxHeight = heights[0][0];
	for ( int x = 0; x < GetSizeX(); ++x )
		for ( int y = 0; y < GetSizeY(); ++y )
		{
			nMinHeight = (std::min)( nMinHeight, heights[y][x] );
			nMaxHeight = (std::max)( nMaxHeight, heights[y][x] );
		}

	DebugTrace( "Min height = %d, Max height = %d", nMinHeight, nMaxHeight );
/*
	CArray2D<uint32_t> image;
	const float fDeltaHeight = 255/(float)( nMaxHeight - nMinHeight );
	image.SetSizes( heights.GetSizeX(), heights.GetSizeY() );
	for ( int x = 0;  x < heights.GetSizeX(); ++x )
		for ( int y = 0; y < heights.GetSizeY(); ++y )
			image[y][x] = NImage::SColor( 255, heights[y][x]*fDeltaHeight, heights[y][x]*fDeltaHeight, heights[y][x]*fDeltaHeight );

	CFileStream imageStream( "debug_images\\warfog\\heights.tga", CFileStream::WIN_CREATE );
	NImage::SaveImageAsTGA( &imageStream, image );
*/
}

static void DumpWarFog( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	theWarFog.DumpWarFog();
}

START_REGISTER( WarFogCheckers )

REGISTER_VAR_EX( "AI.WarFog.CamouflageOpenRadius", NGlobal::VarIntHandler, &CAMOUFLAGE_RADIUS, 8, STORAGE_NONE );
REGISTER_CMD( "warfog_dump", DumpWarFog );

FINISH_REGISTER

REGISTER_SAVELOAD_CLASS( AILOGIC, 0x31261B40, CGlobalWarFog );


