#include "stdafx.h"

#include "CoopPlayerInfo.h"
#include "GameRoomData.h"
#include "GameXClassIDs.h"
#include "InterfaceMPLoading.h"
#include "Misc/StrProc.h"

#include "GameX_export.h"

#include <cmath>

#include <fmt/format.h>

// CInterfaceMPLoading2D

CInterfaceMPLoading2D::CInterfaceMPLoading2D()
{
	NI_ASSERT( 0, "Wrong call" );
}

CInterfaceMPLoading2D::CInterfaceMPLoading2D( const SParams &params )
{
	InitInternal( "MPLoading" );
	MakeInterior( params );
}

CInterfaceMPLoading2D::~CInterfaceMPLoading2D() {}

void CInterfaceMPLoading2D::MakeInterior( const SParams &params )
{
	IWindow *pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	IWindow *pMapPictureWnd = GetChildChecked<IWindow>( pMain, "MapPicture", true );
	ITextView *pMapNameView = GetChildChecked<ITextView>( pMain, "MapNameView", true );

	IWindow *pMinimapPanel = GetChildChecked<IWindow>( pMain, "MinimapPanel", true );
	IMiniMap *pMinimapWnd = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );
	IWindow *pPlayerOnMapTeamWnd = GetChildChecked<IWindow>( pMinimapPanel, "PlayerOnMapTeam", true );
	ITextView *pPlayerNameTemplate =
			pMinimapPanel ? dynamic_cast<ITextView *>( pMinimapPanel->GetChild( "PlayerOnMapNameView", false ) ) : nullptr;
	/*IScrollableContainer *pPlayerList = GetChildChecked<IScrollableContainer>( pMain, "PlayerList", true );
	IWindow *pPlayerListItem = GetChildChecked<IWindow>( pPlayerList, "PlayerListItem", true );
	pPlayerListItem->ShowWindow( false );*/

	if ( pMapPictureWnd && params.pMapPicture )
	{
		pMapPictureWnd->SetTexture( params.pMapPicture );
	}

	const bool bValidMapSize = params.vMapAISize.x > 0 && params.vMapAISize.y > 0;
	if ( pMinimapWnd && bValidMapSize )
	{
		pMinimapWnd->SetLoadingMapParams( params.vMapAISize.x, params.vMapAISize.y );
		if ( params.pMinimap )
		{
			pMinimapWnd->SetTexture( params.pMinimap );
		}
	}
	if ( pMapNameView )
	{
		pMapNameView->SetText( pMapNameView->GetDBText() + params.wszMapName );
	}

	if ( pPlayerOnMapTeamWnd )
	{
		pPlayerOnMapTeamWnd->ShowWindow( false );
	}
	if ( pPlayerNameTemplate )
	{
		pPlayerNameTemplate->ShowWindow( false );
	}

	if ( !pMinimapWnd || !pMinimapPanel || !pPlayerOnMapTeamWnd || !bValidMapSize )
	{
		CInterfaceLoadingBase::MakeInterior();
		return;
	}
	int nMinimapX, nMinimapY;
	pMinimapWnd->GetPlacement( &nMinimapX, &nMinimapY, 0, 0 );

	for ( int i = 0; i < params.players.size(); ++i )
	{
		const SPlayer &player = params.players[i];
		if ( !std::isfinite( player.vMinimapPos.x ) || !std::isfinite( player.vMinimapPos.y ) )
		{
			continue;
		}

		// Put on minimap
		CVec2 vPos = pMinimapWnd->GetAIToScreen( player.vMinimapPos );

		if ( IWindow *pWnd = AddWindowCopy( pMinimapPanel, pPlayerOnMapTeamWnd ) )
		{
			pWnd->ShowWindow( true );

			IWindow *pTeam1Wnd = GetChildChecked<IWindow>( pWnd, "Team1", true );
			IWindow *pTeam2Wnd = GetChildChecked<IWindow>( pWnd, "Team2", true );
			IWindow *pColourWnd = pWnd->GetChild( "TeamBlock", true );
			IWindow *pIconWnd = pWnd->GetChild( "Icon", true );
			ITextView *pNameView = dynamic_cast<ITextView *>( pWnd->GetChild( "Name", true ) );

			int nSizeX, nSizeY;
			pWnd->GetPlacement( 0, 0, &nSizeX, &nSizeY );
			pWnd->SetPlacement(
					nMinimapX + vPos.x - nSizeX * 0.5f, nMinimapY + vPos.y - nSizeY * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );

			CPtr<CColorBackground> pColourBgr = new CColorBackground();
			// Loading-only bots carry their colour without joining the human roster.
			pColourBgr->nColor = player.dwColour;
			if ( pTeam1Wnd )
			{
				pTeam1Wnd->ShowWindow( player.nTeam == 0 );
			}
			if ( pTeam2Wnd )
			{
				pTeam2Wnd->ShowWindow( player.nTeam == 1 );
			}
			if ( params.bCooperative && !pColourWnd )
			{
				// Legacy skins use solid teams and a separately coloured name. Combined
				// skins need their transparent team textures to expose the individual colour below.
				IWindow *pTeamWnd = player.nTeam == 0 ? pTeam1Wnd : pTeam2Wnd;
				if ( pTeamWnd )
				{
					CPtr<CColorBackground> pTeamColourBgr = new CColorBackground();
					pTeamColourBgr->nColor = player.nTeam == 0 ? 0xff008040 : NGameX::COOP_BOT_COLOR;
					pTeamWnd->SetBackground( pTeamColourBgr );
				}
			}
			if ( pColourWnd )
			{
				pColourWnd->SetBackground( pColourBgr );
			}
			if ( pIconWnd )
			{
				pIconWnd->ShowWindow( player.pSideMinimapIcon != nullptr );
				if ( player.pSideMinimapIcon )
				{
					pIconWnd->SetTexture( player.pSideMinimapIcon );
				}
			}
			if ( !pNameView && pPlayerNameTemplate )
			{
				// Stock UI keeps the name beside the marker template, while mods may nest it.
				pNameView = dynamic_cast<ITextView *>( AddWindowCopy( pMinimapPanel, pPlayerNameTemplate ) );
				if ( pNameView )
				{
					int nNameOffsetX, nNameOffsetY, nNameSizeX;
					pNameView->GetPlacement( &nNameOffsetX, &nNameOffsetY, &nNameSizeX, 0 );
					pNameView->SetPlacement( nMinimapX + vPos.x - nNameSizeX * 0.5f + nNameOffsetX,
					                         nMinimapY + vPos.y - nSizeY * 0.5f + nNameOffsetY,
					                         0,
					                         0,
					                         EWPF_POS_X | EWPF_POS_Y );
					pNameView->ShowWindow( true );
				}
			}
			if ( pNameView )
			{
				std::wstring wszName = pNameView->GetDBText();
				if ( params.bCooperative && !pColourWnd )
				{
					// Older templates have no individual-colour block beside the team indicator.
					wszName += NStr::ToUnicode( fmt::format( "<color=0x{:X}>", player.dwColour ) );
				}
				pNameView->SetText( wszName + player.wszName );
			}
		}

		// Add to list
		/*if ( IWindow *pWnd = AddWindowCopy( pPlayerList, pPlayerListItem ) )
		{
			IWindow *pTeam1Wnd = GetChildChecked<IWindow>( pWnd, "Team1", true );
			IWindow *pTeam2Wnd = GetChildChecked<IWindow>( pWnd, "Team2", true );
			IWindow *pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
			ITextView *pNameView = GetChildChecked<ITextView>( pWnd, "ItemName", true );

			CPtr<CColorBackground> pTeamColourBgr = new CColorBackground();
			pTeamColourBgr->nColor = ( player.nTeam == 0 ) ? 0xff008040 : 0xffC00000;
			CPtr<CColorBackground> pColourBgr = new CColorBackground();
			pColourBgr->nColor = player.dwColour;
			pTeam1Wnd->SetBackground( pTeamColourBgr );
			pTeam2Wnd->SetBackground( pColourBgr );
			pIconWnd->SetTexture( player.pSideMinimapIcon );
			pNameView->SetText( pNameView->GetDBText() + player.wszName );

			pPlayerList->PushBack( pWnd, false );
		}*/
	}
	/*if ( pPlayerList )
		pPlayerList->Update();*/

	CInterfaceLoadingBase::MakeInterior();
}

// just for using by MakeObjectVirtual( int nTypeID )
REGISTER_SAVELOAD_CLASS( GAMEX, ML_COMMAND_MP_LOADING_2D, CInterfaceMPLoading2D )
