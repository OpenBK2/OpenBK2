#pragma once

#include <memory>
#include <wx/gdicmn.h>

class wxWindow;

// A GTK popup above native render windows, with an empty mouse input region.
// Kept separate from the editor's headers so only its implementation needs GTK.
class CNativeDockHint
{
	struct SImpl;
	std::unique_ptr<SImpl> pImpl;

public:
	explicit CNativeDockHint( wxWindow *pParent );
	~CNativeDockHint();
	void Show( const wxRect &rScreenRect, bool bResizeHint );
	void Hide();
};
