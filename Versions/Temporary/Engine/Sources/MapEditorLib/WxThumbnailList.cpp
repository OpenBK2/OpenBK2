#include "WxThumbnailList.h"

#ifdef __WXGTK__
#include <wx/imaglist.h>
#include <wx/utils.h>
#include <gtk/gtk.h>

struct CWxThumbnailList::SImpl
{
	enum { LABEL, PIXBUF, IMAGE, DATA, COLUMNS };
	CWxThumbnailList *owner;
	GtkListStore *store = gtk_list_store_new( COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_INT, G_TYPE_UINT64 );
	GtkWidget *view = gtk_icon_view_new_with_model( GTK_TREE_MODEL(store) );
	wxImageList *normal = nullptr, *small = nullptr;
	long style = 0;

	explicit SImpl( CWxThumbnailList *window ) : owner(window)
	{
		// Keep callbacks' widget alive until we disconnect them, even if its
		// native parent starts destruction before the wx wrapper is released.
		g_object_ref_sink( view );
	}
	~SImpl()
	{
		g_signal_handlers_disconnect_by_data( view, this );
		g_object_unref( view );
		g_object_unref( store );
	}
	bool Iter( long index, GtkTreeIter *iter ) const
	{
		return index >= 0 && gtk_tree_model_iter_nth_child( GTK_TREE_MODEL(store), iter, nullptr, index );
	}
	long Selected() const
	{
		GList *paths = gtk_icon_view_get_selected_items( GTK_ICON_VIEW(view) );
		const long result = paths ? gtk_tree_path_get_indices( static_cast<GtkTreePath*>(paths->data) )[0] : -1;
		g_list_free_full( paths, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free) );
		return result;
	}
	void UpdateImage( GtkTreeIter *iter )
	{
		int index = -1;
		gtk_tree_model_get( GTK_TREE_MODEL(store), iter, IMAGE, &index, -1 );
		wxImageList *images = (style & wxLC_ICON) ? normal : small;
		wxBitmap bitmap;
		if ( images && index >= 0 && index < images->GetImageCount() ) bitmap = images->GetBitmap(index);
		// GtkListStore retains the pixbuf, including its alpha or bitmap mask.
		gtk_list_store_set( store, iter, PIXBUF, bitmap.IsOk() ? bitmap.GetPixbuf() : nullptr, -1 );
	}
	void UpdateImages()
	{
		GtkTreeIter iter;
		for ( bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter); valid;
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter) ) UpdateImage( &iter );
	}
	void ContextMenu( const wxPoint &point )
	{
		wxContextMenuEvent event( wxEVT_CONTEXT_MENU, owner->GetId(), point );
		event.SetEventObject( owner );
		owner->GetEventHandler()->ProcessEvent( event );
	}
};

CWxThumbnailList::CWxThumbnailList( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style )
	: impl( new SImpl(this) )
{
	GtkWidget *scroll = gtk_scrolled_window_new( nullptr, nullptr );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_IN );
	gtk_container_add( GTK_CONTAINER(scroll), impl->view );
	gtk_icon_view_set_selection_mode( GTK_ICON_VIEW(impl->view), GTK_SELECTION_SINGLE );
	gtk_icon_view_set_text_column( GTK_ICON_VIEW(impl->view), SImpl::LABEL );
	gtk_icon_view_set_pixbuf_column( GTK_ICON_VIEW(impl->view), SImpl::PIXBUF );
	gtk_icon_view_set_tooltip_column( GTK_ICON_VIEW(impl->view), SImpl::LABEL );
	gtk_icon_view_set_margin( GTK_ICON_VIEW(impl->view), 4 );
	gtk_icon_view_set_row_spacing( GTK_ICON_VIEW(impl->view), 4 );
	gtk_icon_view_set_column_spacing( GTK_ICON_VIEW(impl->view), 4 );
	Create( parent, id, scroll );
	Disown(); // wx owns/destroys the native scrolled window and its child view.
	SetWindowStyleFlag( style );
	SetInitialSize( size );
	if ( pos != wxDefaultPosition ) Move( pos );
	gtk_widget_show_all( scroll );

	g_signal_connect( impl->view, "selection-changed", G_CALLBACK(+[]( GtkIconView *, gpointer data ) {
		auto *self = static_cast<SImpl*>(data);
		const long index = self->Selected();
		if ( index < 0 ) return;
		wxListEvent event( wxEVT_LIST_ITEM_SELECTED, self->owner->GetId() );
		event.SetEventObject( self->owner );
		event.SetIndex( index );
		self->owner->GetEventHandler()->ProcessEvent( event );
	}), impl.get() );
	g_signal_connect( impl->view, "button-press-event", G_CALLBACK(+[]( GtkWidget *widget, GdkEventButton *event, gpointer data ) -> gboolean {
		if ( event->button != 3 ) return FALSE;
		auto *self = static_cast<SImpl*>(data);
		if ( GtkTreePath *path = gtk_icon_view_get_path_at_pos(GTK_ICON_VIEW(widget), event->x, event->y) )
		{
			gtk_icon_view_select_path( GTK_ICON_VIEW(widget), path );
			gtk_icon_view_set_cursor( GTK_ICON_VIEW(widget), path, nullptr, FALSE );
			gtk_tree_path_free( path );
		}
		self->ContextMenu( wxGetMousePosition() );
		return TRUE;
	}), impl.get() );
	g_signal_connect( impl->view, "popup-menu", G_CALLBACK(+[]( GtkWidget *, gpointer data ) -> gboolean {
		static_cast<SImpl*>(data)->ContextMenu( wxDefaultPosition );
		return TRUE;
	}), impl.get() );
}

