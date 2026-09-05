#pragma once

#include "MapEditorLib_export.h"

//! The shell's own UI font, for dialogs whose templates predate it.
//!
//! Every dialog in the editor names its font in its resource template, and the
//! templates are the 2005 ones: 8 point "MS Sans Serif" on twenty seven of
//! them and 8 point "MS Shell Dlg" on seventeen. Neither is what Windows has
//! drawn its own interface in since Vista, and "MS Sans Serif" in particular
//! has no substitution to a scalable face, so it resolves to a raster font --
//! unantialiased, and sitting next to themed buttons that are drawn properly.
//! The resource scripts are the released ones and match the retail editor
//! exactly, so this is faithful rather than broken; it is just very old.
namespace NEditorFont
{
	//! Put the shell font on this window and every control under it.
	//!
	//! A font swap only: nothing is moved, resized or re-laid-out. That is
	//! safe because the font is first shrunk to the cell the template counted
	//! on -- see ShellFont.cpp, where the reason is written out.
	MAPEDITORLIB_EXPORT void ApplyShellFont( CWnd *pWnd );
}
