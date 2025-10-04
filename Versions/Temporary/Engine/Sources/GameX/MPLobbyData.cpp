#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Input/gamemessage.h"
#include "UI/UI.h"
#include "Misc/2Darray.h"
#include "UI/uifactory.h"
#include "GameRoomData.h"
#include "Misc/StrProc.h"
#include "InterfaceMPLobby.h"

#include <zconf.h>

void CClientListViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{	
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );

	CDynamicCast<CClientListData> pInfo = pData;	
	NI_VERIFY( pInfo, "Wrong data", return );

	SetData( pWindow, pData );
}

void CClientListViewer::SetData ( CObjectBase *pWindow, const CObjectBase *pData  ) const
{
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	CDynamicCast<CClientListData> pInfo = pData;	
	NI_VERIFY( pInfo, "Wrong data", return );

	NMPSetData::SetChildText( pItem, "ItemClientName", NStr::ToUnicode( pInfo->szName ) );

	if ( !IsValid( pInfo->pStatusIcon ) )
	{
		pInfo->pStatusIcon = GetChildChecked<IButton>( pItem, "ItemClientStatus", true );
		pInfo->pStatusIcon->SetName( pInfo->szName );
	}
	if ( IsValid( pInfo->pStatusIcon ) )
	{
		if ( IsValid( pInterface ) )
			pInfo->pStatusIcon->SetState( pInterface->GetButtonState( pInfo->szName ) );
		else 
			pInfo->pStatusIcon->SetState( 0 );
	}
}

void CChannelListViewer::MakeInterior( CObjectBase *pWindow, const CObjectBase *pData ) const
{
	CDynamicCast<IListControlItem> pItem = pWindow;
	NI_VERIFY( pItem, "Wrong window", return );
	CDynamicCast<CTextData> pInfo = pData;

	wstring wszText = L"";
	if ( pInfo )
		wszText = pInfo->wszText;	

	CDynamicCast<ITextView> pTxt = pItem->GetChild( "ItemComboText", true );		
	NMPSetData::SetText( pTxt, wszText );			
}


