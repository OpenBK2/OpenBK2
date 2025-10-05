#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "InterfaceChapterMapMenu.h"
#include "InterfaceState.h"
#include "UISpecificB2/DBUISpecificB2.h"
#include "Misc/StrProc.h"
#include "GameXClassIDs.h"
#include "InterfaceMisc.h"
#include "ScenarioTracker.h"
#include "InterfaceEncyclopedia.h"
#include "System/Text.h"

#include <zconf.h>

// CInterfaceChapterMapMenu -- ReinfDesc window

#define REINF_DESC_ITEM_HOR_OFFSET 2
#define REINF_DESC_ITEM_VER_OFFSET 2

void CInterfaceChapterMapMenu::ShowReinfDesc( const NDb::SReinforcement *pReinf )
{
	if ( !pReinf )
		return;

	pReinfDesc->ShowWindow( true );
	eReinfDescState = ERDWS_REINF;
	pReinfDesc1Unit->ShowWindow( false );
	pReinfDesc1Reinf->ShowWindow( true );
	pReinfDescMultiReinf->ShowWindow( false );
	pReinfDescUpgrade->ShowWindow( false );

	IWindow *pOfficerIcon = GetChildChecked<IWindow>( pReinfDesc1Reinf, "OfficerIcon", true );
	if ( Singleton<IScenarioTracker>()->GetLeaderLevel( 0, pReinf->eType ) >= 0 )
		pOfficerIcon->GetChild( 0 )->ShowWindow( true );
	else
		pOfficerIcon->GetChild( 0 )->ShowWindow( false );

	// Find unit buttons
	IWindow *pCurrentWindow = GetCurrentReinfDescPopup();
	reinfDescUnits.resize( 5 );
	for ( int i = 0; i < 5; ++i )
	{
		IButton *pButton = GetChildChecked<IButton>( pCurrentWindow, StrFmt( "ReinfDescButton%1d", i + 1 ), true );
		if ( pButton )
		{
			reinfDescUnits[i].pButton = pButton;
			reinfDescUnits[i].pIcon = GetChildChecked<IWindow>( pButton, "Icon", true );
			HideChildren( reinfDescUnits[i].pIcon );
		}
	}

	reinfDescSingleItem.pButton = GetChildChecked<IButton>( pCurrentWindow, "SingleReinfDescButton", true );
	reinfDescSingleItem.pIcon = GetChildChecked<IWindow>( reinfDescSingleItem.pButton, "Icon", true );
	HideChildren( reinfDescSingleItem.pIcon );
	reinfDescSingleItem.pReinforcement = 0;		
	const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();
	for ( int i = 0; i < pUIC->reinfButtons.size(); ++i )
	{
		if ( pUIC->reinfButtons[i].eType == pReinf->eType )
		{
			reinfDescSingleItem.pReinforcement = pReinf;
			reinfDescSingleItem.pButton->SetState( 0 );
			reinfDescSingleItem.pIcon->ShowWindow( true );
			reinfDescSingleItem.pIcon->SetTexture( pUIC->reinfButtons[i].pTexture );
      break;
		}
	}
	
	if ( !reinfDescSingleItem.pReinforcement )
	{
		OnReinfDescOK();
		return;
	}

	ReinfDescFillUnits( pReinf, reinfDescUnits );
	ReinfDescSelectUnit( reinfDescUnits, 0 );
}

