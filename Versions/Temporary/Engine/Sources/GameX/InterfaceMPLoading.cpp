#include "stdafx.h"
#include "InterfaceMPLoading.h"
#include "GameXClassIDs.h"
#include "Misc/StrProc.h"
#include "GameRoomData.h"
#include "ScenarioTracker.h"

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

CInterfaceMPLoading2D::~CInterfaceMPLoading2D()
{
}

void CInterfaceMPLoading2D::MakeInterior( const SParams &params )
{
	IWindow *pMain = GetChildChecked<IWindow>( GetScreen(), "Main", true );

	IWindow *pMapPictureWnd = GetChildChecked<IWindow>( pMain, "MapPicture", true );
	ITextView *pMapNameView = GetChildChecked<ITextView>( pMain, "MapNameView", true );

	IWindow *pMinimapPanel = GetChildChecked<IWindow>( pMain, "MinimapPanel", true );
	IMiniMap *pMinimapWnd = GetChildChecked<IMiniMap>( pMinimapPanel, "Minimap", true );
	IWindow *pPlayerOnMapTeamWnd = GetChildChecked<IWindow>( pMinimapPanel, "PlayerOnMapTeam", true );
	/*IScrollableContainer *pPlayerList = GetChildChecked<IScrollableContainer>( pMain, "PlayerList", true );
	IWindow *pPlayerListItem = GetChildChecked<IWindow>( pPlayerList, "PlayerListItem", true );
	pPlayerListItem->ShowWindow( false );*/
	
	if ( pMapPictureWnd && params.pMapPicture )
		pMapPictureWnd->SetTexture( params.pMapPicture );

	if ( pMinimapWnd )
	{
		pMinimapWnd->SetLoadingMapParams( params.vMapAISize.x, params.vMapAISize.y );
		pMinimapWnd->SetTexture( params.pMinimap );
	}
	if ( pMapNameView )
		pMapNameView->SetText( pMapNameView->GetDBText() + params.wszMapName );
		
	if ( pPlayerOnMapTeamWnd )
		pPlayerOnMapTeamWnd->ShowWindow( false );

	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	int nMinimapX, nMinimapY;
	pMinimapWnd->GetPlacement( &nMinimapX, &nMinimapY, 0, 0 );

	for ( int i = 0; i < params.players.size(); ++i )
	{
		const SPlayer &player = params.players[i];
	
		// Put on minimap
		CVec2 vPos = pMinimapWnd->GetAIToScreen( player.vMinimapPos );

		if ( IWindow *pWnd = AddWindowCopy( pMinimapPanel, pPlayerOnMapTeamWnd ) )
		{
			pWnd->ShowWindow( true );

			IWindow *pTeam1Wnd = GetChildChecked<IWindow>( pWnd, "Team1", true );
			IWindow *pTeam2Wnd = GetChildChecked<IWindow>( pWnd, "Team2", true );
			IWindow *pColourWnd = GetChildChecked<IWindow>( pWnd, "TeamBlock", true );
			IWindow *pIconWnd = GetChildChecked<IWindow>( pWnd, "Icon", true );
			ITextView *pNameView = GetChildChecked<ITextView>( pWnd, "Name", true );

			int nSizeX, nSizeY;
			pWnd->GetPlacement( 0, 0, &nSizeX, &nSizeY );
			pWnd->SetPlacement( nMinimapX + vPos.x - nSizeX * 0.5f, nMinimapY + vPos.y - nSizeY * 0.5f, 0, 0, EWPF_POS_X | EWPF_POS_Y );

			CPtr<CColorBackground> pColourBgr = new CColorBackground();
			pColourBgr->nColor = pScenarioTracker->GetPlayerColor( player.nPlayerIndex ).dwColor;
			pTeam1Wnd->ShowWindow( player.nTeam == 0 );
			pTeam2Wnd->ShowWindow( player.nTeam == 1 );
			if ( pColourWnd )
				pColourWnd->SetBackground( pColourBgr );
			pIconWnd->SetTexture( player.pSideMinimapIcon );
			pNameView->SetText( pNameView->GetDBText() + player.wszName );
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
			pColourBgr->nColor = pScenarioTracker->GetPlayerColor( player.nPlayerIndex ).dwColor;
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
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MP_LOADING_2D, CInterfaceMPLoading2D )


