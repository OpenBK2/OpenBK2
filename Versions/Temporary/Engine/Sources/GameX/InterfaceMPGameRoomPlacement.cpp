#include "stdafx.h"

#include "CoopPlayerInfo.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "InterfaceMPGameRoom.h"
#include "Misc/StrProc.h"
#include "MPCoopRules.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "UI/DBUserInterface.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
void ShowFlagLayer( IWindow *pFlag, const char *pszLayer, bool bShow )
{
	if ( IWindow *pLayer = pFlag->GetChild( pszLayer, false ) )
	{
		pLayer->ShowWindow( bShow );
	}
}
} // namespace

void CInterfaceMPGameRoom::InitPlacementControls()
{
	pMinimap = dynamic_cast<IMiniMap *>( pMain->GetChild( "Minimap", true ) );
	const NDb::SMapInfo *pMap = gameDesc.pMPMap ? gameDesc.pMPMap->pMap.GetPtr() : nullptr;
	if ( !pMinimap || !pMap || pMap->nNumPatchesX <= 0 || pMap->nNumPatchesY <= 0 )
	{
		return;
	}
	pMinimap->SetLoadingMapParams( pMap->nNumPatchesX * AI_TILES_IN_PATCH * AI_TILE_SIZE,
	                               pMap->nNumPatchesY * AI_TILES_IN_PATCH * AI_TILE_SIZE );
	pMinimap->SetMaterial( pMap->pMiniMap );
	pMinimap->ShowWindow( true );

	// The surround paints a foreground over its children. Put the overlay beside it in the info panel.
	IWindow *pSurround = pMinimap->GetParentWindow();
	pPlacementPanel = pSurround && pSurround != pMain ? pSurround->GetParentWindow() : pMain.GetPtr();
	const int nStarts = NGameX::GetManualPlacementSize( gameDesc );
	if ( !pPlacementPanel || !nStarts )
	{
		return;
	}
	const auto *pFlagDesc =
			NDb::Get<NDb::SWindowSimple>( CDBID( "UI/Game/Menu/MPGameRoom/Placement/Flag_WindowSimple.xdb" ) );
	const auto *pNameDesc =
			NDb::Get<NDb::SWindowTextView>( CDBID( "UI/Game/Menu/MPGameRoom/Placement/Name_WindowTextView.xdb" ) );
	if ( !pFlagDesc || !pNameDesc )
	{
		return;
	}
	placementMarkers.resize( nStarts );
	if ( NGameX::IsCoopGame( gameDesc ) )
	{
		for ( const auto &bot : NGameX::CollectCoopMapPlayers( pMap ) )
		{
			placementMarkers[bot.nPlayerIndex].wszBotName = bot.wszName;
		}
	}
	for ( int i = 0; i < nStarts; ++i )
	{
		auto &marker = placementMarkers[i];
		if ( !NGameX::IsManualPlacementStart( gameDesc, i ) && marker.wszBotName.empty() )
		{
			continue;
		}
		marker.pFlag = AddWindowCopy( pPlacementPanel, pFlagDesc );
		marker.pName = dynamic_cast<ITextView *>( AddWindowCopy( pPlacementPanel, pNameDesc ) );
		if ( marker.pFlag )
		{
			if ( IWindow *pBlock = marker.pFlag->GetChild( "TeamBlock", false ) )
			{
				marker.pPlayerColor = new CColorBackground();
				pBlock->SetBackground( marker.pPlayerColor );
			}
		}
	}
	if ( bHost )
	{
		const auto *pToggleDesc =
				NDb::Get<NDb::SWindowMSButton>( CDBID( "UI/Game/Menu/MPGameRoom/Placement/Toggle_WindowMSButton.xdb" ) );
		if ( pToggleDesc )
		{
			pPlacementToggle = dynamic_cast<IButton *>( AddWindowCopy( pPlacementPanel, pToggleDesc ) );
		}
	}
	UpdatePlacementMarkers();
}

