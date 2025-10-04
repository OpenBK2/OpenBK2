#include "stdafx.h"
#include "UI/commandparam.h"
#include "UI/dbuserinterface.h"
#include "Misc/2Darray.h"
#include "stats_b2_m1/IconsSet.h"
#include "InterfaceMisc.h"
#include "GameXClassIDs.h"
#include "UI/SceneClassIDs.h"
#include "SceneB2/Scene.h"
#include "Misc/StrProc.h"
#include "InterfaceState.h"

#include <zconf.h>

// Интерфейс MessageBox
//
// Позволяет выбирать вид диалога и задавать текст для него. Диалог автоматически переразмеривается
// под размер текста.
//
// Реализация
//
// Интерфейс содержит один экран с набором основных окон для каждого вида MessageBox.
// Добавление нового вида MessageBox осуществляется добавлением нового основного окна.
// Основное окно идентифицируется своим именем.
// Изначально все основные окна должны быть невидимыми.
// При запуске интерфейса заданное основное окно делается видимым.
//
// Основное окно должно содержать окно с именем "MessageBoxTextWindow", которое определяет 
// расположение текста. Это окно должно содержать WindowTextView с именем "MessageBoxTextView",
// в который помещается текст MessageBox.
//
// Закрытие диалога осуществляется посылкой одного из сообщений, зарегистрированных в интерфейсе.
// При закрытии диалога основное окно должно быть сделано невидимым (в редакторе).
// При закрытии диалога отправляется соответствующее сообщение предыдущему интерфейсу.
//
// Минимальный размер диалога определяется его шаблоном, заданным в редакторе.

const float TEXT_VIEW_NICE_X2Y = 2.0f;

// CInterfaceMessageBox

bool CInterfaceMessageBox::CReactions::Execute( const string &szSender, const string &szReaction )
{
	return false;
}

// CInterfaceMessageBox

CInterfaceMessageBox::CInterfaceMessageBox() : 
	CInterfaceScreenBase( "MessageBoxScreen", "message_box_screen" )
{
	AddObserver( "message_box_ok", &CInterfaceMessageBox::MsgOk );
	AddObserver( "message_box_cancel", &CInterfaceMessageBox::MsgCancel );
	AddObserver( "message_box_yes", &CInterfaceMessageBox::MsgYes );
	AddObserver( "message_box_no", &CInterfaceMessageBox::MsgNo );
}

CInterfaceMessageBox::~CInterfaceMessageBox()
{
	if ( pScreen ) 
		Scene()->RemoveScreen( pScreen );
}

int CInterfaceMessageBox::CReactions::Check( const string &szCheckName ) const
{
	return 0;
}

bool CInterfaceMessageBox::Init()
{
	// load screen
	pScreen = MakeObjectVirtual<IScreen>( UI_SCREEN );
	pReactions = new CReactions( pScreen );
	if ( AddUIScreen( pScreen, "MessageBoxScreen", pReactions ) == false )
		return false;

	if ( CInterfaceScreenBase::Init() == false ) 
		return false;

	return true;
}

void CInterfaceMessageBox::SetParams( const string &szName, const wstring &szText )
{
	IWindow *pWindow = pScreen->GetChild( szName, true );
	NI_ASSERT( pWindow, StrFmt( "Main window: '%s' for message box not found", szName.c_str() ) );
	pWindow->ShowWindow( true );

	IWindow *pTextWindow = pScreen->GetVisibleChild( "MessageBoxTextWindow", true );
	NI_ASSERT( pTextWindow, "Text window 'MessageBoxTextWindow' for message box not found" );
	CTPoint<int> sizeTextWindow;
	pTextWindow->GetPlacement( 0, 0, &sizeTextWindow.x, &sizeTextWindow.y );
	
	IWindow *pTextViewWindow = pScreen->GetVisibleChild( "MessageBoxTextView", true );
	ITextView *pTextView = dynamic_cast<ITextView*>( pTextViewWindow );
	NI_ASSERT( pTextView, "Text view window 'MessageBoxTextView' for message box not found" );

	// переразмерим диалог
	ResizeTextView( pTextView, szText, sizeTextWindow.x );
	const CTPoint<int> &sizeTextView = pTextView->GetSize();
	
	int nDeltaX = Max( 0, sizeTextView.x - sizeTextWindow.x );
	int nDeltaY = Max( 0, sizeTextView.y - sizeTextWindow.y );
	
	CTPoint<int> posWindow;
	CTPoint<int> sizeWindow;
	pWindow->GetPlacement( &posWindow.x, &posWindow.y, &sizeWindow.x, &sizeWindow.y );
	pWindow->SetPlacement( posWindow.x - nDeltaX / 2, posWindow.y - nDeltaY / 2, 
		sizeWindow.x + nDeltaX, sizeWindow.y + nDeltaY, EWPF_ALL );
}

void CInterfaceMessageBox::OnGetFocus( bool bFocus )
{
	CInterfaceScreenBase::OnGetFocus( bFocus );
}

void CInterfaceMessageBox::MsgOk( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "message_box_ok" );
}

void CInterfaceMessageBox::MsgCancel( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "message_box_cancel" );
}

void CInterfaceMessageBox::MsgYes( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "message_box_yes" );
}

void CInterfaceMessageBox::MsgNo( const SGameMessage &msg )
{
	NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "message_box_no" );
}

