#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "ED_B2_M1Dll.h"
#include "EdUnitStartCmd.h"
#include "UnitStartCmdDialog.h"

#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>

// The unit start command editor as it has always been: CEdUnitStartCmd over
// IDD_DLG_UNIT_START_CMD, created modeless and kept. The dialog class itself is
// unchanged apart from speaking the boundary's data and events; this is what
// the state used to do around it, and the dispatcher in front of both
// implementations.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}


	// Holds the dialog as the state used to, by CPtr: CEdUnitStartCmd is a
	// CObjectBase and is destroyed by its own refcount, so this owns a reference
	// rather than the object, and the window goes when the reference does.
	class CMfcUnitStartCmdDialog : public NUnitStartCmdDialog::IDialog
	{
		CPtr<CEdUnitStartCmd> pDialog;

	public:
		explicit CMfcUnitStartCmdDialog( NUnitStartCmdDialog::IListener *pListener )
			: pDialog( new CEdUnitStartCmd( pListener ) )
		{
		}

		bool Build( IWidget *pParent )
		{
			// The template is in the editor DLL and the app's resource handle is
			// the executable's, so it is switched for the call. CDialog::Create
			// does not do the switch DoModal does, which is why this is here and
			// why the state used to do it.
			const HINSTANCE hPreviousResource = AfxGetResourceHandle();
			AfxSetResourceHandle( theEDB2M1Instance );
			const BOOL bCreated = pDialog->Create( CEdUnitStartCmd::IDD, ToCWnd( pParent ) );
			AfxSetResourceHandle( hPreviousResource );
			if ( !bCreated )
			{
				return false;
			}
			// The template has no WS_VISIBLE, and the state hid it here anyway.
			pDialog->ShowWindow( SW_HIDE );
			return true;
		}

		virtual void Show( bool bShow )
		{
			pDialog->ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}

		virtual void SetDialogData( const NUnitStartCmdDialog::SData *pData )
		{
			pDialog->SetDialogData( pData );
		}

		virtual void GetDialogData( NUnitStartCmdDialog::SData *pData )
		{
			pDialog->GetDialogData( pData );
		}

		virtual void UpdateTarget( const std::string &rszTarget )
		{
			pDialog->UpdateTarget( rszTarget );
		}

		virtual int GetSelectedCommandType()
		{
			return pDialog->GetSelectedCommandType();
		}
	};
}


namespace NUnitStartCmdDialog
{
	IDialog* CreateMfc( IWidget *pParent, IListener *pListener )
	{
		CMfcUnitStartCmdDialog *pDialog = new CMfcUnitStartCmdDialog( pListener );
		if ( !pDialog->Build( pParent ) )
		{
			delete pDialog;
			return 0;
		}
		return pDialog;
	}


	IDialog* Create( IWidget *pParent, IListener *pListener )
	{
		if ( UseWx() )
		{
			return CreateWx( pParent, pListener );
		}
		return CreateMfc( pParent, pListener );
	}
}