void CInterfaceChapterMapMenu::ShowReinfDesc( const NDb::SReinforcement *pReinf, const NDb::SReinforcement *pReinfUpg )
{
	if ( !pReinfUpg )
		return;

	pReinfDesc->ShowWindow( true );
	eReinfDescState = ERDWS_UPGRADE;
	pReinfDesc1Unit->ShowWindow( false );
	pReinfDesc1Reinf->ShowWindow( false );
	pReinfDescMultiReinf->ShowWindow( false );
	pReinfDescUpgrade->ShowWindow( true );

	IWindow *pCurrentWindow = GetCurrentReinfDescPopup();
	reinfDescSingleItem.pButton = GetChildChecked<IButton>( pCurrentWindow, "SingleReinfDescButton", true );
	reinfDescSingleItem.pIcon = GetChildChecked<IWindow>( reinfDescSingleItem.pButton, "Icon", true );
	HideChildren( reinfDescSingleItem.pIcon );
	reinfDescSingleItem.pReinforcement = 0;		
	const NDb::SUIConstsB2 *pUIC = InterfaceState()->GetUIConsts();
	for ( int i = 0; i < pUIC->reinfButtons.size(); ++i )
	{
		if ( pUIC->reinfButtons[i].eType == pReinfUpg->eType )
		{
			reinfDescSingleItem.pReinforcement = pReinfUpg;
			reinfDescSingleItem.pButton->SetState( 0 );
			reinfDescSingleItem.pIcon->ShowWindow( true );
			reinfDescSingleItem.pIcon->SetTexture( pUIC->reinfButtons[i].pTexture );
			break;
		}
	}

	if ( !reinfDescSingleItem.pReinforcement )
	{
		OnReinfDescOK();
		return;
	}

	// Get buttons1
	IWindow *pUnitsArea = GetChildChecked<IWindow>( pReinfDescUpgrade, "ReinfDescUnitsArea", true );
	reinfDescUnits.resize( 5 );
	for ( int i = 0; i < 5; ++i )
	{
		IButton *pButton = GetChildChecked<IButton>( pUnitsArea, StrFmt( "ReinfDescButton%1d", i + 1 ), true );
		if ( pButton )
		{
			reinfDescUnits[i].pButton = pButton;
			reinfDescUnits[i].pIcon = GetChildChecked<IWindow>( pButton, "Icon", true );
			HideChildren( reinfDescUnits[i].pIcon );
		}
	}
	ReinfDescFillUnits( pReinf, reinfDescUnits );

	// Get buttons2
	pUnitsArea = GetChildChecked<IWindow>( pReinfDescUpgrade, "ReinfDescUnitsArea2", true );
	reinfDescUnits2.resize( 5 );
	for ( int i = 0; i < 5; ++i )
	{
		IButton *pButton = GetChildChecked<IButton>( pUnitsArea, StrFmt( "ReinfDescButtonUpg%1d", i + 1 ), true );
		if ( pButton )
		{
			reinfDescUnits2[i].pButton = pButton;
			reinfDescUnits2[i].pIcon = GetChildChecked<IWindow>( pButton, "Icon", true );
			HideChildren( reinfDescUnits2[i].pIcon );
		}
	}
	ReinfDescFillUnits( pReinfUpg, reinfDescUnits2 );

	ReinfDescSelectUnit( reinfDescUnits, -1 );
	ReinfDescSelectUnit( reinfDescUnits2, 0 );
}

void CInterfaceChapterMapMenu::ShowReinfDesc( const NDb::SMechUnitRPGStats *pMech, const NDb::SSquadRPGStats *pSquad )
{
	pReinfDesc->ShowWindow( true );
	eReinfDescState = ERDWS_UNIT;
	pReinfDesc1Unit->ShowWindow( true );
	pReinfDesc1Reinf->ShowWindow( false );
	pReinfDescMultiReinf->ShowWindow( false );
	pReinfDescUpgrade->ShowWindow( false );

	IWindow *pCurrentWindow = GetCurrentReinfDescPopup();
	IButton *pButton = GetChildChecked<IButton>( pCurrentWindow, "SingleReinfDescButton", true );
	reinfDescSingleItem.pButton = pButton;
	reinfDescSingleItem.pIcon = GetChildChecked<IWindow>( pButton, "Icon", true );
	HideChildren( reinfDescSingleItem.pIcon );
	pButton->SetState( 0 );
	reinfDescSingleItem.pIcon->ShowWindow( true );
	reinfDescSingleItem.pMechUnit = pMech;
	reinfDescSingleItem.pSquad = pSquad;

	if ( pMech )
	{
		reinfDescSingleItem.pIcon->SetTexture( pMech->pIconTexture );
	}
	else if ( pSquad )
	{
		reinfDescSingleItem.pIcon->SetTexture( pSquad->pIconTexture );
	}
	else
	{
		OnReinfDescOK();		// Cancel
		return;
	}

	ReinfDescMakeUnitInfo();
}

