#include "stdafx.h"

#include "FocusMemory.h"

// wxWindowRef is the typedef, in weakref.h rather than window.h.
#include <wx/weakref.h>
#include <wx/window.h>

// wxWindowRef is a weak reference: wx clears it when the window it names is
// destroyed, so asking whether it is still there is asking the toolkit rather
// than asking the operating system about a handle it may have handed to
// someone else since. See FocusMemory.h.
struct CFocusMemory::SImpl
{
	wxWindowRef previous;
};


CFocusMemory::CFocusMemory() : pImpl( new SImpl )
{
}


CFocusMemory::~CFocusMemory() = default;


void CFocusMemory::Remember()
{
	// FindFocus is the focus in this application, which is what GetFocus
	// answered: null when the focus is in another one.
	pImpl->previous = wxWindow::FindFocus();
}


void CFocusMemory::Restore()
{
	if ( wxWindow *const pWindow = pImpl->previous )
	{
		// Only if it can take the focus. A window that has been disabled since
		// -- which a long command may well have done -- would otherwise be given
		// it and the focus would go nowhere.
		if ( pWindow->IsEnabled() && pWindow->IsShownOnScreen() )
		{
			pWindow->SetFocus();
		}
	}
	pImpl->previous = nullptr;
}
