#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MoviesEditorView.h"
#include "MoviesEditorWindow.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The script movies editor as it has always been: CMoviesEditorWindow over
// IDD_DLG_MOVIES_EDITOR, created into the docking pane. The window class is
// untouched; this is what CMapInfoEditor::CreateControls used to do around it,
// and the dispatcher in front of both implementations.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}


	class CMfcMoviesEditorView : public NMoviesEditorView::IView
	{
		// By value, as CMapInfoEditor held it: the window registers itself as a
		// command handler from its constructor, so it exists before Create and
		// after Destroy.
		CMoviesEditorWindow window;
		bool bCreated = false;

	public:
		virtual ~CMfcMoviesEditorView()
		{
			Destroy();
		}

		virtual bool Create( IWidget *pPane )
		{
			bCreated = window.Create( ToCWnd( pPane ) );
			return bCreated;
		}

		virtual void Destroy()
		{
			if ( bCreated )
			{
				window.Destroy();
				bCreated = false;
			}
		}

		virtual void Show( bool bShow )
		{
			if ( bCreated )
			{
				window.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
			}
		}

		virtual IWidget* GetWidget()
		{
			return bCreated ? &widget : 0;
		}

	private:
		// Borrowed, and it has to outlive the call the pane makes with it, so it
		// is a member rather than a temporary at the call site.
		CWndWidget widget { &window };
	};
}


namespace NMoviesEditorView
{
	IView* CreateMfc()
	{
		return new CMfcMoviesEditorView;
	}


	IView* Create()
	{
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return CreateWx();
		}
#endif
		return CreateMfc();
	}
}
