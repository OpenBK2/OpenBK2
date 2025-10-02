#include "StdAfx.h"
#include "misc/2darray.h"
#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "MouseTranslator.h"
#include "Selector.h"
#include "SceneB2/Camera.h"
#include "UI/UI.h"

#include <zconf.h>

// ************************************************************************************************************************ //
// **
// ** B1 mouse translator
// **
// **
// **
// ************************************************************************************************************************ //

BASIC_REGISTER_CLASS( CMouseTranslator );

#define MIN_ACTION_DIST 20.0f
#define MIN_ACTION_TIME 200

void CMouseTranslatorB1::OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect )
{
	switch ( GetActionState() )
	{
	case AS_SCROLL:
		if ( !bMouseMoveDirect )
			RaiseEvent( "scroll_map_true", vPos );
		break;
	case AS_SELECT:
		RaiseEvent( "update_selection", vPos );
		break;
	case AS_MOVE:
		if ( IsAltDown() ) 
		{
			RaiseEvent( "update_direction", vPos );
			bUpdateDir = true;
		}
		else if ( bUpdateDir )
		{
			RaiseEvent( "cancel_direction" );
			bUpdateDir = false;
		}
		break;
	}
}

void CMouseTranslatorB1::OnButtonUp( const CVec2 &vPos )
{
	int nSelectAction;
	switch ( GetActionState() )
	{
	case AS_MOVE:
		if ( !IsDownSamePos() )
			RaiseEvent ( "set_direction", vPos );
		else
			RaiseEvent ( "cancel_direction", vPos );
		break;
	case AS_SELECT:
		if ( !IsShiftDown() )
		{
			if ( IsDownSamePos() ) 
				nSelectAction = SA_CLEAR_ALWAYS;
			else 
				nSelectAction = SA_CLEAR_IF_NEW_NOT_EMPTY;
		}
		else if ( IsDownSamePos() )
			nSelectAction = SA_INVERSE;
		else
			nSelectAction = SA_PRESERVE;
		if ( IsCtrlDown() && IsDownSamePos() ) 
			nSelectAction |= SA_ONE_TYPE;
		if ( !IsDownSamePos() )
			nSelectAction |= SA_SELECT_BY_RECT;
		RaiseEvent( "end_selection", nSelectAction );
		break;
	}
}

void CMouseTranslatorB1::OnLButtonDown( const CVec2 &vPos, bool bObject )
{
	RaiseEvent( "start_selection", vPos );
	SetActionState( AS_SELECT );
}

void CMouseTranslatorB1::OnRButtonDown( const CVec2 &vPos, bool bObject )
{
	if ( IsDownTargetMode() )
			RaiseEvent( "set_target", vPos );
	else
	{
		if ( bObject ) 
		{
			if ( IsSelected( 0 ) ) 
				RaiseEvent( "do_action", vPos );
		}
		else
		{
			bUpdateDir = false;
			RaiseEvent( "set_destination", vPos );
			SetActionState( AS_MOVE );
		}
	}
}

void CMouseTranslatorB1::OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode )
{

}

void CMouseTranslatorB1::OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode )
{

}

// ************************************************************************************************************************ //
// **
// ** B2 mouse translator
// **
// **
// **
// ************************************************************************************************************************ //

CMouseTranslatorB2Base::CMouseTranslatorB2Base() :
	CMouseTranslator( 0 )
{
	InitPrivate();
}

CMouseTranslatorB2Base::CMouseTranslatorB2Base( CSelector *_pSelector ) :
	CMouseTranslator( _pSelector )
{
	InitPrivate();
}

void CMouseTranslatorB2Base::InitPrivate()
{
	AddObserver( "notify_forced_action", &CMouseTranslatorB2Base::MsgNotifyForcedAction );
	ResetState();
}

void CMouseTranslatorB2Base::ResetState()
{
	Camera()->SwitchManualScrolling( "mouse_translator", true );
	bWasForcedAction = false;
}

void CMouseTranslatorB2Base::OnMouseMove( const CVec2 &vPos, const bool bMouseMoveDirect ) 
{
	switch ( GetActionState() )
	{
	case AS_SCROLL:
		if ( !bMouseMoveDirect )
			RaiseEvent( "scroll_map_true", vPos );
		break;
	case AS_SELECT:
		RaiseEvent( "update_selection", vPos );
		break;
	case AS_MOVE:
		NInput::PostEvent( "update_direction", PackCoords( vPos ), IsDownSamePos( vPos ) || IsDownSameTime() );
		break;
	}
}

