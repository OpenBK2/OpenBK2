#include "stdafx.h"

#include "LogView.h"
#include "LogWindow.h"

#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/MfcWidget.h"
#include "ResourceDefines.h"
#include "Scintilla/Scintilla.h"

#include <cstdlib>
#include <cstring>

// The Log Window's contents as they have always been: CLogWindow, which is
// CScintillaEditorWindow, which is the 2005 Scintilla this tree vendors.
//
// Nothing here is new. It is the code that used to sit inline in CDWLog, moved
// behind ILogView so that the pane stops speaking Scintilla and something else
// can take its place. That move is the whole point of the exercise; this file
// existing unchanged in behaviour is how the wx one can be judged against it.

namespace
{
	class CLogViewScintilla : public ILogView
	{
		CLogWindow wndContents;

		void SetStyleColour( ELogOutputType eLogOutputType )
		{
			const NLogView::SLogColour colour = NLogView::GetColour( eLogOutputType );
			const int nScintillaColour = colour.nRed |
																	 ( colour.nGreen << 8 ) |
																	 ( colour.nBlue << 16 );
			wndContents.Command( SCI_STYLESETFORE, eLogOutputType, nScintillaColour );
		}

	public:
		virtual bool Create( IWidget *pParentPane, ICommandHandler *pSelectionHandler )
		{
			CWnd *const pwndPane = ToCWnd( pParentPane );
			if ( pwndPane == nullptr )
			{
				return false;
			}
			if ( !wndContents.CreateEx( pwndPane, WS_EX_CLIENTEDGE, WS_CHILD | WS_VISIBLE,
																	CRect( 0, 0, 0, 0 ), IDC_LOG_WINDOW ) )
			{
				return false;
			}
			wndContents.SetSelectionHandler( pSelectionHandler );
			wndContents.Command( SCI_SETREADONLY, false );
			// One Scintilla style per log type. The pack back to 0x00BBGGRR is
			// here, at the one call that wants Scintilla's byte order, instead of
			// being three literals nobody could read.
			SetStyleColour( LT_NORMAL );
			SetStyleColour( LT_IMPORTANT );
			SetStyleColour( LT_ERROR );
			wndContents.ShowWindow( SW_SHOW );
			return true;
		}

		virtual bool IsCreated() const
		{
			return wndContents.GetSafeHwnd() != nullptr;
		}

		virtual void SetBounds( const CTRect<int> &rBounds )
		{
			if ( IsCreated() )
			{
				wndContents.SetWindowPos( 0, rBounds.left, rBounds.top,
																	rBounds.Width(), rBounds.Height(),
																	SWP_NOZORDER | SWP_NOACTIVATE );
			}
		}

		virtual void Show( bool bShow )
		{
			if ( IsCreated() )
			{
				wndContents.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
			}
		}

		virtual void Append( ELogOutputType eLogOutputType, const std::string &rszText )
		{
			// Unchanged from CDWLog::Append: remember where the caret and anchor
			// were, append, style just the new text, and only scroll to the end if
			// the caret was already there -- so appending does not yank the view
			// away from someone reading further up.
			const int nTextEnd = wndContents.Command( SCI_GETLENGTH );
			const int nPosition = wndContents.Command( SCI_GETCURRENTPOS );
			const int nAnchor = wndContents.Command( SCI_GETANCHOR );
			wndContents.Command( SCI_APPENDTEXT, rszText.size(), (sptr_t)( rszText.c_str() ) );
			wndContents.Command( SCI_STARTSTYLING, nTextEnd, 0x1f );
			wndContents.Command( SCI_SETSTYLING, rszText.size(), eLogOutputType );
			if ( ( nPosition == nAnchor ) && ( nPosition == nTextEnd ) )
			{
				wndContents.Command( SCI_GOTOPOS, wndContents.Command( SCI_GETLENGTH ) );
			}
		}

		virtual void Clear()
		{
			wndContents.Command( SCI_CLEARALL );
		}

		virtual void Redraw()
		{
			if ( IsCreated() )
			{
				wndContents.UpdateWindow();
			}
		}

		virtual void Copy()
		{
			wndContents.Command( SCI_COPY );
		}

		virtual void SelectAll()
		{
			wndContents.Command( SCI_SELECTALL );
		}

		virtual bool HasSelection() const
		{
			CLogWindow &rContents = const_cast<CLogWindow&>( wndContents );
			return rContents.Command( SCI_GETSELECTIONSTART ) != rContents.Command( SCI_GETSELECTIONEND );
		}

		virtual bool IsEmpty() const
		{
			CLogWindow &rContents = const_cast<CLogWindow&>( wndContents );
			return rContents.Command( SCI_GETLENGTH ) == 0;
		}
	};
}


namespace NLogView
{
	// The colours the editor has always used for its log, written as components.
	// Defined here because this translation unit is always compiled and the wx
	// one is not.
	SLogColour GetColour( ELogOutputType eLogOutputType )
	{
		switch ( eLogOutputType )
		{
			case LT_IMPORTANT:
				return SLogColour{ 0x22, 0x77, 0x22 };	// green
			case LT_ERROR:
				return SLogColour{ 0xff, 0x33, 0x33 };	// red, and it always was
			case LT_NORMAL:
			default:
				return SLogColour{ 0x00, 0x00, 0x00 };	// black
		}
	}


	ILogView* CreateScintillaLogView()
	{
		return new CLogViewScintilla();
	}


	ILogView* Create()
	{
#ifdef OBK2_WITH_WX
		// Chosen at run time so the two can be compared without rebuilding, which
		// is the only way a comparison actually gets made.
		const char *pszUseWx = std::getenv( "OBK2_WX_LOG" );
		if ( pszUseWx != nullptr && pszUseWx[0] != '0' && pszUseWx[0] != '\0' )
		{
			if ( ILogView *pWxView = CreateWxLogView() )
			{
				return pWxView;
			}
		}
#endif
		return CreateScintillaLogView();
	}
}