void CInterfaceChapterMapMenu::OnReinfDescItem( const std::string &szSender )
{
	for ( int i = 0; i < reinfDescUnits.size(); ++i )
	{
		if ( reinfDescUnits[i].pButton->GetName() == szSender )
		{
			if ( eReinfDescState == ERDWS_UPGRADE )
				ReinfDescSelectUnit( reinfDescUnits2, -1 );

			ReinfDescSelectUnit( reinfDescUnits, i );
			return;
		}
	}

	if ( eReinfDescState == ERDWS_MULTIREINF )
	{
		for ( int i = 0; i < reinfDescReinfs.size(); ++i )
		{
			if ( reinfDescReinfs[i].pButton->GetName() == szSender )
			{
				ReinfDescSelectReinf( i );
				return;
			}
		}
	}
	else if ( eReinfDescState == ERDWS_UPGRADE )
	{
		for ( int i = 0; i < reinfDescUnits2.size(); ++i )
		{
			if ( reinfDescUnits2[i].pButton->GetName() == szSender )
			{
				ReinfDescSelectUnit( reinfDescUnits, -1 );
				ReinfDescSelectUnit( reinfDescUnits2, i );
				return;
			}
		}
	}
}

void CInterfaceChapterMapMenu::OnReinfDescOK()
{
	pReinfDesc->ShowWindow( false );
	eReinfDescState = ERDWS_NONE;
	bInitialDialogVisible = false;
	ProceedInitialDialogs();
}

void CInterfaceChapterMapMenu::ReinfDescFillUnits( const NDb::SReinforcement *pReinf, CReinfButtonList &units )
{
	// Clear entries
	for ( int j = 0; j < units.size(); ++j )
	{
		units[j].pMechUnit = 0;
		units[j].pSquad = 0;
		units[j].nQuantity = 0;
		units[j].pButton->SetState( 2 );
		units[j].pIcon->ShowWindow( false );
		units[j].pButton->SetTextString( L"" );
		ITextView *pUnitQty = GetChildChecked<ITextView>( units[j].pButton, "UnitNumber", true );
		if ( pUnitQty )
			pUnitQty->SetText( L"" );
}
	ITextView *pReinfName = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "ReinforcementName", true );
	pReinfName->SetText( L"" );

	if ( !pReinf )
	{
		ReinfDescSelectUnit( units, -1 );
		return;
	}

	if ( CHECK_TEXT_NOT_EMPTY_PRE(pReinf->,LocalizedName) )
		pReinfName->SetText( pReinfName->GetDBText() + GET_TEXT_PRE(pReinf->,LocalizedName) );

	int nUsedSlots = 0;
	for ( int i = 0; i < pReinf->entries.size(); ++i )
	{
		const NDb::SReinforcementEntry &entry = pReinf->entries[i];

		int nIndex = -1;
		for ( int j = 0; j < units.size(); ++j )
		{
			if ( units[j].pMechUnit == entry.pMechUnit && units[j].pSquad == entry.pSquad )
			{
				nIndex = j;
				break;
			}
		}

		if ( nIndex < 0 && nUsedSlots < 5 )		// not found, create new
		{
			nIndex = nUsedSlots;
			++nUsedSlots;
			units[nIndex].nQuantity = 1;
			if ( entry.pMechUnit )
			{
				units[nIndex].pMechUnit = entry.pMechUnit;
				units[nIndex].pIcon->ShowWindow( true );
				units[nIndex].pIcon->SetTexture( entry.pMechUnit->pIconTexture );
			}
			else if ( entry.pSquad )
			{
				units[nIndex].pSquad = entry.pSquad;
				units[nIndex].pIcon->ShowWindow( true );
				units[nIndex].pIcon->SetTexture( entry.pSquad->pIconTexture );
			}
			else 
			{
				NI_ASSERT( false, StrFmt( "DESIGN: Invalid reinforcement entry ReinfID \"%s\", entry %d", NDb::GetResName(pReinf), i ) );
			}
		}
		else
		{
			++units[nIndex].nQuantity;
		}
	}
	for ( int j = 0; j < units.size(); ++j )
	{
		ITextView *pUnitQty = GetChildChecked<ITextView>( units[j].pButton, "UnitNumber", true );
		if ( !pUnitQty )
			continue;
		if ( units[j].nQuantity > 1 )
			pUnitQty->SetText( pUnitQty->GetDBText() + NStr::ToUnicode( StrFmt( "%2d", units[j].nQuantity ) ) );
		else
			pUnitQty->SetText( L"" );
	}
}

