#include "NativeDockHint.h"

#ifdef __WXGTK3__
#include <wx/settings.h>
#include <wx/window.h>
#include <gtk/gtk.h>

struct CNativeDockHint::SImpl
{
	GtkWidget *pWindow = nullptr;
	bool bResizeHint = false;
	bool bComposited = false;
	int nBorder = 3;

	static gboolean Draw( GtkWidget *pWidget, cairo_t *pContext, gpointer pData )
	{
		const SImpl &impl = *static_cast<SImpl*>( pData );
		const int nWidth = gtk_widget_get_allocated_width( pWidget );
		const int nHeight = gtk_widget_get_allocated_height( pWidget );
		const wxColour color = wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT );
		const double r = color.Red() / 255.0;
		const double g = color.Green() / 255.0;
		const double b = color.Blue() / 255.0;

		cairo_set_operator( pContext, CAIRO_OPERATOR_SOURCE );
		cairo_set_source_rgba( pContext, r, g, b,
			impl.bResizeHint || !impl.bComposited ? 1.0 : 0.25 );
		cairo_paint( pContext );
		if ( !impl.bResizeHint && nWidth > 2 * impl.nBorder && nHeight > 2 * impl.nBorder )
		{
			cairo_set_source_rgba( pContext, r, g, b, 1.0 );
			cairo_set_line_width( pContext, impl.nBorder );
			cairo_rectangle( pContext, impl.nBorder / 2.0, impl.nBorder / 2.0,
				nWidth - impl.nBorder, nHeight - impl.nBorder );
			cairo_stroke( pContext );
		}
		return TRUE;
	}
};

CNativeDockHint::CNativeDockHint( wxWindow *pParent ) : pImpl( new SImpl )
{
	pImpl->nBorder = pParent->FromDIP( 3 );
	pImpl->pWindow = gtk_window_new( GTK_WINDOW_POPUP );
	g_object_ref( pImpl->pWindow );
	GtkWindow *const pWindow = GTK_WINDOW( pImpl->pWindow );
	gtk_window_set_transient_for( pWindow, GTK_WINDOW( gtk_widget_get_toplevel( pParent->GetHandle() ) ) );
	gtk_window_set_destroy_with_parent( pWindow, TRUE );
	gtk_window_set_accept_focus( pWindow, FALSE );
	gtk_window_set_focus_on_map( pWindow, FALSE );
	gtk_window_set_type_hint( pWindow, GDK_WINDOW_TYPE_HINT_DND );
	gtk_widget_set_app_paintable( pImpl->pWindow, TRUE );

	GdkScreen *const pScreen = gtk_widget_get_screen( pParent->GetHandle() );
	gtk_window_set_screen( pWindow, pScreen );
	GdkVisual *const pVisual = gdk_screen_get_rgba_visual( pScreen );
	pImpl->bComposited = pVisual != nullptr && gdk_screen_is_composited( pScreen );
	if ( pImpl->bComposited )
	{
		gtk_widget_set_visual( pImpl->pWindow, pVisual );
	}
	g_signal_connect( pImpl->pWindow, "draw", G_CALLBACK( SImpl::Draw ), pImpl.get() );

	// The preview must never steal a pane drag, mouse release, or drop target.
	// A separate input shape also works when the X11 compositor is disabled.
	cairo_region_t *const pEmptyInput = cairo_region_create();
	gtk_widget_input_shape_combine_region( pImpl->pWindow, pEmptyInput );
	cairo_region_destroy( pEmptyInput );
}

CNativeDockHint::~CNativeDockHint()
{
	gtk_widget_destroy( pImpl->pWindow );
	g_object_unref( pImpl->pWindow );
}

void CNativeDockHint::Show( const wxRect &rScreenRect, bool bResizeHint )
{
	if ( rScreenRect.IsEmpty() )
	{
		Hide();
		return;
	}
	pImpl->bResizeHint = bResizeHint;
	gtk_window_move( GTK_WINDOW( pImpl->pWindow ), rScreenRect.x, rScreenRect.y );
	gtk_window_resize( GTK_WINDOW( pImpl->pWindow ), rScreenRect.width, rScreenRect.height );

	// Without compositing, use a hollow native window instead of obscuring the
	// proposed dock with an opaque rectangle. The narrow sash stays solid.
	cairo_rectangle_int_t outer = { 0, 0, rScreenRect.width, rScreenRect.height };
	cairo_region_t *const pShape = cairo_region_create_rectangle( &outer );
	if ( !pImpl->bComposited && !bResizeHint &&
		rScreenRect.width > 2 * pImpl->nBorder && rScreenRect.height > 2 * pImpl->nBorder )
	{
		cairo_rectangle_int_t inner = { pImpl->nBorder, pImpl->nBorder,
			rScreenRect.width - 2 * pImpl->nBorder, rScreenRect.height - 2 * pImpl->nBorder };
		cairo_region_subtract_rectangle( pShape, &inner );
	}
	gtk_widget_shape_combine_region( pImpl->pWindow, pShape );
	cairo_region_destroy( pShape );
	gtk_widget_show( pImpl->pWindow );
	gdk_window_raise( gtk_widget_get_window( pImpl->pWindow ) );
	gtk_widget_queue_draw( pImpl->pWindow );
}

void CNativeDockHint::Hide()
{
	gtk_widget_hide( pImpl->pWindow );
}
#endif