void CMouseTranslatorB2Base::OnButtonUp( const CVec2 &vPos )
{
	int nSelectAction;
	switch ( GetActionState() )
	{
	case AS_MOVE:
		if ( !IsDownSamePos() && !IsDownSameTime() )
			RaiseEvent ( "set_direction", vPos );
		else
			RaiseEvent ( "cancel_direction", vPos );
		break;
	case AS_SELECT:
		if ( !IsShiftDown() ) 
			nSelectAction = SA_CLEAR_IF_NEW_NOT_EMPTY;
		else if ( IsDownSamePos() )
			nSelectAction = SA_INVERSE;
		else
			nSelectAction = SA_PRESERVE;
		if ( IsCtrlDown() ) 
			nSelectAction |= SA_ONE_TYPE;
		if ( !IsDownSamePos() )
			nSelectAction |= SA_SELECT_BY_RECT;
		RaiseEvent( "end_selection", nSelectAction );
		Camera()->SwitchManualScrolling( "mouse_translator", true );
		break;
	}
}

void CMouseTranslatorB2Base::OnLButtonDown( const CVec2 &vPos, bool bObject )
{
	if ( IsDownTargetMode() )
			RaiseEventAndFlags( "set_target", vPos );
	else
	{
		RaiseEvent( "start_selection", vPos );
		SetActionState( AS_SELECT );
		Camera()->SwitchManualScrolling( "mouse_translator", false );
	}
}

void CMouseTranslatorB2Base::OnRButtonDown( const CVec2 &vPos, bool bObject )
{
	if ( IsDownTargetMode() )
		RaiseEvent( "reset_target", vPos );
	else
	{
		if ( IsSelected( 0 ) )
		{
			if ( bObject || IsCtrlDown() ) 
			{
				RaiseEventAndFlags( "do_action", vPos );
			}
			else
			{
				RaiseEvent( "set_destination", vPos );
				SetActionState( AS_MOVE );
			}
		}
	}
}

void CMouseTranslatorB2Base::OnMinimapLButtonDown( const CVec2 &vPos, bool bTargetMode )
{
	if ( bTargetMode )
		RaiseEventAndFlags( "set_target_minimap", vPos );
	else
	{
		RaiseEvent( "scroll_map", vPos );
		SetActionState( AS_SCROLL );
	}
}

void CMouseTranslatorB2Base::OnMinimapRButtonDown( const CVec2 &vPos, bool bTargetMode )
{
	if ( bTargetMode )
	{
		RaiseEvent( "scroll_map", vPos );
		SetActionState( AS_SCROLL );
	}
	else if ( IsSelected( 0 ) )
		RaiseEvent( "set_destination_minimap", vPos );
}

void CMouseTranslatorB2Base::CancelAction()
{
	CMouseTranslator::CancelAction();

	Camera()->SwitchManualScrolling( "mouse_translator", true );
}

void CMouseTranslatorB2Base::MsgNotifyForcedAction( const SGameMessage &msg )
{
	bWasForcedAction = true;
	CheckForcedAction();
}

bool CMouseTranslatorB2Base::ProcessEvent( const SGameMessage &event )
{
	bool bResult = CMouseTranslator::ProcessEvent( event );
	CheckForcedAction();
	return bResult;
}

void CMouseTranslatorB2Base::CheckForcedAction()
{
	if ( bWasForcedAction && !IsShiftDown() )
	{
		NInput::PostEvent( "new_reset_forced_action", 0, 0 );
		bWasForcedAction = false;
	}
}

// ************************************************************************************************************************ //
// **
// ** basic mouse translator
// **
// **
// **
// ************************************************************************************************************************ //

CMouseTranslator::CMouseTranslator( CSelector *_pSelector )	: 
	pSelector( _pSelector ),
	bindShift( "shift_key" ),
	bindCtrl( "ctrl_key" ),
	bindAlt( "alt_key" )
{
	ResetState();
	
	AddObserver( "cancel_action", &CMouseTranslator::MsgCancelAction );
	bDownSameTime = false;
	downTime = 0;
}

bool CMouseTranslator::IsSelected( class CMapObj *pMO ) const
{
	if ( pMO ) 
	{
		if ( pSelector->CanSelect( pMO ) )
			return pSelector->IsSelected( checked_cast<CMOSelectable*>(pMO) );
		else
      return false;
	}
	else 
		return !pSelector->IsEmpty();
}