void CInterfaceChapterMapMenu::ReinfDescSelectUnit( CReinfButtonList &units, int nIndex )
{
	if ( nIndex < 0 || nIndex > units.size() )
	{
		reinfDescSingleItem.pMechUnit = 0;
		reinfDescSingleItem.pSquad = 0;
	}
	else if ( units[nIndex].nQuantity == 0 )
		return;

	for ( int i = 0; i < units.size(); ++i )
	{
		if ( units[i].nQuantity > 0 )
		{
			if ( i == nIndex )
			{
				units[i].pButton->SetState( 1 );
				reinfDescSingleItem.pMechUnit = units[i].pMechUnit;
				reinfDescSingleItem.pSquad = units[i].pSquad;
			}
			else
			{
				units[i].pButton->SetState( 0 );
			}
		}
		else
		{
			units[i].pButton->SetState( 2 );
		}
	}

	ReinfDescMakeUnitInfo();
}

void CInterfaceChapterMapMenu::ReinfDescMakeUnitInfo()
{
	ITextView *pUnitName = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "UnitParamsName", true );
	ITextView *pUnitHP = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "UnitParamsHP", true );
	ITextView *pUnitArmor = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "UnitParamsArmor", true );
	ITextView *pUnitDamage = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "UnitParamsDamage", true );
	ITextView *pUnitPiercing = GetChildChecked<ITextView>( GetCurrentReinfDescPopup(), "UnitParamsPiercing", true );
	IButton *pEncyclopedia = GetChildChecked<IButton>( GetCurrentReinfDescPopup(), "ReinfDescEncyclopedia", true );

	std::wstring wszUnitName = L"";
	int nHP = 0;
	int nArmor = 0;
	int nDamage = 0;
	int nPiercing = 0;

	if ( reinfDescSingleItem.pMechUnit )
	{
		const NDb::SMechUnitRPGStats *pUnit = reinfDescSingleItem.pMechUnit;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pUnit->,LocalizedName) )
			wszUnitName = GET_TEXT_PRE(pUnit->,LocalizedName);
		nHP = pUnit->fMaxHP;
		nArmor = pUnit->nMaxArmor;

		for ( int i = 0; i < pUnit->GetPlatformsSize( -1 ); ++i )
		{
			for ( int j = 0; j < pUnit->GetGunsSize( -1, i ); ++j )
			{
				const NDb::SWeaponRPGStats *pWeapon = pUnit->GetGun( -1, i, j ).pWeapon ;

				if ( !pWeapon )
					continue;

				for ( int k = 0; k < pWeapon->shells.size(); ++k )
				{
					if ( pWeapon->shells[k].eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH && 
						   pWeapon->shells[k].fDamagePower > nDamage )
					{
						nDamage = pWeapon->shells[k].fDamagePower;
						nPiercing = pWeapon->shells[k].nPiercing;
					}
				}
			}
		}
	}
	else if ( reinfDescSingleItem.pSquad )
	{
		const NDb::SSquadRPGStats *pSquad = reinfDescSingleItem.pSquad;
		if ( CHECK_TEXT_NOT_EMPTY_PRE(pSquad->,LocalizedName) )
			wszUnitName = GET_TEXT_PRE(pSquad->,LocalizedName);
		nHP = pSquad->members.size();

		for ( int i = 0; i < pSquad->members.size(); ++i )
		{
			const NDb::SInfantryRPGStats *pSoldier = pSquad->members[i];

			if ( !pSoldier || pSoldier->GetGunsSize( -1, 0 ) == 0 )
				continue;

			const NDb::SWeaponRPGStats *pWeapon = pSoldier->GetGun( -1, 0, 0 ).pWeapon;

			if ( !pWeapon )
				continue;

			for ( int k = 0; k < pWeapon->shells.size(); ++k )
			{
				if ( pWeapon->shells[k].eDamageType == NDb::SWeaponRPGStats::SShell::DAMAGE_HEALTH && 
					   pWeapon->shells[k].fDamagePower > nDamage )
				{
					nDamage = pWeapon->shells[k].fDamagePower;
					nPiercing = pWeapon->shells[k].nPiercing;
				}
			}

		}
	}

	// Encyclopedia is shown only for mech units
	if ( reinfDescSingleItem.pMechUnit )
		pEncyclopedia->ShowWindow( true );
	else
		pEncyclopedia->ShowWindow( false );

	pUnitName->SetText( pUnitName->GetDBText() + wszUnitName );
	pUnitHP->SetText( pUnitHP->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nHP ) ) );
	pUnitArmor->SetText( pUnitArmor->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nArmor ) ) );
	pUnitDamage->SetText( pUnitDamage->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nDamage ) ) );
	pUnitPiercing->SetText( pUnitPiercing->GetDBText() + NStr::ToUnicode( StrFmt( "%d", nPiercing ) ) );
}