void CInterfaceMPGameRoom::PositionPlacementMarker( int nSlot, const CVec2 &vCenter )
{
	const auto &marker = placementMarkers[nSlot];
	const auto panel = pPlacementPanel->GetWindowRect();
	const auto flag = marker.pFlag->GetPlacement();
	const float x = vCenter.x - panel.left;
	const float y = vCenter.y - panel.top;
	marker.pFlag->SetPlacement( x - flag.Width() * 0.5f, y - flag.Height() * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	if ( marker.pName )
	{
		const int nWidth = marker.pName->GetPlacement().Width();
		// Keep edge labels inside the panel even when the map preview is narrow.
		const float fLeft = std::clamp( x - nWidth * 0.5f, 0.0f, (std::max)( 0.0f, panel.Width() - nWidth ) );
		marker.pName->SetPlacement( fLeft, y + flag.Height() * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );
	}
}

void CInterfaceMPGameRoom::RestorePlacementMarker( int nSlot )
{
	const auto &mapPlayer = gameDesc.pMPMap->pMap->players[manualPlacement[nSlot]];
	const auto rect = pMinimap->GetWindowRect();
	PositionPlacementMarker( nSlot, CVec2( rect.left, rect.top ) + pMinimap->GetAIToScreen( mapPlayer.vMPStartPos ) );
}

void CInterfaceMPGameRoom::UpdatePlacementMarkers()
{
	if ( pPlacementToggle )
	{
		pPlacementToggle->SetState( manualPlacement.empty() ? 0 : 1 );
	}
	const auto *pConsts = NGameX::GetMPConsts();
	for ( int i = 0; i < placementMarkers.size(); ++i )
	{
		const auto &marker = placementMarkers[i];
		if ( !marker.pFlag )
		{
			continue;
		}
		const SMPSlot *pPlayer = i < slots.size() && slots[i].info.bPresent ? &slots[i].info : nullptr;
		const bool bBot = !marker.wszBotName.empty();
		bool bVisible = !manualPlacement.empty();
		if ( bVisible )
		{
			const CVec2 &pos = gameDesc.pMPMap->pMap->players[manualPlacement[i]].vMPStartPos;
			bVisible = std::isfinite( pos.x ) && std::isfinite( pos.y );
		}
		marker.pFlag->ShowWindow( bVisible );
		if ( marker.pName )
		{
			marker.pName->ShowWindow( bVisible && ( pPlayer || bBot ) );
		}
		if ( !bVisible )
		{
			continue;
		}
		ShowFlagLayer( marker.pFlag, "Team1", pPlayer && pPlayer->nTeam == 0 );
		ShowFlagLayer( marker.pFlag, "Team2", bBot || ( pPlayer && pPlayer->nTeam == 1 ) );
		ShowFlagLayer( marker.pFlag, "Empty", !pPlayer && !bBot );
		ShowFlagLayer( marker.pFlag, "TeamBlock", pPlayer || bBot );
		if ( marker.pPlayerColor )
		{
			marker.pPlayerColor->nColor = bBot ? NGameX::COOP_BOT_COLOR : 0xff808080;
			if ( pPlayer && pConsts && pPlayer->nColour >= 0 && pPlayer->nColour < pConsts->playerColorInfos.size() )
			{
				marker.pPlayerColor->nColor = pConsts->playerColorInfos[pPlayer->nColour].nColor | 0xff000000;
			}
		}
		if ( marker.pName && ( pPlayer || bBot ) )
		{
			marker.pName->SetText( marker.pName->GetDBText() +
			                       ( pPlayer ? NStr::ToUnicode( pPlayer->szName ) : marker.wszBotName ) );
		}
		if ( nDraggedSlot == i && !pPlayer )
		{
			nDraggedSlot = -1;
		}
		if ( nDraggedSlot != i )
		{
			RestorePlacementMarker( i );
		}
	}
}

bool CInterfaceMPGameRoom::ToggleManualPlacement()
{
	if ( bConnected && bHost && pPlacementToggle )
	{
		const auto order = manualPlacement.empty() ? NGameX::GenerateManualPlacement( gameDesc ) : std::vector<uint8_t>{};
		Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIManualPlacementMessage( order ) );
	}
	return true;
}