bool CInterfaceMessageBox::ProcessEvent( const SGameMessage &msg )
{
	return CInterfaceScreenBase::ProcessEvent( msg );
}

void CInterfaceMessageBox::ResizeTextView( ITextView *pTextView, const wstring &szText, int nMinX )
{
	pTextView->SetText( pTextView->GetDBText() + szText );
	pTextView->SetWidth( nMinX );
	const CTPoint<int> &size = pTextView->GetSize();
	int nNiceX = Max( Max( nMinX, size.x), (int)sqrtf( size.x * size.y * TEXT_VIEW_NICE_X2Y ) );
	pTextView->SetWidth( nNiceX );
	const CTPoint<int> &size2 = pTextView->GetSize();
	if ( size2.x < nNiceX ) 
		pTextView->SetWidth( size2.x );
}

bool CInterfaceMessageBox::StepLocal( bool bAppActive )
{
	if ( IInterfaceBase *pInterface = NMainLoop::GetPrevInterface( this ) )
		pInterface->Step( bAppActive );

	return CInterfaceScreenBase::StepLocal( bAppActive );
}

// CICMessageBox

void CICMessageBox::PreCreate()
{
}

void CICMessageBox::PostCreate( IInterface *pInterface )
{
	pInterface->SetParams( szMainWindowName, szText );
	NMainLoop::PushInterface( pInterface );
}

void CICMessageBox::Configure( const char *pszConfig )
{
	// Format: type:message_box_type;text:last_of_string_is_a_message_box_text

	const char *pCurrPos = pszConfig;
	while ( *pCurrPos != '0' )
	{
		const char *p = strchr( pCurrPos, ':' );
		NI_ASSERT( p , StrFmt( "incorrect string format: %s", pszConfig ) );
		string s( pCurrPos, p - pCurrPos );
		if ( s == "type" )
		{
			const char *p2 = strchr( p + 1, ';' );
			NI_ASSERT( p2 , StrFmt( "incorrect string format: %s", pszConfig ) );
			szMainWindowName = string( p + 1, p2 - (p + 1) );
			pCurrPos = p2 + 1;
		}
		else if ( s == "text" )
		{
			NStr::ToUnicode( &szText, string( p + 1 ) );
			break;
		}
		else
		{
			NI_ASSERT( 0, StrFmt( "incorrect string format: %s", pszConfig ) );
		}
	}
}

string CICMessageBox::MakeConfigString( const string &MessageBoxType, const wstring &szText )
{
	string szConfig = "type:";
	szConfig += MessageBoxType + ";text:" + NStr::ToMBCS( szText );
	return szConfig;
}

// CICPreviousMenu

void CICPreviousMenu::Exec()
{
	NI_ASSERT( NMainLoop::GetTopInterface(), "No interface to pop" );
	NMainLoop::PopInterface();
	if ( !szResult.empty() )
		NInput::PostEvent( szResult, 0, 0 );
}

void CICPreviousMenu::Configure( const char *pszConfig )
{
	if ( pszConfig )
		szResult = pszConfig;
}

// CICRemoveInterfacesUpTo

void CICRemoveInterfacesUpTo::Exec()
{
	for ( IInterfaceBase *pI = NMainLoop::GetTopInterface(); pI != 0; pI = NMainLoop::GetTopInterface() )
	{
		CInterfaceScreenBase * pInterface = dynamic_cast<CInterfaceScreenBase*>( pI );
		if ( pInterface )
		{
			if ( pInterface->GetInterfaceType() == szInterfaceID )
				break;
		}
		NMainLoop::PopInterface();
	}
	/*for ( IInterfaceBase *pI = NMainLoop::GetTopInterface(); pI != 0; pI = NMainLoop::GetPrevInterface( pI ) )
	{
		NMainLoop::Command( ML_COMMAND_PREVIOUS_MENU, "" );
		CInterfaceScreenBase * pInterface = dynamic_cast<CInterfaceScreenBase*>( pI );
		if ( pInterface && pInterface->GetInterfaceType() == szInterfaceID )
			break;
	}*/
}

void CICRemoveInterfacesUpTo::Configure( const char *pszConfig )
{
	if ( pszConfig )
		szInterfaceID = pszConfig;
}

// CICClearInterfaces

void CICClearInterfaces::Exec()
{
	NMainLoop::ResetStack();
}

// CICSuppressEnableFocus

void CICSuppressEnableFocus::Exec()
{
	InterfaceState()->SetSuppressEnableFocus( true );
}

// CICHideUnfocusedScreen

void CICHideUnfocusedScreen::Exec()
{
	if ( CDynamicCast<CInterfaceScreenBase> pInterface = NMainLoop::GetTopInterface() )
	{
		pInterface->HideUnfocusedScreen();
	}
}

REGISTER_SAVELOAD_CLASS( 0x170BE401, CInterfaceMessageBox )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_MESSAGE_BOX, CICMessageBox )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_PREVIOUS_MENU, CICPreviousMenu )
REGISTER_SAVELOAD_CLASS_NM( 0x170BE402, CReactions, CInterfaceMessageBox )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_SUPPRESS_ENABLE_FOCUS, CICSuppressEnableFocus )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_HIDE_UNFOCUSED_SCREEN, CICHideUnfocusedScreen )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_HIDE_ALL_UP_TO, CICRemoveInterfacesUpTo )
REGISTER_SAVELOAD_CLASS( ML_COMMAND_CLEAR_INTERFACES, CICClearInterfaces )