CWxThumbnailList::~CWxThumbnailList() = default;

void CWxThumbnailList::SetFocus() { gtk_widget_grab_focus( impl->view ); }

void CWxThumbnailList::SetWindowStyleFlag( long style )
{
	const bool sort = (style & wxLC_SORT_ASCENDING) != 0;
	if ( sort != ((impl->style & wxLC_SORT_ASCENDING) != 0) )
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE(impl->store),
			sort ? SImpl::LABEL : GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING );
	impl->style = style;
	m_windowStyle = style;
	const bool thumbnails = (style & wxLC_ICON) != 0;
	gtk_icon_view_set_item_orientation( GTK_ICON_VIEW(impl->view), thumbnails ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL );
	gtk_icon_view_set_columns( GTK_ICON_VIEW(impl->view), thumbnails ? -1 : 1 );
	gtk_icon_view_set_item_width( GTK_ICON_VIEW(impl->view), thumbnails ? FromDIP(84) : -1 );
	impl->UpdateImages();
	Arrange();
}

void CWxThumbnailList::SetImageList( wxImageList *images, int which )
{
	if ( which == wxIMAGE_LIST_NORMAL ) impl->normal = images;
	else if ( which == wxIMAGE_LIST_SMALL ) impl->small = images;
	impl->UpdateImages();
}

bool CWxThumbnailList::Arrange( int ) { gtk_widget_queue_resize( impl->view ); return true; }

long CWxThumbnailList::InsertItem( long index, const wxString &label, int image )
{
	GtkTreeIter iter;
	gtk_list_store_insert_with_values( impl->store, &iter, index, SImpl::LABEL, label.utf8_str().data(),
		SImpl::IMAGE, image, SImpl::DATA, guint64(0), -1 );
	impl->UpdateImage( &iter );
	GtkTreePath *path = gtk_tree_model_get_path( GTK_TREE_MODEL(impl->store), &iter );
	const long result = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free( path );
	return result;
}

bool CWxThumbnailList::DeleteAllItems() { gtk_list_store_clear( impl->store ); return true; }
long CWxThumbnailList::GetItemCount() const { return gtk_tree_model_iter_n_children( GTK_TREE_MODEL(impl->store), nullptr ); }

bool CWxThumbnailList::SetItemData( long index, wxUIntPtr data )
{
	GtkTreeIter iter;
	if ( !impl->Iter(index, &iter) ) return false;
	gtk_list_store_set( impl->store, &iter, SImpl::DATA, guint64(data), -1 );
	return true;
}

wxUIntPtr CWxThumbnailList::GetItemData( long index ) const
{
	GtkTreeIter iter;
	guint64 data = 0;
	if ( impl->Iter(index, &iter) ) gtk_tree_model_get( GTK_TREE_MODEL(impl->store), &iter, SImpl::DATA, &data, -1 );
	return wxUIntPtr(data);
}

wxString CWxThumbnailList::GetItemText( long index ) const
{
	GtkTreeIter iter;
	if ( !impl->Iter(index, &iter) ) return {};
	gchar *text = nullptr;
	gtk_tree_model_get( GTK_TREE_MODEL(impl->store), &iter, SImpl::LABEL, &text, -1 );
	const wxString result = wxString::FromUTF8( text );
	g_free( text );
	return result;
}

long CWxThumbnailList::GetNextItem( long index, int, int state ) const
{
	const long next = state & wxLIST_STATE_SELECTED ? impl->Selected() : index + 1;
	return next > index && next < GetItemCount() ? next : -1;
}

bool CWxThumbnailList::SetItemState( long index, long state, long mask )
{
	if ( index < 0 || index >= GetItemCount() ) return false;
	GtkTreePath *path = gtk_tree_path_new_from_indices( int(index), -1 );
	if ( mask & wxLIST_STATE_SELECTED )
	{
		if ( state & wxLIST_STATE_SELECTED ) gtk_icon_view_select_path( GTK_ICON_VIEW(impl->view), path );
		else gtk_icon_view_unselect_path( GTK_ICON_VIEW(impl->view), path );
	}
	if ( mask & state & wxLIST_STATE_FOCUSED ) gtk_icon_view_set_cursor( GTK_ICON_VIEW(impl->view), path, nullptr, FALSE );
	gtk_tree_path_free( path );
	return true;
}

bool CWxThumbnailList::EnsureVisible( long index )
{
	if ( index < 0 || index >= GetItemCount() ) return false;
	GtkTreePath *path = gtk_tree_path_new_from_indices( int(index), -1 );
	gtk_icon_view_scroll_to_path( GTK_ICON_VIEW(impl->view), path, FALSE, 0, 0 );
	gtk_tree_path_free( path );
	return true;
}
#endif
