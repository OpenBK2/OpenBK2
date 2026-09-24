#pragma once

#include <wx/listctrl.h>

#ifdef __WXGTK__
#include "MapEditorLib_export.h"
#include <wx/nativewin.h>
#include <memory>

// wxGTK's generic icon list flows into columns and Arrange() is a no-op.
// Use GTK's native wrapping icon view, keeping the palette's list events/data
// and the existing Windows control. The image lists are borrowed from the cache.
class MAPEDITORLIB_EXPORT CWxThumbnailList : public wxNativeWindow
{
	struct SImpl;
	std::unique_ptr<SImpl> impl;
public:
	CWxThumbnailList( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style );
	~CWxThumbnailList() override;
	void SetFocus() override;
	void SetWindowStyleFlag( long style ) override;
	void SetImageList( wxImageList *images, int which );
	bool Arrange( int flag = 0 );
	long InsertItem( long index, const wxString &label, int image = -1 );
	bool DeleteAllItems();
	long GetItemCount() const;
	bool SetItemData( long index, wxUIntPtr data );
	wxUIntPtr GetItemData( long index ) const;
	wxString GetItemText( long index ) const;
	long GetNextItem( long index, int geometry = wxLIST_NEXT_ALL, int state = wxLIST_STATE_DONTCARE ) const;
	bool SetItemState( long index, long state, long mask );
	bool EnsureVisible( long index );
};
#else
using CWxThumbnailList = wxListCtrl;
#endif