void CMouseTranslator::MsgCancelAction( const SGameMessage &msg )
{
	CancelAction();
}

int CMouseTranslator::GetFlags() const
{
	int nFlags = 0;
	if ( IsShiftDown() )
		nFlags |= EKF_SHIFT;
	if ( IsAltDown() )
		nFlags |= EKF_ALT;
	if ( IsCtrlDown() )
		nFlags |= EKF_CTRL;
	return nFlags;
}

void CMouseTranslator::DoLButtonDblClk( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode )
{
//	if ( eActionState == AS_SELECT && pMO && IsSelected( pMO ) )
	if ( pMO && IsSelected( pMO ) )
	{
		RaiseEvent( "start_selection", vPos );
		int nSelectAction = SA_ONE_TYPE;
		if ( !IsShiftDown() ) 
			nSelectAction |= SA_CLEAR_ALWAYS;
		if ( IsCtrlDown() ) 
			nSelectAction |= SA_ON_WORLD;
		RaiseEvent( "end_selection", nSelectAction );
		eActionState = AS_NONE;
	}
}

void CMouseTranslator::DoLButtonDown( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode )
{
	if ( eButtonState == BS_NONE ) 
	{
		eButtonState = BS_LEFT_DOWN;
		bDownTargetMode = bTargetMode;
		vDownPos = vPos;
		downTime = Singleton<IGameTimer>()->GetAbsTime();
		OnLButtonDown( vPos, pMO != 0 );
	}
}

void CMouseTranslator::DoLButtonUp( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode )
{
	bDownSamePos = IsDownSamePos( vPos );
	bDownSameTime = IsDownSameTime( Singleton<IGameTimer>()->GetAbsTime() );
	if ( eButtonState == BS_LEFT_DOWN ) 
	{
		if ( vPos.x != -1 && vPos.y != -1 )
			OnButtonUp( vPos );
		eButtonState = BS_NONE;
		eActionState = AS_NONE;
	}
}

void CMouseTranslator::DoRButtonDown( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode )
{
	if ( eButtonState == BS_NONE )
	{
		eButtonState = BS_RIGHT_DOWN;
		bDownTargetMode = bTargetMode;
		vDownPos = vPos;
		downTime = Singleton<IGameTimer>()->GetAbsTime();
		OnRButtonDown( vPos, pMO != 0 );
	}
}

void CMouseTranslator::DoRButtonUp( const CVec2 &vPos, class CMapObj *pMO, bool bTargetMode )
{
//	bDownSamePos = ( vPos == vDownPos );
	bDownSamePos = IsDownSamePos( vPos );
	bDownSameTime = IsDownSameTime( Singleton<IGameTimer>()->GetAbsTime() );
	if ( eButtonState == BS_RIGHT_DOWN )
	{
		if ( vPos.x != -1 && vPos.y != -1 )
			OnButtonUp( vPos );
		eButtonState = BS_NONE;
		eActionState = AS_NONE;
	}
}

bool CMouseTranslator::ProcessEvent( const SGameMessage &event )
{
	bindShift.ProcessEvent( event );
	bindCtrl.ProcessEvent( event );
	bindAlt.ProcessEvent( event );
	
	return CGMORegContainer::ProcessEvent(event, this);
}

void CMouseTranslator::OnGetFocus( bool bFocus )
{
	if ( !bFocus )
		CancelAction();
}

void CMouseTranslator::CancelAction()
{
	switch ( eActionState )
	{
		case AS_SELECT:
			RaiseEvent( "cancel_selection" );
			break;
		case AS_MOVE:
			RaiseEvent( "cancel_direction" );
			break;
	}
	eActionState = AS_NONE;
	eButtonState = BS_NONE;
}

int CMouseTranslator::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pSelector );
	if ( saver.IsReading() ) 
		ResetState();
	return 0;
}

bool CMouseTranslator::IsDownSamePos( const CVec2 &vPos ) const
{
	return fabs( vDownPos - vPos ) < MIN_ACTION_DIST;
}

bool CMouseTranslator::IsDownSameTime( const NTimer::STime &curTime ) const
{
	return curTime - downTime < MIN_ACTION_TIME;
}

REGISTER_SAVELOAD_CLASS( 0x1507B440, CMouseTranslatorB1 );
REGISTER_SAVELOAD_CLASS( 0x1117AB80, CMouseTranslatorB2Game );
REGISTER_SAVELOAD_CLASS( 0x1117AB81, CMouseTranslatorB2Replay );