void CInterfaceChapterMapMenu::OnReinfDescEncyclopedia()
{
	if ( reinfDescSingleItem.pMechUnit )
	{
		CInterfaceEncyclopedia::RunWithUnit( reinfDescSingleItem.pMechUnit );
	}
	else
	{
		NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOk", 
		NStr::ToUnicode( StrFmt( "No Encyclopedia for infantry" ) ) ).c_str() );
	}
}

IWindow *CInterfaceChapterMapMenu::GetCurrentReinfDescPopup()
{
	switch ( eReinfDescState ) 
	{
	case ERDWS_UNIT:
		return pReinfDesc1Unit;
	case ERDWS_REINF:
		return pReinfDesc1Reinf;
	case ERDWS_MULTIREINF:
		return pReinfDescMultiReinf;
	case ERDWS_UPGRADE:
		return pReinfDescUpgrade;
	default:
		return 0;
	}
}

void CInterfaceChapterMapMenu::ReinfDescScrollRight()
{
	if ( nSelectedReinf == reinfDescReinfs.size() )
		return;

	pButtonScrollRight->Enable( false );
	pButtonScrollLeft->Enable( false );
	ReinfDescFillUnits( 0, reinfDescUnits );			// Hide old info

	reinfDescReinfs[0].pButton->GetPlacement( &nFirstButtonPosX, 0, 0, 0 );
	nFirstButtonPosX -= nButtonStep;
	nDelay = 8;
	reinfDescReinfs[nSelectedReinf].pButton->SetState( 0 );
	++nSelectedReinf;
}

void CInterfaceChapterMapMenu::ReinfDescScrollLeft()
{
	if ( nSelectedReinf == 0 )
		return;

	pButtonScrollRight->Enable( false );
	pButtonScrollLeft->Enable( false );
	ReinfDescFillUnits( 0, reinfDescUnits );			// Hide old info

	reinfDescReinfs[0].pButton->GetPlacement( &nFirstButtonPosX, 0, 0, 0 );
	nFirstButtonPosX += nButtonStep;
	nDelay = -8;
	reinfDescReinfs[nSelectedReinf].pButton->SetState( 0 );
	--nSelectedReinf;
}

