#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MenuDropDownView.h"
#include "MDDLDialog.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The undo and redo drop-down as it has always been, CMDDLDialog, behind the
// boundary; the dispatcher; and what choosing an entry does, which both lists
// call.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}


	// What CControllerContainer::UndoArrow and RedoArrow did with their
	// CMDDLDialog member, moved here unchanged.
	class CMfcMenuDropDown : public NMenuDropDown::IView
	{
		CWnd *pwndParent;
		CMDDLDialog dialog;

	public:
		explicit CMfcMenuDropDown( CWnd *_pwndParent ) : pwndParent( _pwndParent ) {}

		virtual void Show( int nX, int nY, unsigned nCommandID, const std::list<std::string> &rEntries )
		{
			if ( !::IsWindow( dialog.m_hWnd ) )
			{
				dialog.Create( CMDDLDialog::IDD, pwndParent );
			}
			CRect dialogRect;
			dialog.SetParams( nCommandID, rEntries );
			dialog.GetWindowRect( &dialogRect );
			dialog.MoveWindow( nX, nY, dialogRect.Width(), dialogRect.Height(), true );
			dialog.ShowWindow( SW_SHOW );
		}
	};
}


namespace NMenuDropDown
{
	void Choose( unsigned nCommandID, int nIndex )
	{
		if ( nIndex < 0 )
		{
			return;
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_CONTROLLER_CONTAINER, nCommandID,
																												 static_cast<uintptr_t>( nIndex ) );
	}


	IView* CreateMfc( IWidget *pParent )
	{
		return new CMfcMenuDropDown( ToCWnd( pParent ) );
	}


	IView* Create( IWidget *pParent )
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return CreateWx( pParent );
		}
#endif
		return CreateMfc( pParent );
	}
}