bool CInterfaceMPGameRoom::OnManualPlacementMessage( const SMPUIManualPlacementMessage *pMsg )
{
	if ( pMsg->manualPlacement.empty() || NGameX::IsValidManualPlacement( gameDesc, pMsg->manualPlacement ) )
	{
		nDraggedSlot = -1;
		manualPlacement = pMsg->manualPlacement;
		UpdatePlacementMarkers();
	}
	return true;
}

int CInterfaceMPGameRoom::FindPlacementMarker( const CVec2 &vPos, bool bOccupiedOnly ) const
{
	// Flags can overlap on a small preview. Pick the closest center, and only pick up occupied starts.
	int nClosest = -1;
	float fClosest = std::numeric_limits<float>::max();
	for ( int i = 0; i < placementMarkers.size(); ++i )
	{
		const auto &marker = placementMarkers[i];
		if ( marker.pFlag && marker.wszBotName.empty() && marker.pFlag->IsVisible() &&
		     ( !bOccupiedOnly || ( i < slots.size() && slots[i].info.bPresent ) ) && marker.pFlag->IsInside( vPos ) )
		{
			const auto center = marker.pFlag->GetWindowRect().GetCenter();
			const float fDistance = fabs2( vPos - CVec2( center.x, center.y ) );
			if ( fDistance < fClosest )
			{
				fClosest = fDistance;
				nClosest = i;
			}
		}
	}
	return nClosest;
}

bool CInterfaceMPGameRoom::OnPlacementPress( const SGameMessage &msg )
{
	if ( !bConnected || !bHost || !IsInFocus() || manualPlacement.empty() || !pPlacementToggle ||
	     ( pAdvancedPopup && pAdvancedPopup->IsVisible() ) || pMain->GetVisibleChild( "Initializing", false ) )
	{
		return false;
	}
	const CVec2 pos = ScreenToVirtual( UnPackCoords( msg.nParam1 ) );
	const int nSlot = FindPlacementMarker( pos, true );
	if ( pPlacementToggle->IsInside( pos ) || nSlot < 0 )
	{
		return false;
	}
	const auto rect = placementMarkers[nSlot].pFlag->GetWindowRect();
	vDragOffset = CVec2( rect.left + rect.Width() * 0.5f, rect.top + rect.Height() * 0.5f ) - pos;
	nDraggedSlot = nSlot;
	return true;
}

bool CInterfaceMPGameRoom::OnPlacementMove( const SGameMessage &msg )
{
	if ( nDraggedSlot < 0 )
	{
		return false;
	}
	PositionPlacementMarker( nDraggedSlot, ScreenToVirtual( UnPackCoords( msg.nParam1 ) ) + vDragOffset );
	return true;
}

void CInterfaceMPGameRoom::CancelPlacementDrag()
{
	if ( nDraggedSlot >= 0 )
	{
		RestorePlacementMarker( nDraggedSlot );
		nDraggedSlot = -1;
	}
}

bool CInterfaceMPGameRoom::OnPlacementRelease( const SGameMessage &msg )
{
	if ( nDraggedSlot < 0 )
	{
		return false;
	}
	const int nSource = nDraggedSlot;
	// Pick against the actual map positions, including the original start for a cancelled drop.
	CancelPlacementDrag();
	const int nTarget = FindPlacementMarker( ScreenToVirtual( UnPackCoords( msg.nParam1 ) ), false );
	if ( nTarget >= 0 && nTarget != nSource )
	{
		auto order = manualPlacement;
		std::swap( order[nSource], order[nTarget] );
		if ( NGameX::IsValidManualPlacement( gameDesc, order ) )
		{
			Singleton<IMPToUIManager>()->AddUIMessage( new SMPUIManualPlacementMessage( order ) );
		}
	}
	return true;
}