void CInterfaceChapterMapMenu::ReinfDescProcessScroll( const int nStep )
{
	if ( nStep == 0 )						// Finished scroll
	{
		int nPosX = nFirstButtonPosX;
		for ( int i = 0; i < reinfDescReinfs.size(); ++i )
		{
			reinfDescReinfs[i].pButton->SetPlacement( nPosX, 0, 0, 0, EWPF_POS_X );
			nPosX += nButtonStep;
		}

		reinfDescReinfs[nSelectedReinf].pButton->SetState( 1 );
		ReinfDescFillUnits( reinfDescReinfs[nSelectedReinf].pReinforcement, reinfDescUnits );
		ReinfDescSelectUnit( reinfDescUnits, 0 );

		if ( nSelectedReinf > 0 )
			pButtonScrollLeft->Enable( true );
		else
			pButtonScrollLeft->Enable( false );

		if ( nSelectedReinf < reinfDescReinfs.size() - 1 )
			pButtonScrollRight->Enable( true );
		else
			pButtonScrollRight->Enable( false );
	}
	else											
	{
		int nDX = 0;
		if ( nStep > 0 )					// Scrolling right
			nDX = -nButtonStep / 8;
		else											// Scrolling left
			nDX = nButtonStep / 8;

		for ( int i = 0; i < reinfDescReinfs.size(); ++i )
		{
			int nPosX;
			reinfDescReinfs[i].pButton->GetPlacement( &nPosX, 0, 0, 0 );
			nPosX += nDX;
			reinfDescReinfs[i].pButton->SetPlacement( nPosX, 0, 0, 0, EWPF_POS_X );
		}
	}
}

void CInterfaceChapterMapMenu::ReinfDescSelectReinf( int nIndex )
{
	if ( nIndex < 0 || nIndex >= reinfDescReinfs.size() || nIndex == nSelectedReinf )
		return;

	int nOffset = ( nSelectedReinf - nIndex ) * nButtonStep;
	nSelectedReinf = nIndex;
	for ( int i = 0; i < reinfDescReinfs.size(); ++i )
	{
		int nPosX;
		reinfDescReinfs[i].pButton->GetPlacement( &nPosX, 0, 0, 0 );
		reinfDescReinfs[i].pButton->SetState( 0 );
		nPosX += nOffset;
		reinfDescReinfs[i].pButton->SetPlacement( nPosX, 0, 0, 0, EWPF_POS_X );
	}

	reinfDescReinfs[nSelectedReinf].pButton->SetState( 1 );
	ReinfDescFillUnits( reinfDescReinfs[nSelectedReinf].pReinforcement, reinfDescUnits );
	ReinfDescSelectUnit( reinfDescUnits, 0 );

	if ( nSelectedReinf > 0 )
		pButtonScrollLeft->Enable( true );
	else
		pButtonScrollLeft->Enable( false );

	if ( nSelectedReinf < reinfDescReinfs.size() - 1 )
		pButtonScrollRight->Enable( true );
	else
		pButtonScrollRight->Enable( false );
}

bool CInterfaceChapterMapMenu::OnMouseOverReinf( const std::string &szSender, bool bEnter )
{
	const int nLocalPlayer = 0;
	std::vector<IScenarioTracker::SChapterReinf> reinfs;
	Singleton<IScenarioTracker>()->GetChapterCurrentReinforcements( &reinfs, nLocalPlayer );

	for( int nRType = 0; nRType < reinfButtons.size(); ++nRType )
	{
		if ( !reinfButtons[nRType].pButton )
			continue;

		if ( /*reinfButtons[nRType].pButton->GetName() == szSender ||*/ reinfButtons[nRType].pBonusWnd->GetName() == szSender )
		{
			if ( !reinfButtons[nRType].pIcon->IsVisible() )
				continue;

			for ( int i = 0; i < targets.size(); ++i )
			{
				STarget &target = targets[i];

				for ( int j = 0; j < target.rewards.size(); ++j )
				{
					if ( target.rewardDescs[j]->eBonusType == NDb::CBT_REINF_CHANGE && target.rewardDescs[j]->eReinforcementType == nRType )
					{
						SReward &reward = target.rewards[j];

						if ( target.pBonusIcon )
							target.pBonusIcon->ShowWindow( bEnter );
						if ( target.pRecommended )
							target.pRecommended->ShowWindow( !bEnter && target.eState == EMS_RECOMMENDED );

						if ( i != nUpgradeMissionSelected || j != nUpgradeRewardSelected )
						{
							if ( reward.pBtn )
								reward.pBtn->ShowWindow( bEnter );
							if ( reward.pBonusWnd )
								reward.pBonusWnd->ShowWindow( false/*bEnter*/ );
						}
					}
				}
			}

			break;
		}
	}

	return true;
}


