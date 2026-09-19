#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "ProgressView.h"
#include "ProgressDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The progress dialog as it has always been: CProgressDialog over
// IDD_PROGRESS_SIMPLE, created modeless on the frame. The dialog class is
// unchanged; this is what CMainFrame used to do around it, and the dispatcher
// in front of both implementations.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}


	class CMfcProgressView : public NProgressView::IView
	{
		// By value, as CMainFrame held it: the window is created and destroyed
		// over and over, and this object outlives all of that.
		CProgressDialog dialog;

	public:
		virtual ~CMfcProgressView()
		{
			Destroy();
		}

		virtual bool Create( IWidget *pParent )
		{
			return dialog.Create( CProgressDialog::IDD, ToCWnd( pParent ) ) != FALSE;
		}

		virtual bool IsCreated() const
		{
			return ::IsWindow( dialog.GetSafeHwnd() ) != FALSE;
		}

		virtual void Show()
		{
			if ( IsCreated() )
			{
				dialog.ShowWindow( SW_SHOW );
				dialog.UpdateControls();
			}
		}

		virtual void Destroy()
		{
			if ( IsCreated() )
			{
				dialog.DestroyWindow();
			}
		}

		virtual void SetTitle( const std::string &rszTitle )
		{
			if ( IsCreated() )
			{
				dialog.SetProgressTitle( rszTitle );
			}
		}

		virtual void SetMessage( const std::string &rszMessage )
		{
			if ( IsCreated() )
			{
				dialog.SetProgressMessage( rszMessage );
			}
		}

		virtual void SetRange( int nStart, int nFinish )
		{
			if ( IsCreated() )
			{
				dialog.SetProgressRange( nStart, nFinish );
			}
		}

		virtual void SetPosition( int nPosition )
		{
			if ( IsCreated() )
			{
				dialog.SetProgressPosition( nPosition );
			}
		}

		virtual void IteratePosition()
		{
			if ( IsCreated() )
			{
				dialog.IterateProgressPosition();
			}
		}
	};
}


namespace NProgressView
{
	IView* CreateMfc()
	{
		return new CMfcProgressView;
	}


	IView* Create()
	{
		if ( UseWx() )
		{
			return CreateWx();
		}
		return CreateMfc();
	}
}
