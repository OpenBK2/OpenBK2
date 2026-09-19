#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "MapEditorLib/MfcWidget.h"
#include "ModelView.h"
#include "ModelWindow.h"
#include "ED_B2_M1Dll.h"

#include <cstdlib>

// The model palette as it has always been: CModelWindow, a CResizeDialog over
// IDD_TAB_MODEL_TOOL with six combo boxes, four edit boxes with their buttons,
// five check boxes and two radio groups.
//
// The window class is untouched apart from taking its whole command dispatch
// from CModelCommands. This is its creation, moved out of
// CModelEditor::CreateControls.

namespace
{
	class CMfcModelView : public NModelView::IView
	{
		CModelWindow window;
		CWndWidget widget { &window };
		bool bCreated = false;

	public:
		virtual bool Create( IWidget *pPane )
		{
			// The dialog template lives in this module's resources, not the
			// executable's, so the resource handle is swapped for the call and put
			// back. Exactly what CreateControls did.
			AfxSetResourceHandle( theEDB2M1Instance );
			bCreated = window.Create( CModelWindow::IDD, ToCWnd( pPane ) ) != FALSE;
			AfxSetResourceHandle( AfxGetInstanceHandle() );
			return bCreated;
		}

		virtual void Destroy()
		{
			if ( window.GetSafeHwnd() != 0 )
			{
				window.DestroyWindow();
			}
		}

		virtual void Show( bool bShow )
		{
			if ( window.GetSafeHwnd() != 0 )
			{
				window.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
			}
		}

		virtual IWidget* GetWidget()
		{
			return bCreated ? &widget : 0;
		}
	};
}


namespace NModelView
{
	IView* CreateMfc()
	{
		return new CMfcModelView();
	}


	IView* Create()
	{
		// The same flag every migrated piece follows, so a session runs either
		// the MFC set or the wx set.
		if ( NToolkit::UseWxViews() )
		{
			return CreateWx();
		}
		return CreateMfc();
	}
}
