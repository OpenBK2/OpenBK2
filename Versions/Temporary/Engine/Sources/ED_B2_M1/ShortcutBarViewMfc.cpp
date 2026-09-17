#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "ShortcutBarView.h"

#include "MapEditorLib/DefaultShortcutBar.h"
#include "MapEditorLib/DefaultTabWindow.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/MfcWidget.h"

#include <cstdlib>
#include <vector>

// The shortcut bar as it has always been: CDefaultShortcutBar, a Stingray
// SECShortcutBar, with a CDefault3DTabWindow per bar. Every call here is one the
// editors made themselves before, in the order they made them.

namespace
{
	class CMfcShortcutBarView : public NShortcutBar::IView
	{
		CDefaultShortcutBar bar;
		// Borrowed: the bar's shortcut list owns them.
		std::vector<CDefault3DTabWindow*> tabWindows;
		CWndWidget widget { &bar };

		CDefault3DTabWindow* TabWindow( int nBar ) const
		{
			return ( ( nBar >= 0 ) && ( nBar < static_cast<int>( tabWindows.size() ) ) ) ? tabWindows[nBar] : nullptr;
		}

	public:
		virtual bool Create( IWidget *pPane, unsigned nControlID )
		{
			return bar.Create( ToCWnd( pPane ), WS_CHILD | WS_VISIBLE | SEC_OBS_VERT | SEC_OBS_ANIMATESCROLL, nControlID ) != FALSE;
		}

		virtual void Destroy()
		{
			if ( bar.GetSafeHwnd() != 0 )
			{
				bar.DestroyWindow();
			}
		}

		virtual IWidget* GetWidget()
		{
			return &widget;
		}

		virtual void Show( bool bShow )
		{
			if ( bar.GetSafeHwnd() != 0 )
			{
				bar.ShowWindow( bShow ? SW_SHOW : SW_HIDE );
			}
		}

		virtual int BeginBar( unsigned nTabCommandHandlerID, unsigned nTabCommandID )
		{
			CDefault3DTabWindow *const pTabWindow = bar.AddNewShortcut( static_cast<CDefault3DTabWindow*>( 0 ) );
			if ( pTabWindow == 0 )
			{
				return -1;
			}
			if ( nTabCommandHandlerID != INVALID_COMMAND_HANDLER_ID )
			{
				pTabWindow->SetCommandHandlerID( nTabCommandHandlerID, nTabCommandID );
			}
			pTabWindow->Create( &bar, WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM | TWS_DRAW_3D_NORMAL );
			tabWindows.push_back( pTabWindow );
			return static_cast<int>( tabWindows.size() ) - 1;
		}

		virtual bool AddTab( int nBar, const std::string &rszLabel, const NShortcutBar::TPaletteFactory &rFactory )
		{
			CDefault3DTabWindow *const pTabWindow = TabWindow( nBar );
			if ( pTabWindow == nullptr )
			{
				return false;
			}
			CWnd *const pPalette = rFactory( pTabWindow );
			if ( pPalette == 0 )
			{
				return false;
			}
			pTabWindow->AddTab( pPalette, rszLabel.c_str() );
			return true;
		}

		virtual void ActivateTab( int nBar, int nTab )
		{
			if ( CDefault3DTabWindow *const pTabWindow = TabWindow( nBar ) )
			{
				pTabWindow->ActivateTab( nTab );
			}
		}

		virtual void EndBar( int nBar, const std::string &rszLabel )
		{
			if ( CDefault3DTabWindow *const pTabWindow = TabWindow( nBar ) )
			{
				bar.AddBar( pTabWindow, rszLabel.c_str(), true );
			}
		}

		virtual void SelectBar( int nBar )
		{
			bar.SelectPane( nBar );
		}

		virtual void SetCommandHandlerID( unsigned nCommandHandlerID, unsigned nCommandID )
		{
			bar.SetCommandHandlerID( nCommandHandlerID, nCommandID );
		}
	};
}


namespace NShortcutBar
{
	IView* CreateMfc()
	{
		return new CMfcShortcutBarView();
	}


	IView* Create()
	{
#ifdef OBK2_WITH_WX
		// The same flag every migrated piece follows -- and the one the palettes'
		// Create functions follow, which is what gives a wx bar wx palettes.
		if ( NToolkit::UseWxViews() )
		{
			return CreateWx();
		}
#endif
		return CreateMfc();
	}
}
