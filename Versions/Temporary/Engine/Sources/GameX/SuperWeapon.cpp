#include "stdafx.h"
#include "SuperWeapon.hpp"
#include "Stats_B2_M1/UserActions.h"
#include "Input/Bind.h"
#include "MissionUnitFullInfo.h"
#include "CommandsSender.h"
#include "WorldClient.h"
#include "Stats_B2_M1/AIUnitCmd.h"

// CMissionSuperWeapon

CMissionSuperWeapon::CMissionSuperWeapon()
{
}

void CMissionSuperWeapon::Init( IWindow *pScr, CWorldClient *_pWorld, NDb::ESeason eSeason )
{
	pWorld = _pWorld;

	pSuperWeaponBtn = GetChildChecked<IButton>( pScr, "Button04", true );
	pSuperWeaponProgress = GetChildChecked<IWindowRoundProgressBar>( pSuperWeaponBtn, "Progress", true );
	pSuperWeaponMaskWnd = GetChildChecked<IWindow>( pSuperWeaponBtn, "Mask", true );
	if ( pSuperWeaponBtn )
		pSuperWeaponBtn->ShowWindow( false );
	if ( pSuperWeaponProgress )
		pSuperWeaponProgress->ShowWindow( false );
	if ( pSuperWeaponMaskWnd )
		pSuperWeaponMaskWnd->ShowWindow( false );

	pUnitFullInfo = new CMissionUnitFullInfo();
	IWindow *pSuperWeaponInfoPanel = GetChildChecked<IWindow>( pScr, "SuperWeaponInfoPanel", true );
//	IWindow *pFullInfoWnd = GetChildChecked<IWindow>( pSuperWeaponInfoPanel, "UnitFullInfo3", true );
	IWindow *pAppearanceWnd = GetChildChecked<IWindow>( pScr, "AppearancePanel", true );
	IWindow *pAppearanceForSuperWeaponWnd = GetChildChecked<IWindow>( pAppearanceWnd, "AppearanceForSuperWeapon", true );
	pUnitFullInfo->InitBySuperWeapon( pSuperWeaponInfoPanel, pAppearanceForSuperWeaponWnd, eSeason );

	bExist = false;
	bEnabled = false;
	fProgress = 0.0f;
	bActive = false;
	bDisabledByInterface = false;

	//{ CRAP - test
/*	bExist = true;
	bEnabled = true;
	fProgress = 0.3f;
	bActive = false;*/
	//}
	
	UpdateVisual();
}

void CMissionSuperWeapon::OnUpdateSuperWeaponControl( CMapObj *_pMO, const NDb::SHPObjectRPGStats *_pDBUnit, bool _bExist )
{
	pMO = _pMO;
	bExist = _bExist;
	pDBUnit = _pDBUnit;

	if ( pMO )
		pUnitFullInfo->SetObject( pMO );
	else if ( pDBUnit )
		pUnitFullInfo->SetReinfUnit( pDBUnit );
	
	bDisabledByInterface = false;

	UpdateVisual();
}

void CMissionSuperWeapon::OnUpdateSuperWeaponRecycle( float _fProgress )
{
	fProgress = _fProgress;
	bEnabled = (fProgress == 1.0f);

	bDisabledByInterface = false;

	UpdateVisual();
}

void CMissionSuperWeapon::Call( const CVec2 &vPos )
{
	SAIUnitCmd cmd;
	cmd.nCmdType = ACTION_COMMAND_USE_SUPER_WEAPON;
	cmd.vPos = vPos;
	pWorld->GetCommandsSender()->CommandUnitCommand( &cmd );
	
	bDisabledByInterface = true;

	Deactivate();
}

void CMissionSuperWeapon::Activate()
{
	bActive = true;

	NDb::EUserAction eUserAction = NDb::USER_ACTION_SUPER_WEAPON_MODE;
	NInput::PostEvent( "set_forced_action", eUserAction, 0 );

	UpdateVisual();
}

void CMissionSuperWeapon::Deactivate()
{
	bActive = false;

	NInput::PostEvent( "game_reset_forced_action", 0, 0 );

	UpdateVisual();
}

void CMissionSuperWeapon::UpdateVisual()
{
//	if ( pSuperWeaponBtn )
//		pSuperWeaponBtn->ShowWindow( bExist );

	if ( !bExist )
		return;

	if ( pSuperWeaponBtn )
	{
		int nTargetState = bActive ? 1 : 0;
		if ( pSuperWeaponBtn->GetState() != nTargetState )
			pSuperWeaponBtn->SetStateWithVisual( nTargetState );
		if ( pSuperWeaponBtn->IsEnabled() != IsEnabled() )
			pSuperWeaponBtn->Enable( IsEnabled() );
	}

	bool bClockVisible = !IsEnabled();
	if ( pSuperWeaponProgress )
		pSuperWeaponProgress->ShowWindow( bClockVisible );
	if ( pSuperWeaponMaskWnd )
		pSuperWeaponMaskWnd->ShowWindow( bClockVisible );

	if ( bClockVisible )
	{
		if ( pSuperWeaponProgress )
			pSuperWeaponProgress->SetAngles( FP_PI2, FP_PI2 - fProgress * FP_2PI );
	}
}

bool CMissionSuperWeapon::IsEnabled() const
{
	return bEnabled && !bDisabledByInterface;
}

bool CMissionSuperWeapon::CanActivate() const
{
	return bExist && IsEnabled() && !bActive;
}

void CMissionSuperWeapon::UpdateObject( CMapObj *_pMO )
{
	if ( pMO && pMO == _pMO )
		pUnitFullInfo->UpdateObject( pMO );
}

REGISTER_SAVELOAD_CLASS( 0x17249BC0, CMissionSuperWeapon )


